#ifndef _KVPAXOS_KVSTORE_H_
#define _KVPAXOS_KVSTORE_H_

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
#include "utils.h"
#include "input_graph.hpp"
#include "graph.hpp"
#include "partitioning.h"
#include "partition.hpp"

#include "operation.hpp"
#include "get_callback_operation.hpp"
#include "get_future_operation.hpp"
#include "set_callback_operation.hpp"
#include "set_operation.hpp"
#include "scan_callback_operation.hpp"
#include "scan_future_operation.hpp"
#include "del_callback_operation.hpp"
#include "del_operation.hpp"
#include "repartition_operation.hpp"
#include "tracking_info.hpp"

namespace kvpaxos {

using namespace kvstorage;


template <typename T, bool Rebalance, size_t TL = 0, size_t QSize = 0, types::interval_type IntervalType = types::OPERATIONS>
class KVStore{

typedef kvpaxos::Partition<T, QSize> partition_t;
typedef std::unordered_map<T, partition_t*> partition_map_t;
typedef model::Queue<Operation<T>*, TrackingInfo<T>*> scheduling_queue_t;
public:

    KVStore() {}
    KVStore(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method,
                size_t dh
    ) {
        __n_partitions = n_partitions;
        if constexpr(Rebalance) {
            if (dh == 0) {
                scheduling_queue = scheduling_queue_t(SEM_VALUE_MAX, 0);
            } else {
                scheduling_queue = scheduling_queue_t(1, dh);
            }
        } else {
            scheduling_queue = scheduling_queue_t(SEM_VALUE_MAX, 0);
        }

        round_robin_counter = 0;
        __n_dispatched_operations = 0;

        partition_t::create_storage(__n_partitions);
        __partitions = new partition_t*[__n_partitions];
        for (auto i = 0; i < __n_partitions; i++) {
            __partitions[i] = new partition_t(i);
        }
        __data_to_partition = new partition_map_t();

        scheduling_thread = std::thread(&KVStore<T, Rebalance, TL, QSize, IntervalType>::scheduling_loop, this);
        utils::set_affinity(2,scheduling_thread, scheduler_cpu_set);

        if constexpr(Rebalance) {
            workload_graph = model::Graph<T>();

            if constexpr(IntervalType == types::MICROSECONDS){
                __time_start = utils::now();
                time_interval = std::chrono::microseconds(repartition_interval);
                __operation_start = 0;
            } else if constexpr(IntervalType == types::OPERATIONS){
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
                    TrackingInfo<T> *dummy = new TrackingInfo<T>(DUMMY);
                    graph_deletion_queue.push_back(dummy);
                }
            }

            sem_init(&repart_semaphore, 0, 0);
            reparting_thread = std::thread(&KVStore<T, Rebalance, TL, QSize, IntervalType>::partitioning_loop, this);
            utils::set_affinity(4, reparting_thread, reparting_cpu_set);

            graph_thread = std::thread(&KVStore<T, Rebalance, TL, QSize, IntervalType>::update_graph_loop, this);
            utils::set_affinity(3, graph_thread, graph_cpu_set);
        }

    }

