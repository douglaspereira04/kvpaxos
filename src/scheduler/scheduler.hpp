#ifndef _KVPAXOS_SCHEDULER_H_
#define _KVPAXOS_SCHEDULER_H_

#include <condition_variable>
#include <memory>
#include <netinet/tcp.h>
#include <pthread.h>
#include <queue>
#include <deque>
#include <semaphore.h>
#include <shared_mutex>
#include <string>
#include <string.h>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <atomic>
#include <assert.h>
#include "input_graph.hpp"
#include "graph/graph.hpp"
#include "graph/partitioning.h"
#include "partition.hpp"
#include "request/request.hpp"
#include "queue/queue.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include <iostream>
#include "utils/utils.h"
#include <algorithm>
#include <limits.h>


namespace kvpaxos {

using namespace kvstorage;
using namespace std;


template <typename T, size_t TL = 0, size_t WorkerCapacity = 0, interval_type IntervalType = interval_type::OPERATIONS, size_t MaxSucessiveImbalances = 100>
class Scheduler{
typedef Partition<T, WorkerCapacity> partition_t;
typedef unordered_map<T, partition_t*> partition_map_t;
public:

    Scheduler() {}
    Scheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method,
                size_t queue_head_distance,
                float balance_threshold
    ) {
        __n_partitions = n_partitions;
        sucessive_imbalance = new uint32_t[__n_partitions];
        __in_queue_amount = new size_t[__n_partitions];
        if (queue_head_distance == 0) { //uses old tracking
            scheduling_queue = model::Queue<client_message>(SEM_VALUE_MAX, 0);
        } else {
            scheduling_queue = model::Queue<client_message>(queue_head_distance);
        }
        if constexpr(IntervalType == interval_type::MICROSECONDS){
            time_start = utils::now();
            time_interval = chrono::microseconds(repartition_interval);
            operation_start = 0;
            cross_operation_start = 0;
        } else if constexpr(IntervalType == interval_type::OPERATIONS){
            operation_start = 0;
            operation_interval = repartition_interval;
        }
        cross_partition_count = 0;
        repartition_method = repartition_method;

        round_robin_counter = 0;
        sync_counter = 0;
        __n_dispatched_requests = 0;

        partition_t::create_storage(__n_partitions);
        for (auto i = 0; i < __n_partitions; i++) {
            auto* partition = new partition_t(i);
            partitions_.emplace(i, partition);
            sucessive_imbalance[i] = 0b1;
        }
        data_to_partition = new partition_map_t();
        updated_data_to_partition = new partition_map_t();

        set_balance_threshold(balance_threshold);
        clear_imbalance_count();

        repartitioning.store(false, memory_order_seq_cst);
        update.store(false, memory_order_seq_cst);
        repartition.store(false, memory_order_seq_cst);

        scheduling_thread = thread(&Scheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::scheduling_loop, this);
        utils::set_affinity(2,scheduling_thread, scheduler_cpu_set);

        graph_thread = thread(&Scheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::update_graph_loop, this);
	    utils::set_affinity(3, graph_thread, graph_cpu_set);

        sem_init(&repart_semaphore, 0, 0);
        reparting_thread = thread(&Scheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::partitioning_loop, this);
	    utils::set_affinity(4, reparting_thread, reparting_cpu_set);

        client_message dummy;
        dummy.type = DUMMY;
        if constexpr(TL > 0){
            for (size_t i = 0; i < TL; i++)
            {
                graph_deletion_queue.push_back(dummy);
            }
        }

    }

    void run() {
        for (auto& kv : partitions_) {
            kv.second->start_worker_thread();
        }
    }

    void join(){
        scheduling_thread.join();
    }

    void set_balance_threshold(float balance_threshold){
        balance_threshold = balance_threshold;
    }


    inline void clear_imbalance_count() const{
        for (int i = 0; i < __n_partitions; i++) {
            sucessive_imbalance[i] = 0;//0b1;
        }
    }

    inline bool is_cross_partition_intensive() {
        bool cross_partition_intensive = false;
        if ( __n_dispatched_requests > operation_start){
            float cross_partition_ratio = (cross_partition_count - cross_operation_start)/ static_cast<float>(__n_dispatched_requests - operation_start);
            if (cross_partition_ratio > balance_threshold){
                sucessive_cross_partition_intensive += 1;
                if (sucessive_cross_partition_intensive > MaxSucessiveImbalances){
                    cross_partition_intensive = true;
                }
            } else {
                sucessive_cross_partition_intensive = 0; //sucessive_cross_partition_intensive  - (sucessive_cross_partition_intensive > 0);
            }
        }
        return cross_partition_intensive;
    }

