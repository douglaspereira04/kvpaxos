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
#include "ankerl/unordered_dense.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/btree_set.h"
#include "absl/synchronization/notification.h"
#include "absl/synchronization/mutex.h"
#include <unistd.h>
#include <string_view>

#include "tkrzw_dbm_tree.h"

namespace kvpaxos {
enum SubmitStatus{
    OK = 0,
    NOT_FOUND = 1
};

template <typename T, size_t Partitions, typename storage_t, bool Rebalance, size_t QSize = 0, types::interval_type IntervalType = types::OPERATIONS>
class KVStore{
typedef KVStore<T, Partitions, storage_t, Rebalance, QSize, IntervalType> kvstore_t;
typedef kvpaxos::Worker<T, storage_t, QSize> worker_t;
typedef ankerl::unordered_dense::map<T, worker_t*> worker_map_t;
typedef ankerl::unordered_dense::map<T, storage_t*> storage_map_t;
typedef model::Queue<T*> graph_queue_t;
typedef absl::flat_hash_set<worker_t*> worker_set_t;
typedef absl::btree_set<T> key_set_t;

public:

    KVStore() {}
    KVStore(int repartition_interval,
                model::CutMethod repartition_method
    ) {
        __joined = false;
        __rr_worker_counter = 0;
        __n_dispatched_operations = 0;
        init_keys(__keys);
        __level = 0;
        __storages = new storage_t[Partitions];
        for (auto i = 0; i < Partitions; i++) {
            __storages[i].level(0);
            __storages[i].init();
            __workers[i].~worker_t();
            new (&__workers[i]) worker_t(i, &__storages[i]);
        }

        if constexpr(Rebalance) {
            __edgeless_graph = model::EdgelessGraph<T>();
            __input_graph = InputGraph<T>(&__edgeless_graph);
            if constexpr(IntervalType == types::MICROSECONDS){
                __time_start = utils::now();
                time_interval = std::chrono::microseconds(repartition_interval);
                __operation_start = 0;
            } else if constexpr(IntervalType == types::OPERATIONS){
                __operation_start = 0;
                operation_interval = repartition_interval;
            }
            __repartition_method = repartition_method;


            __update.store(false, std::memory_order_seq_cst);
            __repartitioning = false;

            __repart_notification = new absl::Notification();
            reparting_thread = std::thread(&kvstore_t::partitioning_loop, this);
            utils::set_affinity(4, reparting_thread, reparting_cpu_set);
        }

    }

    void init_keys(tkrzw::TreeDBM &keys){
        tkrzw::Status status;
        std::string path = "/tmp/repart_kv_storage/"+ std::to_string(static_cast<int>(getpid())) +"/keys";
        size_t i = 0;
        do {
            status = keys.Open(path, true);
        } while(!status.IsOK() && 10 > i++);
        assert(status.IsOK());
    }

    inline void get_storage(T& key, storage_t* &storage){
        std::string stored;
        if (__keys.Get(key, &stored)) {
            storage = *reinterpret_cast<storage_t**>(stored.data());
        } else {
            storage = nullptr;
        }
    }

    inline size_t get_storages(const T& key, size_t len, ScanOperation<T> &operation){
        std::string stored;
        auto it = __keys.MakeIterator();
        it->Jump(key);
        std::string value;
        size_t i;
        for (i = 0; i < len; i++, it->Next()) {
            std::string storag_str;
            tkrzw::Status status = it->Get(&operation.key(i), &storag_str);
            storage_t* storage = *reinterpret_cast<storage_t**>(stored.data());
            operation.storage(i, storage);
            if(!status.IsOK()){
                break;
            }
        }
        return i;
    }

    inline void set_storage(const T& key, storage_t* &storage){
        std::string_view value(reinterpret_cast<const char*>(&storage), sizeof(storage));
        tkrzw::Status status = __keys.Set(key, value);
        if (!status.IsOK()) {
            abort();
        }
    }


    inline void remove_storage(const T& key, storage_t* &storage){
        std::string stored;
        tkrzw::Status status = __keys.Remove(key, &stored);
        if (!status.IsOK()) {
            storage = *reinterpret_cast<storage_t**>(stored.data());
            abort();
        }
    }

    inline void swap_storage(const T& key, storage_t* &storage, storage_t* &old_storage){
        std::string_view storage_view(reinterpret_cast<const char*>(&storage), sizeof(storage));
        std::string old_storage_string;
        tkrzw::Status status = __keys.Set(key, storage_view, true, &old_storage_string);
        if (status.IsOK()) {
            old_storage = *reinterpret_cast<storage_t**>(old_storage_string.data());
            return;
        }
        storage = nullptr;
        old_storage = nullptr;
    }

    ~KVStore(){
        if (!__joined){
            join();
        }
    }

    void run() {
        for (size_t i = 0; i < Partitions; i++)
        {
            __workers[i].start_worker_thread();
        }
    }

    void join(){
        __joined = true;
        if constexpr(Rebalance) {
            __stop = true;
            __repart_notification->Notify();
            reparting_thread.join();
        }
        stop_signal();
    }
    
