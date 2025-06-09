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


template <typename T, bool Rebalance, size_t TL = 0, size_t QSize = 0, interval_type IntervalType = interval_type::OPERATIONS>
class Scheduler{

typedef kvpaxos::Partition<T, QSize> partition_t;
typedef std::unordered_map<T, std::pair<partition_t*, storage_t*>> partition_map_t;
public:

    Scheduler() {}
    Scheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method,
                size_t dh
    ) {
        __n_partitions = n_partitions;

        __version_count = 0;
        __storage = new storage_t[__n_partitions];
        for (size_t i = 0; i < __n_partitions; i++)
        {
            __storage[i] = storage_t(__version_count);
        }

        if constexpr(Rebalance) {
            if (dh == 0) {
                scheduling_queue = model::Queue<Request*>(SEM_VALUE_MAX, 0);
            } else {
                scheduling_queue = model::Queue<Request*>(1, dh);
            }
        } else {
            scheduling_queue = model::Queue<Request*>(SEM_VALUE_MAX, 0);
        }

        round_robin_counter = 0;
        __n_dispatched_requests = 0;

        __partitions = new partition_t*[__n_partitions];
        for (auto i = 0; i < __n_partitions; i++) {
            __partitions[i] = new partition_t(i, &__storage[i]);
        }
        __data_to_partition = new partition_map_t();

        scheduling_thread = std::thread(&Scheduler<T, Rebalance, TL, QSize, IntervalType>::scheduling_loop, this);
        utils::set_affinity(2,scheduling_thread, scheduler_cpu_set);

        if constexpr(Rebalance) {
            workload_graph = model::Graph<T>();

            if constexpr(IntervalType == interval_type::MICROSECONDS){
                __time_start = utils::now();
                time_interval = std::chrono::microseconds(repartition_interval);
                __operation_start = 0;
            } else if constexpr(IntervalType == interval_type::OPERATIONS){
                __operation_start = 0;
                operation_interval = repartition_interval;
            }
            repartition_method = repartition_method;

            updated_data_to_partition = new partition_map_t();


            __repartition_signal.store(false, std::memory_order_seq_cst);
            __update.store(false, std::memory_order_seq_cst);
            __repartitioning = false;

            if constexpr(TL > 0){
                for (size_t i = 0; i < TL; i++)
                {
                    Request *dummy = new Request(DUMMY);
                    graph_deletion_queue.push_back(dummy);
                }
            }

            sem_init(&repart_semaphore, 0, 0);
            reparting_thread = std::thread(&Scheduler<T, Rebalance, TL, QSize, IntervalType>::partitioning_loop, this);
            utils::set_affinity(4, reparting_thread, reparting_cpu_set);

            graph_thread = std::thread(&Scheduler<T, Rebalance, TL, QSize, IntervalType>::update_graph_loop, this);
            utils::set_affinity(3, graph_thread, graph_cpu_set);
        }

    }

    ~Scheduler(){
        scheduling_thread.join();
        if constexpr(Rebalance) {
            graph_thread.join();
            reparting_thread.join();
            delete __data_to_partition;
            delete updated_data_to_partition;
        }

        for (size_t i = 0; i < __n_partitions; i++)
        {
            delete __partitions[i];
        }
        delete __partitions;
    }

    void run() {

        for (size_t i = 0; i < __n_partitions; i++)
        {
            __partitions[i]->start_worker_thread();
        }
    }

    void join(){
        scheduling_thread.join();
    }
    

    std::unordered_set<partition_t*> prepare_request(
        Request* request)
    {
        std::unordered_set<partition_t*> partitions;
        auto type = request->type();
        size_t range = 1;
        bool new_mapping = false;
        bool is_multi_partition_scan = false;
        int key;
        partition_t* new_assignment = nullptr;
        partition_t* next_partition = __partitions[round_robin_counter];
        storage_t* next_storage = &__storage[round_robin_counter]; 
        std::pair<partition_t*, storage_t*> next_mapping = std::make_pair(next_partition, next_storage);
        if (type == SCAN) {
            range = request->args_len();
            if (range == 1){
                key = request->key();
                auto [it, inserted] = __data_to_partition->try_emplace(key, next_mapping);
                std::pair<partition_t*, storage_t*> mapping = it->second;
                if (inserted){
                    new_assignment = next_partition;
                    request->set_storage(next_storage);
                } else {
                    partitions.insert(mapping.first);
                    request->set_storage(mapping.second);
                    it->second = next_mapping;
                }
                request->set_single_partition();
            } else{
                is_multi_partition_scan = true;
                request->init_scan_data();
                for (size_t i = 0; i < range; i++) {
                    key = request->key() + i;
                    auto [it, inserted] = __data_to_partition->try_emplace(key, next_mapping);
                    std::pair<partition_t*, storage_t*> mapping = it->second;

                    if (inserted){
                        new_assignment = next_partition;
                        request->set_key_to_partition(i, new_assignment);
                        request->set_storage(i, next_storage);
                    } else {
                        partitions.insert(mapping.first);
                        request->set_key_to_partition(i, mapping.first);
                        request->set_storage(i, mapping.second);
                        it->second = next_mapping;
                    }
                }
            }
        } else {
            key = request->key();

            auto [it, inserted] = __data_to_partition->try_emplace(key, next_mapping);
            std::pair<partition_t*, storage_t*> mapping = it->second;
            if (inserted){
                new_assignment = next_partition;
                request->set_storage(next_storage);
            } else {
                partitions.insert(mapping.first);
                request->set_storage(mapping.second);
                it->second = next_mapping;
            }
        }

        if(new_assignment != nullptr){
            partitions.insert(new_assignment);
            round_robin_counter = (round_robin_counter+1) % __n_partitions;
        }
        if (is_multi_partition_scan){
            request->init_coordination(partitions.size());
        }
        return partitions;
    }
    
    void dispatch(Request* request){
        std::unordered_set<partition_t*> partitions = prepare_request(request);
        for (auto partition : partitions) {
            partition->push_request(request);
        }
    }

    inline bool interval_achieved(){
        bool interval_achieved;
        time_point now_ = utils::now();
        if constexpr(IntervalType == interval_type::MICROSECONDS)
            interval_achieved = utils::to_us(now_ - __time_start) >= time_interval;
        else if constexpr(IntervalType == interval_type::OPERATIONS)
            interval_achieved = __n_dispatched_requests - __operation_start >= operation_interval;
        return interval_achieved;
    }

    partition_map_t* rebuild_map(std::vector<int> &partition_scheme){
        time_point reconstruction_begin;
        if constexpr(utils::ENABLE_INFO){
            __repartition_end_timestamps.push_back(utils::now());
            reconstruction_begin = utils::now();
        }
        partition_map_t* new_data_to_partition = new partition_map_t(*__data_to_partition);
        for (auto& it : input_graph.vertice_to_pos) {
            T key = it.first;
            int position = it.second;
            int partition_idx = partition_scheme[position];  
            if (partition_idx >= __n_partitions) {
                printf("ERROR: partition was %d!\n", partition_idx);
                fflush(stdout);
            }
            partition_t* partition = __partitions[partition_idx];
            storage_t* storage = (*__data_to_partition)[key].second;
            (*new_data_to_partition)[key] = std::make_pair(partition, storage);
        }

        if constexpr(utils::ENABLE_INFO){
            __reconstruction_duration.push_back(utils::now() - reconstruction_begin);
        }
        return new_data_to_partition;
    }

    void schedule_and_answer(Request* request) {
        dispatch(request);
        __n_dispatched_requests++;

        if constexpr(Rebalance) {
            if (!__repartitioning){
                if (interval_achieved()) {
                    __repartition_signal.store(true, std::memory_order_release);
                    __repartitioning = true;
                }
            } else if(__update.load(std::memory_order_acquire) == true){
                update_partition_scheme();

                if constexpr(utils::ENABLE_INFO){
                    __repartition_apply_timestamp.push_back(utils::now());
                }
                __update.store(false, std::memory_order_relaxed);

                if constexpr(IntervalType == interval_type::MICROSECONDS)
                    __time_start = utils::now();
                else if constexpr(IntervalType == interval_type::OPERATIONS)
                    __operation_start = __n_dispatched_requests;

                __repartitioning = false;
            }
        }
    }

    void end_signal(Request* request){
        for (size_t i = 0; i < __n_partitions; i++)
        {
            Request* end_request = new Request(END);
            __partitions[i]->push_request(end_request);
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


    void sync_repartition() {
        Request *sync_request = new Request(REPARTITION);
        sync_request->init_barrier(__n_partitions);
        storage_t* new_storage = new storage_t[__n_partitions];

        __version_count++;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            new_storage[i] = storage_t(__version_count);
        }
        sync_request->set_new_storage(new_storage);

        for (size_t i = 0; i < __n_partitions; i++)
        {
            __partitions[i]->push_request(sync_request);
        }
        __storage = new_storage;
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
        partition_map_t* temp = __data_to_partition;
        __data_to_partition = rebuild_map(*__partition_scheme);
        delete __partition_scheme;
        delete temp;
        sync_repartition();
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

            if(__repartition_signal.load(std::memory_order_acquire)){
                __repartition_signal.store(false, std::memory_order_relaxed);
                if(workload_graph.n_vertex() > 0){
                    order_partitioning();
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
            if (__n_partitions > 1){
                __partition_scheme = partitioning();
            }
            __update.store(true, std::memory_order_release);
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


    std::vector<int>* partitioning() {

        if constexpr(utils::ENABLE_INFO){
            __repartition_timestamps.push_back(utils::now());
        }

        return model::multilevel_cut(
                input_graph.vertice_weight, 
                input_graph.x_edges, 
                input_graph.edges, 
                input_graph.edges_weight,
                __n_partitions, 
                repartition_method
        );
    }

    size_t n_executed_requests() const{
        size_t n_executed_requests = 0;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            n_executed_requests += __partitions[i]->n_executed_requests();
        }
        return n_executed_requests;
    }

    size_t n_processed_requests() const{
        return __n_processed_requests;
    }

    size_t n_enqueued_requests() const{
        size_t n_enqueued_requests = 0;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            n_enqueued_requests += __partitions[i]->request_queue_size();
        }
        return n_enqueued_requests;
    }

    std::vector<size_t> in_queue_amount() const{
        std::vector<size_t> in_queue;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            size_t amount = __partitions[i]->request_queue_size();
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
        for (size_t i = 0; i < __n_partitions; i++)
        {
            count += __partitions[i]->error_count();
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

    size_t __n_partitions;
    int round_robin_counter = 0;
    int __n_dispatched_requests = 0;

    partition_t** __partitions; //to be refactored to partition_t*
    partition_map_t* __data_to_partition;

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
    time_point __time_start;
    std::vector<int> *__partition_scheme;

    std::vector<time_point> __repartition_timestamps;
    std::vector<duration> __graph_copy_duration;
    std::vector<time_point> __repartition_end_timestamps;
    std::vector<time_point> __repartition_request_timestamp;
    std::vector<time_point> __repartition_apply_timestamp;
    std::vector<duration> __reconstruction_duration;
    time_point __schedule_end;

    model::Queue<Request*> scheduling_queue;

    size_t __n_processed_requests = 0;

    partition_map_t* updated_data_to_partition;
    InputGraph<T> input_graph;

    sem_t repart_semaphore;

    std::thread reparting_thread;
    cpu_set_t reparting_cpu_set;


    std::atomic_bool __repartition_signal;
    std::atomic_bool __update;
    std::atomic_bool stop = false;

    bool __repartitioning;


    int __operation_start = 0;
    
    storage_t* __storage;
    int __version_count;
};

};


#endif