    ~KVStore(){
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
        delete[] __partitions;
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

    void stop(){
        submit<END>(new Operation<T>(END));
    }
    
    void get(T key, void (*cb)(T key, std::string*)){
        GetCallbackOperation<T>* operation = new GetCallbackOperation<T>(key, cb);
        submit<GET_CALLBACK>(operation);
    }
    
    void set(T key, const std::string &value, void (*cb)(T key, std::string*)){
        SetCallbackOperation<T>* operation = new SetCallbackOperation<T>(key, new std::string(value), cb);
        submit<SET_CALLBACK>(operation);
    }
    
    void scan(T key, size_t len, void (*cb)(T key, std::string*)){
        std::string *values = new std::string[len];
        ScanCallbackOperation<T>* operation = new ScanCallbackOperation<T>(key, len, values, cb);
        submit<SCAN_CALLBACK>(operation);
    }
    
    void del(T key, void (*cb)(T key)){
        DelCallbackOperation<T>* operation = new DelCallbackOperation<T>(key, cb);
        submit<DEL_CALLBACK>(operation);
    }
    
    std::string get(T key){
        std::string value;
        GetFutureOperation<T> operation(key, &value);
        submit<GET_FUTURE>(&operation);
        operation.wait();
        return value;
    }
    
    void set(T key, const std::string &value){
        SetOperation<T>* operation = new SetOperation<T>(key, new std::string(value));
        submit<SET>(operation);
    }
    
    std::vector<std::string> scan(T key, size_t len){
        std::vector<std::string> values(len);
        ScanFutureOperation<T> operation(key, len, values.data());
        submit<SCAN_FUTURE>(&operation);
        operation.wait();
        return values;
    }
    
    void del(T key){
        DelOperation<T>* operation = new DelOperation<T>(key);
        submit<DEL>(operation);
    }

    inline std::pair<typename partition_map_t::iterator, bool> try_map(T key){
        return __data_to_partition->try_emplace(key, __partitions[round_robin_counter]);
    }

    std::unordered_set<partition_t*> prepare_operation(
        Operation<T>* operation)
    {
        std::unordered_set<partition_t*> partitions;
        size_t range = 1;
        bool new_mapping = false;
        T key;
        partition_t* new_assignment;
        
        if (operation->is_scan()) {
            ScanOperation<T>* scan_op = static_cast<ScanOperation<T>*>(operation);
            range = scan_op->len();
            if (range == 1){
                auto [it, inserted] = try_map(scan_op->key());
                if (inserted){
                    new_assignment = __partitions[round_robin_counter];
                    partitions.insert(new_assignment);
                    round_robin_counter = (round_robin_counter+1) % __n_partitions;
                } else {
                    partitions.insert(it->second);
                }
                scan_op->set_is_single_partition();
            } else{
                scan_op->init_scan_data();
                new_assignment = nullptr;
                for (size_t i = 0; i < range; i++) {
                    key = scan_op->key() + i;
                    auto [it, inserted] = try_map(key);
                    if (inserted){
                        if (new_assignment == nullptr){
                            new_assignment = __partitions[round_robin_counter];
                        }
                        scan_op->set_key_to_partition(i, new_assignment);
                    } else {
                        partitions.insert(it->second);
                        scan_op->set_key_to_partition(i, it->second);
                    }
                }
                if(new_assignment != nullptr){
                    partitions.insert(new_assignment);
                    round_robin_counter = (round_robin_counter+1) % __n_partitions;
                }
                scan_op->init_coordination(partitions.size());
            }
        } else {
            auto [it, inserted] = try_map(operation->key());
            if (inserted){
                new_assignment = __partitions[round_robin_counter];
                partitions.insert(new_assignment);
                round_robin_counter = (round_robin_counter+1) % __n_partitions;
            } else {
                partitions.insert(it->second);
            }
        }

        return partitions;
    }
    
    void dispatch(Operation<T>* operation){
        std::unordered_set<partition_t*> partitions = prepare_operation(operation);
        for (auto partition : partitions) {
            partition->push_operation(operation);
        }
    }

    inline bool interval_achieved(){
        bool interval_achieved;
        types::time_point now_ = utils::now();
        if constexpr(IntervalType == types::MICROSECONDS)
            interval_achieved = utils::to_us(now_ - __time_start) >= time_interval;
        else if constexpr(IntervalType == types::OPERATIONS)
            interval_achieved = __n_dispatched_operations - __operation_start >= operation_interval;
        return interval_achieved;
    }

    void schedule_and_answer(Operation<T>* operation) {
        dispatch(operation);
        __n_dispatched_operations++;

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

                if constexpr(IntervalType == types::MICROSECONDS)
                    __time_start = utils::now();
                else if constexpr(IntervalType == types::OPERATIONS)
                    __operation_start = __n_dispatched_operations;

                __repartitioning = false;
            }
        }
    }

    void stop_signal(){
        for (size_t i = 0; i < __n_partitions; i++)
        {
            Operation<T>* end_operation = new Operation<T>(END);
            __partitions[i]->push_operation(end_operation);
        }
    }


    void scheduling_loop() {
        while(true){
            scheduling_queue.template wait<0>();
            Operation<T> *operation = scheduling_queue.template pop<Operation<T>*>();
            if (operation->type() == END){
                stop_signal();
                delete operation;
                break;
            }
            schedule_and_answer(operation);
        }
        
        __schedule_end = utils::now();
    }


    void sync_repartition(partition_map_t * old_partition_map) {
        RepartitionOperation<T> *sync_operation = new RepartitionOperation<T>(__n_partitions);
        partition_t::add_old_partition_map(old_partition_map);

        for (size_t i = 0; i < __n_partitions; i++)
        {
            __partitions[i]->push_operation(sync_operation);
        }
    }

    void map_key(T key) {
        auto partition_id = round_robin_counter;
        __data_to_partition->emplace(key, __partitions[partition_id]);

        round_robin_counter = (round_robin_counter+1) % __n_partitions;
    }

    void map_key(T key, int partition_id) {
        __data_to_partition->emplace(key, __partitions[partition_id]);
    }

    bool mapped(T key) const {
        return __data_to_partition->find(key) != __data_to_partition->end();
    }

    template<OperationType type>
    void submit(Operation<T>* operation){
        TrackingInfo<T>* tracking_info = TrackingInfo<T>::template get_tracking_info<type>(operation);
        scheduling_queue.template push(operation, tracking_info);
        scheduling_queue.template notify<0>();
        scheduling_queue.template notify<1>();
    }

    void update_partition_scheme(){
        partition_map_t *old_data_to_partition =  __data_to_partition;
        __data_to_partition = updated_data_to_partition;

        sync_repartition(old_data_to_partition);
    }

    void order_partitioning(){
        types::time_point begin;
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
            __n_processed_operations++;
            scheduling_queue.template wait<1>();
            TrackingInfo<T> *tracking_info = scheduling_queue.template pop<TrackingInfo<T>*>();
            if (tracking_info->type() == END){
                __stop.store(true, std::memory_order_relaxed);
                sem_post(&repart_semaphore);
                delete tracking_info;
                break;
            }
            update_graph(tracking_info);

            if constexpr(TL > 0){
                graph_deletion_queue.push_back(tracking_info);

                TrackingInfo<T> *expired_info = graph_deletion_queue.front();
                expire(expired_info);
                delete expired_info;
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
            if (__stop.load(std::memory_order_relaxed)){
                break;
            }
            if (__n_partitions > 1){
                updated_data_to_partition = partitioning(input_graph);
            } else {
                updated_data_to_partition = new partition_map_t(*__data_to_partition);
            }
            __update.store(true, std::memory_order_release);
        }
    }

    void update_graph(TrackingInfo<T>* tracking_info) {
        size_t data_size = 1;
        if (tracking_info->type() == SCAN) {
            data_size = tracking_info->len();
        }

        for (auto i = 0; i < data_size; i++) {
            workload_graph.add_vertice(tracking_info->key()+i);
            workload_graph.increment_vertice_weight(tracking_info->key()+i, 1);

            for (auto j = i+1; j < data_size; j++) {
                workload_graph.add_vertice(tracking_info->key()+j);
                workload_graph.add_edge(tracking_info->key()+i, tracking_info->key()+j);
                workload_graph.increment_edge_weight(tracking_info->key()+i, tracking_info->key()+j, 1);
            }
        }
    }

    void expire(TrackingInfo<T>* tracking_info) {
        if(tracking_info->type() != DUMMY){
            int data_size = 1;
            if (tracking_info->type() == SCAN) {
                data_size = tracking_info->len();
            }

            for (int i = data_size-1; i >= 0; i--) {
                for (int j = data_size-1; j >= i+1; j--) {
                    workload_graph.increment_edge_weight(tracking_info->key()+i, tracking_info->key()+j, -1);
                    workload_graph.remove_weightless_edge(tracking_info->key()+i, tracking_info->key()+j);
                    workload_graph.remove_weightless_vertice(tracking_info->key()+j);
                }
                workload_graph.increment_vertice_weight(tracking_info->key()+i, -1);
                workload_graph.remove_weightless_vertice(tracking_info->key()+i);
            }
        }
    }


    partition_map_t* partitioning(InputGraph<T> &graph) {
        
        if constexpr(utils::ENABLE_INFO){
            __repartition_timestamps.push_back(utils::now());
        }

        std::vector<int> partition_scheme = move(
            model::multilevel_cut(
                graph.vertice_weight, 
                graph.x_edges, 
                graph.edges, 
                graph.edges_weight,
                __n_partitions, 
                repartition_method
            )
        );

        types::time_point reconstruction_begin;
        if constexpr(utils::ENABLE_INFO){
            __repartition_end_timestamps.push_back(utils::now());
            reconstruction_begin = utils::now();
        }
        partition_map_t* data_to_partition = new partition_map_t();
        data_to_partition->reserve(graph.vertice_to_pos.size());

        for (auto& it : graph.vertice_to_pos) {
            T key = it.first;
            int position = it.second;
            int partition = partition_scheme[position];  
            if (partition >= __n_partitions) {
                printf("ERROR: partition was %d!\n", partition);
                fflush(stdout);
            }
            data_to_partition->emplace(key, __partitions[partition]);
        }

        if constexpr(utils::ENABLE_INFO){
            __reconstruction_duration.push_back(utils::now() - reconstruction_begin);
        }
        return data_to_partition;
    }

    size_t n_executed_operations() const{
        size_t n_executed_operations = 0;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            n_executed_operations += __partitions[i]->n_executed_operations();
        }
        return n_executed_operations;
    }

    size_t n_processed_operations() const{
        return __n_processed_operations;
    }

    size_t n_enqueued_operations() const{
        size_t n_enqueued_operations = 0;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            n_enqueued_operations += __partitions[i]->operation_queue_size();
        }
        return n_enqueued_operations;
    }

    std::vector<size_t> in_queue_amount() const{
        std::vector<size_t> in_queue;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            size_t amount = __partitions[i]->operation_queue_size();
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

    types::time_point schedule_end(){
        return __schedule_end;
    }

    int n_dispatched_operations(){
        return __n_dispatched_operations;
    }

    int error_count(){
        int count = 0;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            count += __partitions[i]->error_count();
        }
        return count;
    }

    const std::vector<types::time_point>& repartition_timestamps() const {
        return __repartition_timestamps;
    }

    const std::vector<types::duration>& graph_copy_duration() const {
        return __graph_copy_duration;
    }

    const std::vector<types::time_point>& repartition_end_timestamps() const {
        return __repartition_end_timestamps;
    }

    const std::vector<types::time_point>& repartition_apply_timestamp() const {
        return __repartition_apply_timestamp;
    }

    const std::vector<types::time_point>& repartition_request_timestamp() const {
        return __repartition_request_timestamp;
    }

    const std::vector<types::duration>& reconstruction_duration() const {
        return __reconstruction_duration;
    }

