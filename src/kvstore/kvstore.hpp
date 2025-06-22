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
#include "edgeless_graph.hpp"
#include "partitioning.h"
#include "worker.hpp"

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
#include "ankerl/unordered_dense.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/btree_set.h"
#include "absl/synchronization/notification.h"

namespace kvpaxos {
enum PrepareStatus{
    OK = 0,
    NOT_FOUND = 1
};

template <typename T, typename storage_t, bool Rebalance, size_t QSize = 0, types::interval_type IntervalType = types::OPERATIONS>
class KVStore{
typedef KVStore<T, storage_t, Rebalance, QSize, IntervalType> kvstore_t;
typedef kvpaxos::Worker<T, storage_t, QSize> worker_t;
typedef ankerl::unordered_dense::map<T, worker_t*> worker_map_t;
typedef ankerl::unordered_dense::map<T, storage_t*> storage_map_t;
typedef model::Queue<Operation<T>*, TrackingInfo<T>*> schedule_queue_t;
typedef absl::flat_hash_set<worker_t*> worker_set_t;
typedef absl::btree_set<T> key_set_t;

public:

    KVStore() {}
    KVStore(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        __n_partitions = n_partitions;
        if constexpr(Rebalance) {
            __scheduling_queue = new schedule_queue_t(repartition_interval, repartition_interval);
        } else {
            __scheduling_queue = new schedule_queue_t(SEM_VALUE_MAX, SEM_VALUE_MAX);
        }

        __rr_counter = 0;
        __n_dispatched_operations = 0;
        __involved_workers =  worker_set_t();
                __involved_workers.reserve(__n_partitions);

        __level = 0;
        __storages = new storage_t[__n_partitions];
        __workers = new worker_t*[__n_partitions];
        for (auto i = 0; i < __n_partitions; i++) {
            __storages[i].level(0);
            __storages[i].init();
            __workers[i] = new worker_t(i, &__storages[i]);
        }
        __storage_map = storage_map_t();
        __worker_map = new worker_map_t();

        scheduling_thread = std::thread(&kvstore_t::scheduling_loop, this);
        utils::set_affinity(2,scheduling_thread, scheduler_cpu_set);

        if constexpr(Rebalance) {
            __graph = model::Graph<T>();
            __edgeless_graph = model::EdgelessGraph<T>();
            if constexpr(utils::ENABLE_EDGES){
                __input_graph = InputGraph<T>(&__graph);
            } else {
                __input_graph = InputGraph<T>(&__edgeless_graph);
            }
            if constexpr(IntervalType == types::MICROSECONDS){
                __time_start = utils::now();
                time_interval = std::chrono::microseconds(repartition_interval);
                __operation_start = 0;
            } else if constexpr(IntervalType == types::OPERATIONS){
                __operation_start = 0;
                operation_interval = repartition_interval;
            }
            __repartition_method = repartition_method;

            __updated_worker_map = new worker_map_t();


            __repartition_signal.store(false, std::memory_order_seq_cst);
            __update.store(false, std::memory_order_seq_cst);
            __repartitioning = false;

            __repart_notification = new absl::Notification();
            reparting_thread = std::thread(&kvstore_t::partitioning_loop, this);
            utils::set_affinity(4, reparting_thread, reparting_cpu_set);

            graph_thread = std::thread(&kvstore_t::update_graph_loop, this);
            utils::set_affinity(3, graph_thread, graph_cpu_set);
        }

    }

    ~KVStore(){
        scheduling_thread.join();
        if constexpr(Rebalance) {
            graph_thread.join();
            reparting_thread.join();
            delete __worker_map;
            delete __updated_worker_map;
        }

        for (size_t i = 0; i < __n_partitions; i++)
        {
            delete __workers[i];
        }
        delete[] __workers;
        delete __scheduling_queue;
    }

    void run() {
        for (size_t i = 0; i < __n_partitions; i++)
        {
            __workers[i]->start_worker_thread();
        }
    }

    void join(){
        scheduling_thread.join();
    }

    void stop(){
        submit<END>(new Operation<T>(END));
    }
    
    void get(T &key, void (*cb)(T &key, std::string*)){
        GetCallbackOperation<T>* operation = new GetCallbackOperation<T>(key, cb);
        submit<GET_CALLBACK>(operation);
    }
    