    inline void get(T &key, void (*cb)(T &key, std::string*)){
        GetCallbackOperation<T>* operation = new GetCallbackOperation<T>(key, cb);
        dispatch<GET_CALLBACK>(key, *operation);
    }
    
    void set(T &key, const std::string &value, void (*cb)(T &key, std::string*)){
        SetCallbackOperation<T>* operation = new SetCallbackOperation<T>(key, new std::string(value), cb);
        dispatch<SET_CALLBACK>(key, *operation);
    }
    
    void scan(T &key, size_t len, void (*cb)(T &key, std::string*)){
        std::string *values = new std::string[len];
        ScanCallbackOperation<T>* operation = new ScanCallbackOperation<T>(key, len, values, cb);
        dispatch<SCAN_CALLBACK>(key, *operation);
    }
    
    void del(T &key, void (*cb)(T &key)){
        DelCallbackOperation<T>* operation = new DelCallbackOperation<T>(key, cb);
        dispatch<DEL_CALLBACK>(key, *operation);
    }
    
    std::string get(T &key){
        std::string value;
        GetFutureOperation<T> operation(key, &value);
        dispatch<GET_FUTURE>(key, operation);
        operation.wait();
        return value;
    }
    
    void set(T &key, const std::string &value){
        SetOperation<T>* operation = new SetOperation<T>(key, new std::string(value));
        dispatch<SET>(key, *operation);
    }
    
    std::vector<std::string> scan(T &key, size_t len){
        std::vector<std::string> values(len);
        ScanFutureOperation<T> operation(key, len, values.data());
        dispatch<SCAN_FUTURE>(key, operation);
        operation.wait();
        return values;
    }
    
    void del(T &key){
        DelOperation<T>* operation = new DelOperation<T>(key);
        dispatch<DEL>(key, *operation);
    }

    template<OperationType TYPE>
    inline SubmitStatus dispatch(T &key, Operation<T> &operation){
        SubmitStatus status;
        constexpr OperationType generic_type = workload::generic_type<TYPE>();
        if constexpr(generic_type == GET){
            status = dispatch_single_read(key, operation);
        } else if constexpr(generic_type == SET){
            status = dispatch_single_write(key, operation);
        } else if constexpr(generic_type == SCAN){
            status = dispatch_multi_read(key, *static_cast<ScanOperation<T>*>(&operation));
        } else if constexpr(generic_type == DEL){
            status = dispatch_single_del(key, operation);
        }
        __n_dispatched_operations++;
        if constexpr(Rebalance) {
            if (status == SubmitStatus::OK){
                check_and_rebalance();
            }
        }
        return status;
    }

    inline void get_worker(const T &key, worker_t* &worker){
        auto worker_it = __worker_map.find(key);
        if (worker_it != __worker_map.end()){
            worker = worker_it->second;
        }
    }

    inline void get_or_set_worker(const T &key, worker_t* &worker){
        auto worker_it = __worker_map.find(key);
        if (worker_it == __worker_map.end()){
            auto [worker_it, worker_emplaced] = __worker_map.try_emplace(key, &__workers[__rr_worker_counter++%Partitions]);
            worker = worker_it->second;
        } else {
            worker = worker_it->second;
        }
    }

    inline void get_worker_storage(const T &key, const int &worker_id, storage_t* &storage){
        storage_t* worker_storage = &__storages[worker_id];
        swap_storage(key, worker_storage, storage);
    }

    inline SubmitStatus dispatch_single_read(const T &key, Operation<T> &operation){
        worker_t* worker;
        storage_t* storage;

        __schedule_lock.ReaderLock();

        get_worker(key, worker);
        if (worker == nullptr){
            return SubmitStatus::NOT_FOUND;
        }
        int worker_id = worker->id();
        get_worker_storage(key, worker_id, storage);
        operation.storage(storage);
        worker->push_operation(&operation);

        __schedule_lock.ReaderUnlock();

        update_graph(key);
        return SubmitStatus::OK;
    }

    inline SubmitStatus dispatch_single_write(const T &key, Operation<T> &operation){
        worker_t* worker;
        storage_t* storage;

        __schedule_lock.WriterLock();
        get_or_set_worker(key, worker);
        int worker_id = worker->id();
        get_worker_storage(key, worker_id, storage);
        operation.storage(storage);

        worker->push_operation(&operation);
        __schedule_lock.WriterUnlock();

        update_graph(key);
        return SubmitStatus::OK;
    }

    inline SubmitStatus dispatch_single_del(const T &key, Operation<T> &operation){
        worker_t* worker;
        storage_t* storage;

        __schedule_lock.WriterLock();
        get_worker(key, worker);
        remove_storage(key, storage);
        operation.storage(storage);

        worker->push_operation(&operation);
        __schedule_lock.WriterUnlock();
  
        update_graph(key);
        return SubmitStatus::OK;
    }

