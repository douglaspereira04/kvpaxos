#ifndef _KVPAXOS_SCHEDULER_H_
#define _KVPAXOS_SCHEDULER_H_

#include <memory>
#include <pthread.h>
#include <deque>
#include <semaphore.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <iostream>

#include "queue.hpp"
#include "types.h"
#include "utils.h"
#include "request.hpp"
#include "input_graph.hpp"
#include "graph.hpp"
#include "partitioning.h"
#include "partition.hpp"


namespace kvpaxos {

using namespace kvstorage;
using namespace workload;


template <typename T, bool Rebalance, size_t TL = 0, size_t QSize = 0, interval_type IntervalType = interval_type::OPERATIONS, size_t MaxSucessiveImbalances = 100>
class Scheduler{

typedef kvpaxos::Partition<T, QSize> partition_t;
typedef std::unordered_map<T, partition_t*> partition_map_t;
public:

    Scheduler() {}
    Scheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method,
                size_t dh,
                float balance_threshold
    ) {
        __n_partitions = n_partitions;
        if (dh == 0) {
            scheduling_queue = model::Queue<Request*>(SEM_VALUE_MAX, 0);
        } else {
            scheduling_queue = model::Queue<Request*>(1, dh);
        }

        round_robin_counter = 0;
        sync_counter = 0;
        __n_dispatched_requests = 0;

        partition_t::create_storage(__n_partitions);
        for (auto i = 0; i < __n_partitions; i++) {
            auto* partition = new partition_t(i);
            __partitions.emplace(i, partition);
        }
        data_to_partition = new partition_map_t();

        scheduling_thread = std::thread(&Scheduler<T, Rebalance, TL, QSize, IntervalType, MaxSucessiveImbalances>::scheduling_loop, this);
        utils::set_affinity(2,scheduling_thread, scheduler_cpu_set);

        if constexpr(Rebalance) {
            workload_graph = model::Graph<T>();
            __in_queue_amount = new size_t[__n_partitions];
            sucessive_imbalance = new uint32_t[__n_partitions];
            for (auto i = 0; i < __n_partitions; i++) {
                sucessive_imbalance[i] = 0b1;
            }

            if constexpr(IntervalType == interval_type::MICROSECONDS){
                time_start = utils::now();
                time_interval = std::chrono::microseconds(repartition_interval);
                operation_start = 0;
                cross_operation_start = 0;
            } else if constexpr(IntervalType == interval_type::OPERATIONS){
                operation_start = 0;
                operation_interval = repartition_interval;
            }
            cross_partition_count = 0;
            repartition_method = repartition_method;

            updated_data_to_partition = new partition_map_t();

            set_balance_threshold(balance_threshold);
            clear_imbalance_count();

            repartitioning.store(false, std::memory_order_seq_cst);
            update.store(false, std::memory_order_seq_cst);
            repartition.store(false, std::memory_order_seq_cst);

            if constexpr(TL > 0){
                for (size_t i = 0; i < TL; i++)
                {
                    Request *dummy = new Request(DUMMY);
                    graph_deletion_queue.push_back(dummy);
                }
            }

            sem_init(&repart_semaphore, 0, 0);
            reparting_thread = std::thread(&Scheduler<T, Rebalance, TL, QSize, IntervalType, MaxSucessiveImbalances>::partitioning_loop, this);
            utils::set_affinity(4, reparting_thread, reparting_cpu_set);

            graph_thread = std::thread(&Scheduler<T, Rebalance, TL, QSize, IntervalType, MaxSucessiveImbalances>::update_graph_loop, this);
            utils::set_affinity(3, graph_thread, graph_cpu_set);
        }

    }

    ~Scheduler(){
        scheduling_thread.join();
        if constexpr(Rebalance) {
            graph_thread.join();
            reparting_thread.join();
            delete __in_queue_amount;
            delete sucessive_imbalance;
            delete data_to_partition;
            delete updated_data_to_partition;
        }

        for (auto& kv: __partitions) {
            auto* partition = kv.second;
            delete partition;
        }
    }

    void run() {
        for (auto& kv : __partitions) {
            kv.second->start_worker_thread();
        }
    }

    void join(){
        scheduling_thread.join();
    }