    void set(T &key, const std::string &value, void (*cb)(T &key, std::string*)){
        SetCallbackOperation<T>* operation = new SetCallbackOperation<T>(key, new std::string(value), cb);
        submit<SET_CALLBACK>(operation);
    }
    
    void scan(T &key, size_t len, void (*cb)(T &key, std::string*)){
        std::string *values = new std::string[len];
        ScanCallbackOperation<T>* operation = new ScanCallbackOperation<T>(key, len, values, cb);
        submit<SCAN_CALLBACK>(operation);
    }
    
    void del(T &key, void (*cb)(T &key)){
        DelCallbackOperation<T>* operation = new DelCallbackOperation<T>(key, cb);
        submit<DEL_CALLBACK>(operation);
    }
    
    std::string get(T &key){
        std::string value;
        GetFutureOperation<T> operation(key, &value);
        submit<GET_FUTURE>(&operation);
        operation.wait();
        return value;
    }
    
    void set(T &key, const std::string &value){
        SetOperation<T>* operation = new SetOperation<T>(key, new std::string(value));
        submit<SET>(operation);
    }
    
    std::vector<std::string> scan(T &key, size_t len){
        std::vector<std::string> values(len);
        ScanFutureOperation<T> operation(key, len, values.data());
        submit<SCAN_FUTURE>(&operation);
        operation.wait();
        return values;
    }
    
    void del(T &key){
        DelOperation<T>* operation = new DelOperation<T>(key);
        submit<DEL>(operation);
    }

    inline PrepareStatus prepare_single(Operation<T> *operation){
        T key = operation->key();

        if (operation->is_set()){
            __keys.insert(key);
        } else if (operation->is_del()){
            auto key_it = __keys.find(key);
            if (key_it == __keys.end()){
                return NOT_FOUND;
            } else {
                __keys.erase(key_it);
            }
        }

        auto [worker_it, worker_emplaced] = __worker_map->try_emplace(key, __workers[__rr_counter]);
        __involved_workers.insert(worker_it->second);
        storage_t* worker_storage = &__storages[worker_it->second->id()];
        auto [storage_it, storage_emplaced] = __storage_map.try_emplace(key, worker_storage);
        storage_t* storage = storage_it->second;
        operation->storage(storage);
        if (!storage_emplaced && worker_storage != storage){
            storage_it->second = worker_storage;
        }

        if (worker_emplaced){
            __rr_counter = (__rr_counter+1) % __n_partitions;
        }

        return PrepareStatus::OK;
    }

    inline PrepareStatus prepare_range(ScanOperation<T>* operation){
        T key = operation->key();
        size_t len = operation->len();

        operation->init_scan_data();

        auto key_it = __keys.lower_bound(key);
        size_t i = 0;
        for (; i < len && key_it != __keys.end(); i++) {
            T key_i = *key_it;

            auto [worker_it, worker_emplaced] = __worker_map->try_emplace(key_i, __workers[__rr_counter]);
            worker_t* worker = worker_it->second;
            __involved_workers.insert(worker);
            operation->worker(i, worker);
            storage_t* worker_storage = &__storages[worker->id()];
            auto [storage_it, storage_emplaced] = __storage_map.try_emplace(key_i, worker_storage);
            storage_t* storage = storage_it->second;
            operation->storage(i, storage);
            operation->key(i, key_i);
            if (!storage_emplaced && worker_storage != storage){
                storage_it->second = worker_storage;
            }

            if (worker_emplaced){
                __rr_counter = (__rr_counter+1) % __n_partitions;
            }
            key_it++;
        }

        if (i < len){
            return PrepareStatus::NOT_FOUND;
        }

        operation->init_coordination(__involved_workers.size());
        return PrepareStatus::OK;
    }

    inline PrepareStatus prepare_operation(
        Operation<T>* operation)
    {
        PrepareStatus status;
        if (operation->is_scan()) {
            ScanOperation<T>* scan_op = static_cast<ScanOperation<T>*>(operation);
            status = prepare_range(scan_op);
        } else {
            status = prepare_single(operation);
        }
        return status;
    }
    
    void dispatch(Operation<T>* operation){
        __involved_workers.clear();
        prepare_operation(operation);
        for (worker_t* worker : __involved_workers) {
            worker->push_operation(operation);
        }
    }

