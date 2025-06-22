#ifndef KVPAXOS_WORKER_H
#define KVPAXOS_WORKER_H


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

#include "operation.hpp"
#include "get_callback_operation.hpp"
#include "set_callback_operation.hpp"
#include "scan_callback_operation.hpp"
#include "del_callback_operation.hpp"
#include "get_operation.hpp"
#include "set_operation.hpp"
#include "scan_operation.hpp"
#include "del_operation.hpp"
#include "scan_future_operation.hpp"
#include "get_future_operation.hpp"
#include "repartition_operation.hpp"
#include <semaphore>

#include <readerwriterqueue.h>

namespace kvpaxos {
using namespace workload;


template <typename T, typename storage_t, size_t QSize = 0>
class Worker {
typedef Worker<T, storage_t, QSize> worker_t;
typedef std::unordered_map<T, worker_t*> worker_map_t;
typedef moodycamel::BlockingReaderWriterQueue<Operation<T>*> operation_queue_t;

public:
    Worker(size_t id, storage_t *storage)
        : __id{id},
          __n_executed_operations{0},
          __available(0),
          __remaining_space(QSize)

    {
        __storage = storage;
        __output_file = std::ofstream("partition_output_" + std::to_string(__id));
    }
    
    ~Worker() {
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        __output_file.flush();
        __output_file.close();
    }

    void start_worker_thread() {

        worker_thread_ = std::thread(&worker_t::thread_loop, this);
        utils::set_affinity(__id+5, worker_thread_, cpu_set);
    }

    size_t operation_queue_size() {
        return __operations_queue.size_approx();
    }

    size_t error_count() {
        return __error_count;
    }

    void push_operation(Operation<T> *operation) {
        if constexpr(QSize > 0){
            __remaining_space.acquire();
        }
        __operations_queue.enqueue(operation);
        __available.release();
    }

    Operation<T> * pop_operation() {
        Operation<T> *operation;
        __available.acquire();
        __operations_queue.try_dequeue(operation);
        if constexpr(QSize > 0){
            __remaining_space.release();
        }
        return operation;
    }


    int id() const {
        return __id;
    }

    size_t n_executed_operations() const {
        return __n_executed_operations;
    }

private:

    inline void read(storage_t* &storage, T &key, std::string& val){
        int len = storage->read(key, val);
        if (len >= 0){
            if (storage != __storage){
                __storage->write(key, val);
            }
            return;
        }
        __error_count++;
    }

    inline void read_some(ScanOperation<T>* &operation, T &key, size_t &len){
        for (size_t i = 0; i < len; i++)
        {
            if (operation->worker_manages_key(i, this)){
                storage_t* storage = operation->template storage<storage_t>(i);
                T key_i = operation->key(i);
                read(storage, key_i, operation->get_scaned_value(i));
            }
        }
        

    }

    inline void print_read(T &key, std::string &value){
        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "read( " << key << " ): " << value << "\n";
        }
    }

    inline void print_write(T &key, const std::string &value){
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

    template<OperationType TYPE>
    inline void get(GetOperation<T> *operation){
        T key = operation->key();
        storage_t* storage = operation->template storage<storage_t>();
        if constexpr(TYPE == GET_CALLBACK){
            GetCallbackOperation<T>* get_cb = static_cast<GetCallbackOperation<T>*>(operation);
            std::string value;
            read(storage, key, value);
            print_read(key, value);
            get_cb->callback(&value);
            delete get_cb;
        } else if constexpr(TYPE == GET_FUTURE){
            GetFutureOperation<T>* get_future = static_cast<GetFutureOperation<T>*>(operation);
            std::string *value = get_future->value();
            read(storage, key, *value);
            print_read(key, *value);
            get_future->notify();
        }
        __n_executed_operations++;
    }

    template<OperationType TYPE>
    inline void set(SetOperation<T> *operation){
        T key = operation->key();
        std::string value = operation->value();
        __storage->write(key, value);
        print_write(key, value);
        if constexpr(TYPE == SET_CALLBACK){
            SetCallbackOperation<T>* set_cb = static_cast<SetCallbackOperation<T>*>(operation);
            set_cb->callback(&value);
            delete set_cb;
        } else {
            delete operation;
        }
        __n_executed_operations++;
    }

    template<OperationType TYPE>
    inline void del(DelOperation<T> *operation){
        T key = operation->key();
        __storage->del(key);
        print_del(key);
        if constexpr(TYPE == DEL_CALLBACK){
            DelCallbackOperation<T>* del_cb = static_cast<DelCallbackOperation<T>*>(operation);
            del_cb->callback();
            delete del_cb;
        } else {
            delete operation;
        }
        __n_executed_operations++;
    }

    template<OperationType TYPE>
    inline void scan(ScanOperation<T> *operation){
        size_t len = operation->len();
        T key = operation->key();
        read_some(operation, key, len);
        bool is_coordinator = operation->is_coordinator();
        std::string *values = operation->get_scaned_values();
        print_scan(is_coordinator, key, len, values);
        if constexpr(TYPE == SCAN_CALLBACK){
            ScanCallbackOperation<T>* scan_cb = static_cast<ScanCallbackOperation<T>*>(operation);
            if (is_coordinator) {
                scan_cb->callback(values);
                delete[] values;
                delete scan_cb;
                __n_executed_operations++;
            }
        } else if constexpr(TYPE == SCAN_FUTURE){
            ScanFutureOperation<T>* scan_future = static_cast<ScanFutureOperation<T>*>(operation);
            if (is_coordinator){
                scan_future->notify();
                __n_executed_operations++;
            }
        }
    }

    inline void repart(RepartitionOperation<T>* operation){
        __storage = &operation->template storage<storage_t>()[__id];
        __storage->init();

        int is_coordinator = operation->barrier_wait();
        if (is_coordinator) {
            delete operation;
        }
    }

    void thread_loop() {
        Operation<T> *operation;

        while (true) {

            operation = pop_operation();
            OperationType type = operation->type();
            switch (type){
            case GET_FUTURE:
                get<GET_FUTURE>(static_cast<GetOperation<T>*>(operation));
                break;
            case GET_CALLBACK:
                get<GET_CALLBACK>(static_cast<GetOperation<T>*>(operation));
                break;
            case SET:
                set<SET>(static_cast<SetOperation<T>*>(operation));
                break;
            case SET_CALLBACK:
                set<SET_CALLBACK>(static_cast<SetOperation<T>*>(operation));
                break;
            case SCAN_FUTURE:
                scan<SCAN_FUTURE>(static_cast<ScanOperation<T>*>(operation));
                break;
            case SCAN_CALLBACK:
                scan<SCAN_CALLBACK>(static_cast<ScanOperation<T>*>(operation));
                break;
            case DEL:
                del<DEL>(static_cast<DelOperation<T>*>(operation));
                break;
            case DEL_CALLBACK:
                del<DEL_CALLBACK>(static_cast<DelOperation<T>*>(operation));
                break;
            case REPARTITION:
                repart(static_cast<RepartitionOperation<T>*>(operation));
                continue;
            case END:
                __output_file << "end() \n";
                delete operation;
                return;
            default:
                abort();
                return;
            }
        }
    }

    size_t __id;
    size_t __n_executed_operations;
    storage_t *__storage;
    cpu_set_t cpu_set;

    std::thread worker_thread_;
    std::counting_semaphore<SEM_VALUE_MAX> __available;
    operation_queue_t __operations_queue;

    std::counting_semaphore<SEM_VALUE_MAX> __remaining_space;

    size_t __error_count = 0;

    std::ofstream __output_file;



};

}

#endif