    void set_balance_threshold(float balance_threshold){
        __balance_threshold = balance_threshold;
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
            if (cross_partition_ratio > __balance_threshold){
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
        for (auto& kv: __partitions) {
            auto* partition = kv.second;
            size_t si = partition->request_queue_size();
            __in_queue_amount[i] = si;
            sum += si;
            i++;
        }

        float avg = static_cast<float>(sum)/__n_partitions;
        float threshold = avg * __balance_threshold;
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

    std::unordered_set<partition_t*> involved_partitions(
        Request* request)
    {
        std::unordered_set<partition_t*> partitions;
        auto type = request->type();
        size_t range = 1;
        bool new_mapping = false;
        int key;
        partition_t* new_assignment = nullptr;

        if (type == SCAN) {
            range = request->args_len();
            if (range == 1){
                key = request->key();
                if(!mapped(key)){
                    map_key(key, round_robin_counter);
                    new_assignment = __partitions.at(round_robin_counter);
                } else {
                    partitions.insert(data_to_partition->at(key));
                }
                request->set_single_partition();
            } else{
                request->init_scan_data();
                for (size_t i = 0; i < range; i++) {
                    key = request->key() + i;
                    if(!mapped(key)){
                        map_key(key, round_robin_counter);
                        new_assignment = __partitions.at(round_robin_counter);
                        request->set_key_to_partition(i, new_assignment);
                    } else {
                        partition_t* partition = data_to_partition->at(key);
                        partitions.insert(partition);
                        request->set_key_to_partition(i, partition);
                    }
                }
                request->init_coordination(partitions.size());
            }
        } else {
            key = request->key();
            if(!mapped(key)){
                map_key(key, round_robin_counter);
                new_assignment = __partitions.at(round_robin_counter);
            } else {
                partitions.insert(data_to_partition->at(key));
            }
        }

        if(new_assignment != nullptr){
            partitions.insert(new_assignment);
            round_robin_counter = (round_robin_counter+1) % __n_partitions;
        }
        return partitions;
    }
    
    void dispatch(Request* request){
        std::unordered_set<partition_t*> partitions = involved_partitions(request);
        bool is_cross_partition = partitions.size() > 1;
        if (is_cross_partition) {
            for (auto partition : partitions) {
                partition->push_request(request);
            }
        } else {
            auto partition = *begin(partitions);
            partition->push_request(request);
        }
        cross_partition_count += is_cross_partition;
    }
    void schedule_and_answer(Request* request) {
        dispatch(request);
        __n_dispatched_requests++;

        if constexpr(Rebalance) {

            if(update.load(std::memory_order_acquire) == true){
                update_partition_scheme();

                if constexpr(utils::ENABLE_INFO){
                    __repartition_apply_timestamp.push_back(utils::now());
                }
                update.store(false, std::memory_order_relaxed);


                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    time_start = utils::now();
                }
                operation_start = __n_dispatched_requests;
                cross_operation_start = cross_partition_count;
                repartitioning.store(false, std::memory_order_release);
                
            }
        }
    }

    void end_signal(Request* request){
        for (auto& kv: __partitions) {
            auto* partition = kv.second;
            partition->push_request(request->no_value_copy());
        }
        delete request;
    }


    void scheduling_loop() {
        while(true){
            scheduling_queue.template wait<0>();
            Request *request = scheduling_queue.template pop<0>();
            if (request->type() == END){
                end_signal(request);
                break;
            }
            schedule_and_answer(request);
        }
        
        __schedule_end = utils::now();
    }


    void sync_repartition(partition_map_t * old_partition_map) {
        Request *sync_request = new Request(REPARTITION);
        sync_request->init_barrier(__n_partitions);
        partition_t::add_old_partition_map(old_partition_map);

        for (auto& kv: __partitions) {
            auto* partition = kv.second;
            partition->push_request(sync_request);
        }
    }

    void map_key(T key) {
        auto partition_id = round_robin_counter;
        data_to_partition->emplace(key, __partitions.at(partition_id));

        round_robin_counter = (round_robin_counter+1) % __n_partitions;
    }

    void map_key(T key, int partition_id) {
        data_to_partition->emplace(key, __partitions.at(partition_id));
    }

    bool mapped(T key) const {
        return data_to_partition->find(key) != data_to_partition->end();
    }

    int submited = 0;
    void submit(Request* request){
        submited++;
        Request* request_copy = request->no_value_copy();
        scheduling_queue.push(request, request_copy);
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
            __n_processed_requests++;
            scheduling_queue.template wait<1>();
            Request *request = scheduling_queue.template pop<1>();
            if (request->type() == END){
                stop.store(true, std::memory_order_relaxed);
                sem_post(&repart_semaphore);
                delete request;
                break;
            }
            update_graph(request);

            if constexpr(TL > 0){
                graph_deletion_queue.push_back(request);

                Request *expired_request = graph_deletion_queue.front();
                expire(expired_request);
                delete expired_request;
                graph_deletion_queue.pop_front();
            }

            if(!repartitioning.load(std::memory_order_acquire)){
                bool interval_achieved;
                time_point now_ = utils::now();
                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    interval_achieved = utils::to_us(now_ - time_start) >= time_interval;
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    interval_achieved = __n_processed_requests - operation_start >= operation_interval;
                }
                if (interval_achieved) {
                    bool repartition = imbalance();

                    if (repartition) {
                        repartitioning.store(true, std::memory_order_relaxed);
                        if(workload_graph.n_vertex() > 0){
                            order_partitioning();
                            clear_imbalance_count();
                        }
                    }

                    if constexpr(IntervalType == interval_type::MICROSECONDS){
                        time_start = utils::now();
                    } else if constexpr(IntervalType == interval_type::OPERATIONS){
                        operation_start = __n_processed_requests;
                    }
                }
            }

        }
    }


    void partitioning_loop(){
        while(true){
            sem_wait(&repart_semaphore);
            if (stop.load(std::memory_order_relaxed)){
                break;
            }

            updated_data_to_partition = partitioning(input_graph);
            update.store(true, std::memory_order_release);
        }
    }

    void update_graph(Request* request) {
        size_t data_size = 1;
        if (request->type() == SCAN) {
            data_size = request->args_len();
        }

        for (auto i = 0; i < data_size; i++) {
            workload_graph.add_vertice(request->key()+i);
            workload_graph.increment_vertice_weight(request->key()+i, 1);

            for (auto j = i+1; j < data_size; j++) {
                workload_graph.add_vertice(request->key()+j);
                workload_graph.add_edge(request->key()+i, request->key()+j);
                workload_graph.increment_edge_weight(request->key()+i, request->key()+j, 1);
            }
        }
    }

    void expire(Request* request) {
        if(request->type() != DUMMY){
            int data_size = 1;
            if (request->type() == SCAN) {
                data_size = request->args_len();
            }

            for (int i = data_size-1; i >= 0; i--) {
                for (int j = data_size-1; j >= i+1; j--) {
                    workload_graph.increment_edge_weight(request->key()+i, request->key()+j, -1);
                    workload_graph.remove_weightless_edge(request->key()+i, request->key()+j);
                    workload_graph.remove_weightless_vertice(request->key()+j);
                }
                workload_graph.increment_vertice_weight(request->key()+i, -1);
                workload_graph.remove_weightless_vertice(request->key()+i);
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
                __partitions.size(), 
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
            int partition = partition_scheme[position];  
            if (partition >= __n_partitions) {
                printf("ERROR: partition was %d!\n", partition);
                fflush(stdout);
            }
            data_to_partition->emplace(key, __partitions.at(partition));
        }

        if constexpr(utils::ENABLE_INFO){
            __reconstruction_duration.push_back(utils::now() - reconstruction_begin);
        }
        return data_to_partition;
    }

    size_t n_executed_requests() const{
        size_t n_executed_requests = 0;
        for (auto& kv: __partitions) {
            auto* partition = kv.second;
            n_executed_requests += partition->n_executed_requests();
        }
        return n_executed_requests;
    }

    size_t n_processed_requests() const{
        return __n_processed_requests;
    }

    size_t n_enqueued_requests() const{
        size_t n_enqueued_requests = 0;
        for (auto& kv: __partitions) {
            auto* partition = kv.second;
            n_enqueued_requests += partition->request_queue_size();
        }
        return n_enqueued_requests;
    }

    std::vector<size_t> in_queue_amount() const{
        std::vector<size_t> in_queue;
        for (auto& kv: __partitions) {
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
        for (auto& kv: __partitions) {
            auto* partition = kv.second;
            count += partition->error_count();
        }
        return count;
    }

    const std::vector<time_point>& repartition_timestamps() const {
        return __repartition_timestamps;
    }

    const std::vector<duration>& graph_copy_duration() const {
        return __graph_copy_duration;
    }

    const std::vector<time_point>& repartition_end_timestamps() const {
        return __repartition_end_timestamps;
    }

    const std::vector<time_point>& repartition_apply_timestamp() const {
        return __repartition_apply_timestamp;
    }

    const std::vector<time_point>& repartition_request_timestamp() const {
        return __repartition_request_timestamp;
    }

    const std::vector<duration>& reconstruction_duration() const {
        return __reconstruction_duration;
    }

public:

    int __n_partitions;
    int round_robin_counter = 0;
    int sync_counter = 0;
    int __n_dispatched_requests = 0;

    partition_map_t __partitions;
    partition_map_t* data_to_partition;

    std::thread graph_thread;
    cpu_set_t graph_cpu_set;

    std::thread scheduling_thread;
    cpu_set_t scheduler_cpu_set;

    std::deque<Request*> graph_deletion_queue;

    model::Graph<T> workload_graph;
    model::CutMethod repartition_method;
    pthread_barrier_t repartition_barrier;

    int operation_interval;
    duration time_interval;
    time_point time_start;


    std::vector<time_point> __repartition_timestamps;
    std::vector<duration> __graph_copy_duration;
    std::vector<time_point> __repartition_end_timestamps;
    std::vector<time_point> __repartition_request_timestamp;
    std::vector<time_point> __repartition_apply_timestamp;
    std::vector<duration> __reconstruction_duration;
    time_point __schedule_end;

    model::Queue<Request*> scheduling_queue;

    size_t __n_processed_requests = 0;

    size_t cross_partition_count = 0;

    partition_map_t* updated_data_to_partition;
    InputGraph<T> input_graph;

    sem_t repart_semaphore;

    std::thread reparting_thread;
    cpu_set_t reparting_cpu_set;


    std::atomic_bool repartitioning;
    std::atomic_bool update;
    std::atomic_bool stop = false;

    int cross_operation_start = 0;
    size_t sucessive_cross_partition_intensive = 0;

    std::atomic_bool repartition;
    int operation_start = 0;

    float __balance_threshold;
    size_t * __in_queue_amount;

    uint32_t* sucessive_imbalance;
    

};

};


#endif