    inline bool interval_achieved(){
        bool interval_achieved;
        if constexpr(IntervalType == types::MICROSECONDS){
            types::time_point now_ = utils::now();
            interval_achieved = utils::to_us(now_ - __time_start) >= time_interval;
        } else if constexpr(IntervalType == types::OPERATIONS) {
            interval_achieved = __n_dispatched_operations - __operation_start >= operation_interval;
        }
        return interval_achieved;
    }

    void schedule(Operation<T>* operation) {
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

                if constexpr(IntervalType == types::MICROSECONDS){
                    __time_start += time_interval;
                } else if constexpr(IntervalType == types::OPERATIONS){
                    __operation_start += operation_interval;
                }
                __repartitioning = false;
            }
        }
    }

    void stop_signal(){
        for (size_t i = 0; i < __n_partitions; i++)
        {
            Operation<T>* end_operation = new Operation<T>(END);
            __workers[i]->push_operation(end_operation);
        }
    }


    void scheduling_loop() {
        while(true){
            Operation<T>* operation;
            __scheduling_queue->template pop(operation);
            if (operation->type() == END){
                stop_signal();
                delete operation;
                break;
            }
            schedule(operation);
        }
        
        __schedule_end = utils::now();
    }


    void sync_repartition() {
        __level++;
        __old_storages.push_back(__storages);
        __storages = new storage_t[__n_partitions];
        RepartitionOperation<T> *sync_operation = new RepartitionOperation<T>(__n_partitions, __storages);
        for (size_t i = 0; i < __n_partitions; i++)
        {
            __storages[i].level(__level);
            __workers[i]->push_operation(sync_operation);
        }
    }

    template<OperationType type>
    void submit(Operation<T>* operation){
        TrackingInfo<T>* tracking_info = TrackingInfo<T>::template get_tracking_info<type>(operation);
        __scheduling_queue->template push(operation, tracking_info);
    }

    void update_partition_scheme(){
        if (__n_partitions > 1){
            worker_map_t *old_map =  __worker_map;
            __worker_map = __updated_worker_map;
            __updated_worker_map = old_map;
        }

        sync_repartition();
    }

    void order_partitioning(){
        types::time_point begin;
        if constexpr(utils::ENABLE_INFO){
            begin = utils::now();
        }

        __input_graph.update();
        if constexpr(utils::ENABLE_INFO){
            __graph_copy_duration.push_back(utils::now() - begin);
        }

        if constexpr(utils::ENABLE_INFO){
            __repartition_request_timestamp.push_back(utils::now());
        }
        __repart_notification->Notify();
    }

    void update_graph_loop() {
        while(true) {
            __n_processed_operations++;
            TrackingInfo<T> *tracking_info;
            __scheduling_queue->template pop(tracking_info);
            if (tracking_info->type() == END){
                __stop.store(true, std::memory_order_relaxed);
                __repart_notification->Notify();
                delete tracking_info;
                break;
            }
            update_graph(tracking_info);

            if(__repartition_signal.load(std::memory_order_acquire)){
                bool has_vertices;
                if constexpr(utils::ENABLE_EDGES){
                    has_vertices = __graph.n_vertex() > 1;
                } else {
                    has_vertices = __edgeless_graph.n_vertex() > 1;
                }
                if(has_vertices){
                    __repartition_signal.store(false, std::memory_order_relaxed);
                    order_partitioning();
                }
            }

        }
    }

    void not_partitioning(){
        
        if constexpr(utils::ENABLE_INFO){
            __repartition_timestamps.push_back(utils::now());
        }


        types::time_point reconstruction_begin;
        if constexpr(utils::ENABLE_INFO){
            __repartition_end_timestamps.push_back(utils::now());
            reconstruction_begin = utils::now();
        }

        if constexpr(utils::ENABLE_INFO){
            __reconstruction_duration.push_back(utils::now() - reconstruction_begin);
        }
    }

    void partitioning_loop(){
        while(true){
            __repart_notification->WaitForNotification();
            delete __repart_notification;
            __repart_notification = new absl::Notification();
            if (__stop.load(std::memory_order_relaxed)){
                break;
            }
            if (__n_partitions > 1){
                partitioning();
            } else {
                not_partitioning();
            }
            __update.store(true, std::memory_order_release);
        }
    }

    void update_graph(TrackingInfo<T>* tracking_info) {
        if (tracking_info->type() == SCAN) {
            size_t len = tracking_info->len();
            auto key_it = __graph_keys.lower_bound(tracking_info->key());
            for (auto i = 0; i < len && key_it != __graph_keys.end(); i++) {
                T key_i = *key_it;

                if constexpr(utils::ENABLE_EDGES){
                    __graph.increment_vertice_weight(key_i, 1);
                    auto key_j_it = key_it;
                    for (auto j = i+1; j < len && key_j_it != __graph_keys.end(); j++) {
                        T key_j = *key_j_it;
                        __graph.increment_edge_weight(key_i, key_j, 1);
                    }
                } else {
                    __edgeless_graph.increment_vertice_weight(key_i, 1);
                }
                key_it++;
            }
        } else {
            int final_weight;
            T key = tracking_info->key();
            if constexpr(utils::ENABLE_EDGES){
                final_weight = __graph.increment_vertice_weight(key, 1);
            } else {
                final_weight = __edgeless_graph.increment_vertice_weight(key, 1);
            }
            if (final_weight == 1){
                __graph_keys.insert(key);
            }
        }
    }


    void partitioning() {
        
        if constexpr(utils::ENABLE_INFO){
            __repartition_timestamps.push_back(utils::now());
        }

        std::vector<int> scheme;
        if constexpr(utils::ENABLE_EDGES){
            model::multilevel_cut(
                __input_graph.vertice_weight, 
                __input_graph.x_edges, 
                __input_graph.edges, 
                __input_graph.edges_weight,
                __n_partitions, 
                __repartition_method,
                scheme
            );
        } else {
            model::greedy_partition(
                __input_graph.vertice_weight,
                __n_partitions,
                scheme
            );
        }

        types::time_point reconstruction_begin;
        if constexpr(utils::ENABLE_INFO){
            __repartition_end_timestamps.push_back(utils::now());
            reconstruction_begin = utils::now();
        }

        __updated_worker_map->clear();
        __updated_worker_map->reserve(__input_graph.vertice_to_pos.size());

        for (auto& it : __input_graph.vertice_to_pos) {
            T key = it.first;
            int position = it.second;
            int worker = scheme[position];  
            if (worker >= __n_partitions) {
                printf("ERROR: worker was %d!\n", worker);
                fflush(stdout);
            }
            __updated_worker_map->emplace(key, __workers[worker]);
        }

        if constexpr(utils::ENABLE_INFO){
            __reconstruction_duration.push_back(utils::now() - reconstruction_begin);
        }
    }

    size_t n_executed_operations() const{
        size_t n_executed_operations = 0;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            n_executed_operations += __workers[i]->n_executed_operations();
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
            n_enqueued_operations += __workers[i]->operation_queue_size();
        }
        return n_enqueued_operations;
    }

    std::vector<size_t> in_queue_amount() const{
        std::vector<size_t> in_queue;
        for (size_t i = 0; i < __n_partitions; i++)
        {
            size_t amount = __workers[i]->operation_queue_size();
            in_queue.push_back(amount);
        }
        return in_queue;
    }

    size_t graph_vertices(){
        if constexpr(utils::ENABLE_EDGES){
            return __graph.n_vertex();
        } else {
            return __edgeless_graph.n_vertex();
        }
    }

    size_t graph_edges(){
        if constexpr(utils::ENABLE_EDGES){
            return __graph.n_edges();
        } else {
            return __edgeless_graph.n_edges();
        }
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
            count += __workers[i]->error_count();
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
    int __rr_counter = 0;
    int __n_dispatched_operations = 0;

    worker_t** __workers;
    worker_map_t* __worker_map;

    std::thread graph_thread;
    cpu_set_t graph_cpu_set;

    std::thread scheduling_thread;
    cpu_set_t scheduler_cpu_set;

    model::Graph<T> __graph;
    model::EdgelessGraph<T> __edgeless_graph;
    model::CutMethod __repartition_method;
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

    schedule_queue_t* __scheduling_queue;

    size_t __n_processed_operations = 0;

    worker_map_t* __updated_worker_map;
    InputGraph<T> __input_graph;

    absl::Notification *__repart_notification;

    std::thread reparting_thread;
    cpu_set_t reparting_cpu_set;


    std::atomic_bool __repartition_signal;
    std::atomic_bool __update;
    std::atomic_bool __stop = false;

    bool __repartitioning;


    int __operation_start = 0;
    
    worker_set_t __involved_workers;

    storage_map_t __storage_map;
    storage_t* __storages;
    size_t __level;

    std::vector<storage_t*> __old_storages;

    key_set_t __keys;
    key_set_t __graph_keys;
};

};


#endif