    inline SubmitStatus dispatch_multi_read(const T &key, ScanOperation<T> &operation){
        worker_set_t involved_workers;
        size_t len = operation.len();
        operation.init_scan_data();
        std::string* keys = new std::string[len];

        __schedule_lock.ReaderLock();

        size_t count = get_storages(key, len, operation);
        if (count < len) {
            __schedule_lock.ReaderUnlock();
            delete[] keys;
            return SubmitStatus::NOT_FOUND; 
        }
        for (size_t i = 0; i < len; i++) {
            worker_t* worker;
            get_worker(operation.key(i), worker);
            keys[i] = operation.key(i);
            worker->push_operation(&operation);
            operation.worker(i, worker);
        }

        __schedule_lock.ReaderUnlock();

        for (size_t i = 0; i < len; i++) {
            update_graph(keys[i]);
        }
        delete[] keys;
        return SubmitStatus::OK;
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

    inline void check_and_rebalance() {
        if (!__repartitioning){
            if (interval_achieved()) {
                order_partitioning();
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

    void stop_signal(){
        __schedule_lock.WriterLock();
        for (size_t i = 0; i < Partitions; i++)
        {
            Operation<T>* end_operation = new Operation<T>(END);
            __workers[i].push_operation(end_operation);
        }
        __schedule_lock.WriterUnlock();
    }


    void sync_repartition() {
        __level++;
        __old_storages.push_back(__storages);
        __storages = new storage_t[Partitions];
        RepartitionOperation<T> *sync_operation = new RepartitionOperation<T>(Partitions, __storages);
        for (size_t i = 0; i < Partitions; i++)
        {
            __storages[i].level(__level);
            __workers[i].push_operation(sync_operation);
        }
    }

    void update_partition_scheme(){
        if (Partitions > 1){
            std::swap(__worker_map, __updated_worker_map);
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

    inline void update_graph(const T &key) {
        __edgeless_graph.increment_vertice_weight(key, 1);
        __n_processed_operations++;
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
            if (__stop){
                break;
            }
            if (Partitions > 1){
                partitioning();
            } else {
                not_partitioning();
            }
            __update.store(true, std::memory_order_release);
        }
    }


    void partitioning() {
        
        if constexpr(utils::ENABLE_INFO){
            __repartition_timestamps.push_back(utils::now());
        }

        std::vector<int> scheme;
        model::greedy_partition(
            __input_graph.vertice_weight,
            Partitions,
            scheme
        );

        types::time_point reconstruction_begin;
        if constexpr(utils::ENABLE_INFO){
            __repartition_end_timestamps.push_back(utils::now());
            reconstruction_begin = utils::now();
        }

        __updated_worker_map.clear();
        __updated_worker_map.reserve(__input_graph.vertice_to_pos.size());

        for (auto& it : __input_graph.vertice_to_pos) {
            T key = it.first;
            int position = it.second;
            int worker = scheme[position];  
            if (worker >= Partitions) {
                printf("ERROR: worker was %d!\n", worker);
                fflush(stdout);
            }
            __updated_worker_map.emplace(key, &__workers[worker]);
        }

        if constexpr(utils::ENABLE_INFO){
            __reconstruction_duration.push_back(utils::now() - reconstruction_begin);
        }
    }

    size_t n_executed_operations() const{
        size_t n_executed_operations = 0;
        for (size_t i = 0; i < Partitions; i++)
        {
            n_executed_operations += __workers[i].n_executed_operations();
        }
        return n_executed_operations;
    }

    size_t n_processed_operations() const{
        return __n_processed_operations;
    }

    size_t in_queue_amount(size_t idx) const{
        return __workers[idx].operation_queue_size();
    }

    size_t graph_vertices(){
        return __edgeless_graph.n_vertex();
    }

    size_t graph_edges(){
        return __edgeless_graph.n_edges();
    }

    int n_dispatched_operations(){
        return __n_dispatched_operations;
    }

    int error_count(){
        int count = 0;
        for (size_t i = 0; i < Partitions; i++)
        {
            count += __workers[i].error_count();
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

    int __rr_worker_counter = 0;
    int __n_dispatched_operations = 0;

    tkrzw::TreeDBM __keys;

    worker_t  __workers[Partitions];
    absl::Mutex __schedule_lock;
    worker_map_t __worker_map;
    worker_map_t __updated_worker_map;

    model::EdgelessGraph<T> __edgeless_graph;
    model::CutMethod __repartition_method;

    int operation_interval;
    types::duration time_interval;
    types::time_point __time_start;


    std::vector<types::time_point> __repartition_timestamps;
    std::vector<types::duration> __graph_copy_duration;
    std::vector<types::time_point> __repartition_end_timestamps;
    std::vector<types::time_point> __repartition_request_timestamp;
    std::vector<types::time_point> __repartition_apply_timestamp;
    std::vector<types::duration> __reconstruction_duration;

    size_t __n_processed_operations = 0;

    InputGraph<T> __input_graph;

    absl::Notification *__repart_notification;

    std::thread reparting_thread;
    cpu_set_t reparting_cpu_set;


    std::atomic_bool __update;
    bool __stop = false;

    bool __repartitioning;


    int __operation_start = 0;
    
    bool __joined;

    storage_t* __storages;
    size_t __level;

    std::vector<storage_t*> __old_storages;

};

};


#endif