    bool imbalance() {

        if (is_cross_partition_intensive()){
            sucessive_cross_partition_intensive = 0;
            clear_imbalance_count();
            return true;
        }

        bool imbalance = false;
        size_t sum = 0;
        int i = 0;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            size_t si = partition->request_queue_size();
            __in_queue_amount[i] = si;
            sum += si;
            i++;
        }

        float avg = static_cast<float>(sum)/__n_partitions;
        float threshold = avg * balance_threshold;
        for (i = 0; i < __n_partitions; i++) {
            if (abs(__in_queue_amount[i] - avg) > threshold){
                sucessive_imbalance[i] = sucessive_imbalance[i] + 1; //<< 1;
                if (sucessive_imbalance[i] > MaxSucessiveImbalances){//& (0b1 << MaxSucessiveImbalances)){
                    imbalance = true;
                    sucessive_cross_partition_intensive = 0;
                    clear_imbalance_count();
                    break;
                }
            } else {
                sucessive_imbalance[i] = 0; //sucessive_imbalance[i] - (sucessive_imbalance[i] > 0); // (sucessive_imbalance[i] >> 1) | 0b1;
            }
        }

        return imbalance;
    }

    unordered_set<partition_t*> involved_partitions(
        const struct client_message& request)
    {
        unordered_set<partition_t*> partitions;
        auto type = static_cast<request_type>(request.type);

        auto range = 1;
        if (type == SCAN) {
            range = stoi(request.args);
        }


        bool new_mapping = false;
        for (auto i = 0; i < range; i++) {

            if(!mapped(request.key + i)){
                map_key(request.key + i, round_robin_counter);
                new_mapping = true;
            } else {
                partitions.insert(data_to_partition->at(request.key + i));
            }
        }

        if(new_mapping){
            partitions.insert(partitions_.at(round_robin_counter));
            round_robin_counter = (round_robin_counter+1) % __n_partitions;
        }

        return partitions;
    }
    
    void dispatch(struct client_message& request){

        auto partitions = move(involved_partitions(request));
        bool is_cross_partition = partitions.size() > 1;
        if (is_cross_partition) {
            auto* barrier = new pthread_barrier_t();
            pthread_barrier_init(barrier, NULL, partitions.size());
            request.s_addr = (unsigned long) barrier;
            for (auto partition : partitions) {
                partition->push_request(request);
            }
        } else {
            auto partition = *begin(partitions);
            partition->push_request(request);
        }
        cross_partition_count += is_cross_partition;
    }
    void schedule_and_answer(struct client_message& request) {
        dispatch(request);
        __n_dispatched_requests++;

        if (repartition_method != model::ROUND_ROBIN) {

            if(update.load(memory_order_acquire) == true){
                update_partition_scheme();

                if constexpr(utils::ENABLE_INFO){
                    __repartition_apply_timestamp.push_back(utils::now());
                }
                update.store(false, memory_order_relaxed);


                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    time_start = utils::now();
                }
                operation_start = __n_dispatched_requests;
                cross_operation_start = cross_partition_count;
                repartitioning.store(false, memory_order_release);
                
            }
        }
    }


    void scheduling_loop() {
        while(true){
            scheduling_queue.template wait<0>();
            client_message message = scheduling_queue.template pop<0>();
            if (message.type == END){
                break;
            }
            schedule_and_answer(message);
        }
        
        __schedule_end = utils::now();
    }



    struct client_message create_sync_request(int n_partitions) {
        struct client_message sync_message;
        sync_message.id = sync_counter;
        sync_message.type = SYNC;

        auto* barrier = new pthread_barrier_t();
        pthread_barrier_init(barrier, NULL, n_partitions);
        sync_message.s_addr = (unsigned long) barrier;

        return sync_message;
    }

    void sync_partitions(const unordered_set<partition_t*>& partitions) {
        auto sync_message = move(
            create_sync_request(partitions.size())
        );
        for (auto partition : partitions) {
            partition->push_request(sync_message);
        }
    }

    void sync_repartition(partition_map_t * old_partition_map) {
        auto sync_message = move(
            create_sync_request(partitions_.size())
        );
        sync_message.type = REPARTITION;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            partition_t::add_old_partition_map(old_partition_map);
            partition->push_request(sync_message);
        }
    }

    void map_key(T key) {
        auto partition_id = round_robin_counter;
        data_to_partition->emplace(key, partitions_.at(partition_id));

        round_robin_counter = (round_robin_counter+1) % __n_partitions;
    }

    void map_key(T key, int partition_id) {
        data_to_partition->emplace(key, partitions_.at(partition_id));
    }

    bool mapped(T key) const {
        return data_to_partition->find(key) != data_to_partition->end();
    }


    void submit(client_message &message){
        scheduling_queue.push(message);
        scheduling_queue.template notify<0>();
        scheduling_queue.template notify<1>();
    }

    void update_partition_scheme(){
        partition_map_t *old_data_to_partition =  data_to_partition;
        data_to_partition = updated_data_to_partition;

        sync_repartition(old_data_to_partition);
    }

    void order_partitioning(){
        time_point begin;
        if constexpr(utils::ENABLE_INFO){
            begin = utils::now();
        }
        input_graph = InputGraph<T>(workload_graph);

        if constexpr(utils::ENABLE_INFO){
            __graph_copy_duration.push_back(utils::now() - begin);
        }

        if constexpr(utils::ENABLE_INFO){
            __repartition_request_timestamp.push_back(utils::now());
        }
        sem_post(&repart_semaphore);
    }

    void update_graph_loop() {
        while(true) {
            n_processed_requests++;
            scheduling_queue.template wait<1>();
            client_message request = scheduling_queue.template pop<1>();

            update_graph(request);

            if constexpr(TL > 0){
                graph_deletion_queue.push_back(request);

                auto expired_request = move(graph_deletion_queue.front());
                graph_deletion_queue.pop_front();
                expire(expired_request);
            }

            if(!repartitioning.load(memory_order_acquire)){
                bool interval_achieved;
                time_point now_ = utils::now();
                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    interval_achieved = utils::to_us(now_ - time_start) >= time_interval;
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    interval_achieved = n_processed_requests - operation_start >= operation_interval;
                }
                if (interval_achieved) {
                    bool repartition = imbalance();

                    if (repartition) {
                        repartitioning.store(true, memory_order_relaxed);
                        if(workload_graph.n_vertex() > 0){
                            order_partitioning();
                            clear_imbalance_count();
                        }
                    }

                    if constexpr(IntervalType == interval_type::MICROSECONDS){
                        time_start = utils::now();
                    } else if constexpr(IntervalType == interval_type::OPERATIONS){
                        operation_start = n_processed_requests;
                    }
                }
            }

        }
    }


    void partitioning_loop(){
        while(true){
            sem_wait(&repart_semaphore);

            auto temp = partitioning(input_graph);

            updated_data_to_partition = temp;
            update.store(true, memory_order_release);
        }
    }

    void update_graph(const client_message& message) {
        size_t data_size = 1;
        if (message.type == SCAN) {
            data_size = stoi(message.args);
        }

        for (auto i = 0; i < data_size; i++) {
            workload_graph.add_vertice(message.key+i);
            workload_graph.increment_vertice_weight(message.key+i, 1);

            for (auto j = i+1; j < data_size; j++) {
                workload_graph.add_vertice(message.key+j);
                workload_graph.add_edge(message.key+i, message.key+j);
                workload_graph.increment_edge_weight(message.key+i, message.key+j, 1);
            }
        }
    }

    void expire(const client_message& message) {
        if(message.type != DUMMY){
            int data_size = 1;
            if (message.type == SCAN) {
                data_size = stoi(message.args);
            }

            for (int i = data_size-1; i >= 0; i--) {
                for (int j = data_size-1; j >= i+1; j--) {
                    workload_graph.increment_edge_weight(message.key+i, message.key+j, -1);
                    workload_graph.remove_weightless_edge(message.key+i, message.key+j);
                    workload_graph.remove_weightless_vertice(message.key+j);
                }
                workload_graph.increment_vertice_weight(message.key+i, -1);
                workload_graph.remove_weightless_vertice(message.key+i);
            }
        }
    }


    partition_map_t* partitioning(InputGraph<T> &graph) {

        if constexpr(utils::ENABLE_INFO){
            __repartition_timestamps.push_back(utils::now());
        }

        auto partition_scheme = move(
            model::multilevel_cut(
                graph.vertice_weight, 
                graph.x_edges, 
                graph.edges, 
                graph.edges_weight,
                partitions_.size(), 
                repartition_method
            )
        );

        time_point reconstruction_begin;
        if constexpr(utils::ENABLE_INFO){
            __repartition_end_timestamps.push_back(utils::now());
            reconstruction_begin = utils::now();
        }
        auto data_to_partition = new partition_map_t();

        for (auto& it : graph.vertice_to_pos) {
            T key = it.first;
            int position = it.second;
            //position indicates the position of the key in partition scheme
            int partition = partition_scheme[position];  
            if (partition >= __n_partitions) {
                printf("ERROR: partition was %d!\n", partition);
                fflush(stdout);
            }
            data_to_partition->emplace(key, partitions_.at(partition));
        }

        if constexpr(utils::ENABLE_INFO){
            __reconstruction_duration.push_back(utils::now() - reconstruction_begin);
        }
        return data_to_partition;
    }

    size_t n_executed_requests() const{
        size_t n_executed_requests = 0;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            n_executed_requests += partition->n_executed_requests();
        }
        return n_executed_requests;
    }

    size_t n_enqueued_requests() const{
        size_t n_enqueued_requests = 0;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            n_enqueued_requests += partition->request_queue_size();
        }
        return n_enqueued_requests;
    }

    vector<size_t> in_queue_amount() const{
        vector<size_t> in_queue;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            size_t amount = partition->request_queue_size();
            in_queue.push_back(amount);
        }
        return in_queue;
    }

    size_t graph_vertices(){
        return workload_graph.n_vertex();
    }

    size_t graph_edges(){
        return workload_graph.n_edges();
    }

    time_point schedule_end(){
        return __schedule_end;
    }

    int n_dispatched_requests(){
        return __n_dispatched_requests;
    }

    int error_count(){
        int count = 0;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            count += partition->error_count();
        }
        return count;
    }

    const vector<time_point>& repartition_timestamps() const {
        return __repartition_timestamps;
    }

    const vector<duration>& graph_copy_duration() const {
        return __graph_copy_duration;
    }

    const vector<time_point>& repartition_end_timestamps() const {
        return __repartition_end_timestamps;
    }

    const vector<time_point>& repartition_apply_timestamp() const {
        return __repartition_apply_timestamp;
    }

    const vector<time_point>& repartition_request_timestamp() const {
        return __repartition_request_timestamp;
    }

    const vector<duration>& reconstruction_duration() const {
        return __reconstruction_duration;
    }

