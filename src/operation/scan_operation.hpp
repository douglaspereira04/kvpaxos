#ifndef WORKLOAD_SCAN_OPERATION_H
#define WORKLOAD_SCAN_OPERATION_H

#include <string>
#include "absl/synchronization/barrier.h"
#include <atomic>

#include "utils.h"
#include "operation.hpp"


namespace workload {

union sync_t{
    absl::Barrier* barrier;
    std::atomic_int *counter;
};

template<typename T>
class ScanOperation: public Operation<T> {
public:
    ScanOperation(){}

    ScanOperation(T &key, size_t len, std::string* values)
        : Operation<T>(SCAN, key){
        __len = len;
        __values = values;
    }

    ~ScanOperation(){
        if constexpr(utils::ENABLE_LINEARIZABLE){
            delete __synchronizer.barrier;
        } else {
            delete __synchronizer.counter;
        }
        delete[] __key_to_addr;
        delete[] __next_keys;
        delete[] this->__storage;
    }

    inline void key(size_t idx, T &key){
        if (idx == 0){
            this->__key = key;
        } else {
            __next_keys[idx-1] = key;
        }
    }

    inline const T& key(){
        return Operation<T>::key();
    }

    inline const T& key(size_t idx){
        if (idx == 0){
            return this->__key;
        } else {
            return __next_keys[idx-1];
        }
    }

    inline void init_scan_data(){
        __key_to_addr = new char*[__len];
        __next_keys = new T[__len-1];
        this->__storage = new char*[this->__len];
    }

    inline void init_coordination(size_t involved_partitions){
        if constexpr(utils::ENABLE_LINEARIZABLE){
            __synchronizer.barrier = new absl::Barrier(involved_partitions);
        } else {
            __synchronizer.counter = new std::atomic_int;
            __synchronizer.counter->store(involved_partitions, std::memory_order_relaxed);
        }
    }

    template<typename Worker_T>
    inline void worker(size_t &idx, Worker_T* &p_addr){
        __key_to_addr[idx] = reinterpret_cast<char*>(p_addr);
    }
    template<typename Worker_T>
    inline bool worker_manages_key(size_t &idx, Worker_T* p_addr){
        return __key_to_addr[idx] == reinterpret_cast<char*>(p_addr);
    }

    inline std::string& get_scaned_value(size_t idx){
        return __values[idx];
    }

    inline std::string* get_scaned_values(){
        return __values;
    }

    inline void set_scaned_value(size_t idx, std::string&& value){
        __values[idx] = std::move(value);
    }

    inline bool is_coordinator(){
        if constexpr(utils::ENABLE_LINEARIZABLE){
            return __synchronizer.barrier->Block();
        } else {
            return 1 == __synchronizer.counter->fetch_add(-1);
        }
    }

    inline size_t len(){
        return __len;
    }

protected:
    size_t __len;
    sync_t __synchronizer;
    std::string* __values;
    char** __key_to_addr; // TODO: Refactor to template parameter
    T* __next_keys;
};

}

#endif