public:

    size_t __n_partitions;
    int round_robin_counter = 0;
    int __n_dispatched_operations = 0;

    partition_t** __partitions;
    partition_map_t* __data_to_partition;

    std::thread graph_thread;
    cpu_set_t graph_cpu_set;

    std::thread scheduling_thread;
    cpu_set_t scheduler_cpu_set;

    std::deque<TrackingInfo<T>*> graph_deletion_queue;

    model::Graph<T> workload_graph;
    model::CutMethod repartition_method;
    pthread_barrier_t repartition_barrier;

    int operation_interval;
    types::duration time_interval;
    types::time_point __time_start;


    std::vector<types::time_point> __repartition_timestamps;
    std::vector<types::duration> __graph_copy_duration;
    std::vector<types::time_point> __repartition_end_timestamps;
    std::vector<types::time_point> __repartition_request_timestamp;
    std::vector<types::time_point> __repartition_apply_timestamp;
    std::vector<types::duration> __reconstruction_duration;
    types::time_point __schedule_end;

    scheduling_queue_t scheduling_queue;

    size_t __n_processed_operations = 0;

    partition_map_t* updated_data_to_partition;
    InputGraph<T> input_graph;

    sem_t repart_semaphore;

    std::thread reparting_thread;
    cpu_set_t reparting_cpu_set;


    std::atomic_bool __repartition_signal;
    std::atomic_bool __update;
    std::atomic_bool __stop = false;

    bool __repartitioning;


    int __operation_start = 0;
    

};

};


#endif