public:

    int __n_partitions;
    int round_robin_counter = 0;
    int sync_counter = 0;
    int __n_dispatched_requests = 0;

    unordered_map<int, partition_t*> partitions_;
    partition_map_t* data_to_partition;

    thread graph_thread;
    cpu_set_t graph_cpu_set;

    thread scheduling_thread;
    cpu_set_t scheduler_cpu_set;

    deque<struct client_message> graph_deletion_queue;

    model::Graph<T> workload_graph;
    model::CutMethod repartition_method;
    pthread_barrier_t repartition_barrier;

    int operation_interval;
    duration time_interval;
    time_point time_start;


    vector<time_point> __repartition_timestamps;
    vector<duration> __graph_copy_duration;
    vector<time_point> __repartition_end_timestamps;
    vector<time_point> __repartition_request_timestamp;
    vector<time_point> __repartition_apply_timestamp;
    vector<duration> __reconstruction_duration;
    time_point __schedule_end;

    model::Queue<client_message> scheduling_queue;

    size_t n_processed_requests = 0;

    size_t cross_partition_count = 0;

    partition_map_t* updated_data_to_partition;
    InputGraph<T> input_graph;

    sem_t repart_semaphore;

    thread reparting_thread;
    cpu_set_t reparting_cpu_set;


    atomic_bool repartitioning;
    atomic_bool update;

    int cross_operation_start = 0;
    size_t sucessive_cross_partition_intensive = 0;

    atomic_bool repartition;
    int operation_start = 0;

    float balance_threshold;
    size_t * __in_queue_amount;

    uint32_t* sucessive_imbalance;
    

};

};


#endif
