#ifndef KVPAXOS_PARTITION_H
#define KVPAXOS_PARTITION_H


#include <pthread.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <csignal>
#include <iostream>
#include "utils.h"
#include "rocks_db_storage.h"

#include "operation.hpp"
#include "get_callback_operation.hpp"
#include "set_callback_operation.hpp"
#include "scan_callback_operation.hpp"
#include "del_callback_operation.hpp"
#include "get_operation.hpp"
#include "set_operation.hpp"
#include "scan_operation.hpp"
#include "del_operation.hpp"
#include "repartition_operation.hpp"


namespace kvpaxos {
using namespace kvstorage;
using namespace workload;

typedef RocksDBStorage storage_t;

template <typename T, size_t QSize = 0>
class Partition {

typedef Partition<T, QSize> partition_t;
typedef std::unordered_map<T, partition_t*> partition_map_t;
typedef std::queue<Operation<T>*> operation_queue_t;
public:
    Partition(size_t id)
        : __id{id},
          __n_executed_operations{0}
    {
        storage[__id] = storage_t(0);
        __output_file = std::ofstream("partition_output_" + std::to_string(__id));
    }
    
    ~Partition() {
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        __output_file.flush();
        __output_file.close();
    }

    void start_worker_thread() {
        sem_init(&semaphore_, 0, 0);
        if constexpr(QSize > 0){
            sem_init(&remaining_space_, 0, QSize);
        }

        worker_thread_ = std::thread(&partition_t::thread_loop, this);
        utils::set_affinity(__id+5, worker_thread_, cpu_set);
    }

    size_t operation_queue_size() const {
        return __operations_queue.size();
    }

    size_t error_count() {
        return __error_count;
    }

    void push_operation(Operation<T> *operation) {
        if constexpr(QSize > 0){
            sem_wait(&remaining_space_);
        }
        __queue_mutex.lock();
            __operations_queue.push(operation);
        __queue_mutex.unlock();
        sem_post(&semaphore_);
    }

    Operation<T> * pop_operation() {
        Operation<T> *operation;
        sem_wait(&semaphore_);

        __queue_mutex.lock();
            operation = __operations_queue.front();
            __operations_queue.pop();
        __queue_mutex.unlock();
        if constexpr(QSize > 0){
            sem_post(&remaining_space_);
        }
        return operation;
    }


    int id() const {
        return __id;
    }

    size_t n_executed_operations() const {
        return __n_executed_operations;
    }


    static void add_old_partition_map(std::unordered_map<int, partition_t*>* version_map){
        version_maps_mtx.lock();
        version_maps.push_back(version_map);
        version_maps_mtx.unlock();
    }

    static void create_storage(size_t partitions_){
        partitions = partitions_;
        storage = new storage_t[partitions];
    }
private:

    void read(T &key, std::string& val){
        int len = storage[__id].read(key, val);
        if (len >= 0){
            return;
        } else {
            for (size_t i = version_count-1; i >= 0; i--)
            {
                version_maps_mtx.lock_shared();
                partition_map_t *map = version_maps.at(i);
                version_maps_mtx.unlock_shared();

                auto map_it = map->find(key);
                if (map_it != map->end()){
                    size_t prev_id = map_it->second->__id;
                    int len_old = prev_storage[i][prev_id].read(key, val);
                    if (len_old >= 0){
                        storage[__id].write(key, val);
                        return;
                    }
                }
            }
            
        }
        __error_count++;
    }

