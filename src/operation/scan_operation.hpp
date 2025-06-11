#ifndef WORKLOAD_SCAN_OPERATION_H
#define WORKLOAD_SCAN_OPERATION_H

#include <string>
#include <pthread.h>
#include <atomic>

#include "utils.h"
#include "operation.hpp"


namespace workload {

union sync_t{
    pthread_barrier_t barrier;
    std::atomic_int counter;
};

template<typename T>
class ScanOperation: public Operation<T> {
public:
    ScanOperation(){}

    ScanOperation(T key, size_t len)
        : Operation<T>(SET, key){
        __len = len;
    }

    void destroy_multi_partition_scan(){
        if constexpr(utils::ENABLE_LINEARIZABLE){
            pthread_barrier_destroy(&__synchronizer.barrier);
        }
        delete[] __values;
        delete[] __key_to_addr;
    }

    ~ScanOperation(){}

    inline void init_scan_data(){
        __key_to_addr = new char*[__len];
        __values = new std::string[__len];
    }

    inline void init_coordination(size_t involved_partitions){
        if constexpr(utils::ENABLE_LINEARIZABLE){
            pthread_barrier_init(&__synchronizer.barrier, NULL, involved_partitions);
        } else {
            __synchronizer.counter.store(involved_partitions, std::memory_order_relaxed);
        }
    }

    inline void set_is_single_partition(){
        __values = nullptr;
    }

    inline bool is_multi_partition(){
        return __values != nullptr;
    }

    template<typename PartitionT>
    inline void set_key_to_partition(size_t &idx, PartitionT* &p_addr){
        __key_to_addr[idx] = reinterpret_cast<char*>(p_addr);
    }
    template<typename PartitionT>
    inline bool key_in_partition(size_t &idx, PartitionT* p_addr){
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
            return pthread_barrier_wait(&__synchronizer.barrier) == PTHREAD_BARRIER_SERIAL_THREAD;
        } else {
            return 1 == __synchronizer.counter.fetch_add(-1);
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
};

}

#endif