    inline void scan_some(ScanOperation<T>* operation, T &key, size_t &len){
        for (size_t i = 0; i < len; i++)
        {
            if (operation->key_in_partition(i, this)){
                T key_i = key + i;
                read(key_i, operation->get_scaned_value(i));
            }
        }
        

    }
    inline void scan(T &key, size_t &len, std::string* values){

        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "scan( " << key << ", "<< len << " ): [";
        }
        for (auto i = 0; i < len; i++) {
            T key_i = key+i;
            read(key_i, values[i]);
            if constexpr(utils::ENABLE_ANSWER){
                 __output_file << "\"" << values[i] << "\", ";
            }
        }

        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "]\n";
        }
    }

    inline void print_read(T &key, std::string &value){
        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "read( " << key << " ): " << value << "\n";
        }
    }

    inline void print_write(T &key, std::string &value){
        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "write( " << key << ", " << value << " )\n";
        }
    }

    inline void print_scan(bool is_coordinator, T &key, size_t &len, const std::string* values){
        if (is_coordinator) {
            if constexpr(utils::ENABLE_ANSWER){
                __output_file << "scan( " << key << ", "<< len << " ): [";
                for (size_t i = 0; i < len; i++)
                {
                    __output_file << "\"" << values[i] << "\",";
                }
                __output_file << "]\n";
            }
        }
    }

    void print_del(T &key){
        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "del( " << key << " )\n";
        }
    }


    void thread_loop() {
        std::string value;
        T key;
        OperationType type;
        Operation<T> *operation;

        while (true) {

            operation = pop_operation();
            type = operation->type();
            key = operation->key();

            switch (type){
            case GET:
            {
                read(key, value);
                print_read(key, value);
                GetCallbackOperation<T>* get_op = static_cast<GetCallbackOperation<T>*>(operation);
                delete get_op;
                __n_executed_operations++;
                break;
            }
            case GET_CALLBACK:
            {
                read(key, value);
                print_read(key, value);
                GetCallbackOperation<T>* get_op = static_cast<GetCallbackOperation<T>*>(operation);
                get_op->callback(&value);
                __n_executed_operations++;
                break;
            }
            case SET:
            {
                SetOperation<T>* set_op = static_cast<SetOperation<T>*>(operation);
                value = set_op->value();
                storage[__id].write(key, value);
                print_write(key, value);
                delete set_op;
                __n_executed_operations++;
                break;
            }
            case SET_CALLBACK:
            {
                SetCallbackOperation<T>* set_op = static_cast<SetCallbackOperation<T>*>(operation);
                value = set_op->value();
                storage[__id].write(key, value);
                print_write(key, value);
                set_op->callback(&value);
                delete set_op;
                __n_executed_operations++;
                break;
            }
            case SCAN:
            {
                ScanOperation<T>* scan_op = static_cast<ScanOperation<T>*>(operation);
                size_t len = scan_op->len();
                if (scan_op->is_multi_partition()){
                    scan_some(scan_op, key, len);
                    bool is_coordinator = scan_op->is_coordinator();
                    print_scan(is_coordinator, key, len, scan_op->get_scaned_values());
                    if constexpr(utils::ENABLE_LINEARIZABLE){
                        is_coordinator = scan_op->is_coordinator();
                    }
                    if (is_coordinator) {
                        scan_op->destroy_multi_partition_scan();
                        delete scan_op;
                        __n_executed_operations++;
                    }
                    continue;
                } else {
                    std::string values[len];
                    scan(key, len, values);
                    delete scan_op;
                    print_scan(true, key, len, values);
                    __n_executed_operations++;
                }
                break;
            }
            case SCAN_CALLBACK:
            {
                ScanCallbackOperation<T>* scan_op = static_cast<ScanCallbackOperation<T>*>(operation);
                size_t len = scan_op->len();
                if (scan_op->is_multi_partition()){
                    scan_some(scan_op, key, len);
                    bool is_coordinator = scan_op->is_coordinator();
                    print_scan(is_coordinator, key, len, scan_op->get_scaned_values());
                    if constexpr(utils::ENABLE_LINEARIZABLE){
                        scan_op->callback(scan_op->get_scaned_values());
                        is_coordinator = scan_op->is_coordinator();
                    } else {
                        scan_op->callback(scan_op->get_scaned_values());
                    }
                    if (is_coordinator) {
                        scan_op->destroy_multi_partition_scan();
                        delete scan_op;
                        __n_executed_operations++;
                    }
                    continue;
                } else {
                    std::string values[len];
                    scan(key, len, values);
                    print_scan(true, key, len, values);
                    scan_op->callback(&value);
                    delete scan_op;
                    __n_executed_operations++;
                }
                break;
            }
            case DEL:
            {
                storage[__id].del(key);
                print_del(key);
                DelOperation<T>* del_op = static_cast<DelOperation<T>*>(operation);
                delete del_op;
                __n_executed_operations++;
                break;
            }
            case DEL_CALLBACK:
            {
                storage[__id].del(key);
                print_del(key);
                DelCallbackOperation<T>* del_op = static_cast<DelCallbackOperation<T>*>(operation);
                static_cast<DelCallbackOperation<T>*>(del_op)->callback();
                delete del_op;
                __n_executed_operations++;
                break;
            }
            case REPARTITION:
            {
                RepartitionOperation<T>* rep_op = static_cast<RepartitionOperation<T>*>(operation);
                int coordinator = rep_op->barrier_wait();
                if (coordinator == PTHREAD_BARRIER_SERIAL_THREAD) {
                    prev_storage.push_back(storage);
                    version_count++;
                    storage = new storage_t[partitions];
                    if constexpr(utils::ENABLE_ANSWER){
                        __output_file << "repartition() \n";
                    }
                }
                coordinator = rep_op->barrier_wait();
                if (coordinator == PTHREAD_BARRIER_SERIAL_THREAD) {
                    delete rep_op;
                }
                storage[__id] = storage_t(version_count);
                continue;
            }
            case END:
            {
                __output_file << "end() \n";
                return;
            }
            default:
            {
                std::raise(SIGINT);
                return;
            }
            }
        }
    }

    size_t __id;
    size_t __n_executed_operations;
    static storage_t *storage;
    cpu_set_t cpu_set;

    std::thread worker_thread_;
    sem_t semaphore_;
    operation_queue_t __operations_queue;
    std::mutex __queue_mutex;

    sem_t remaining_space_;

    size_t __error_count = 0;
    static size_t partitions;
    static std::vector<storage_t*> prev_storage;
    static int version_count;
    static std::vector<partition_map_t*> version_maps;
    static std::shared_mutex version_maps_mtx;

    std::ofstream __output_file;


};
template<typename T, size_t QSize>
std::vector<storage_t*> Partition<T, QSize>::prev_storage;

template<typename T, size_t QSize>
int Partition<T, QSize>::version_count = 0;

template<typename T, size_t QSize>
size_t Partition<T, QSize>::partitions = 0;

template<typename T, size_t QSize>
storage_t* Partition<T, QSize>::storage;

template<typename T, size_t QSize>
std::vector<std::unordered_map<T, Partition<T, QSize>*>*> Partition<T, QSize>::version_maps;

template<typename T, size_t QSize>
std::shared_mutex Partition<T, QSize>::version_maps_mtx;

}

#endif
