#ifndef WORKLOAD_REQUEST_H
#define WORKLOAD_REQUEST_H

#include <string>
#include <cstring>
#include <pthread.h>
#include <fstream>
#include <atomic>

#include "types.h"


namespace workload {


struct scan_data_t{
    std::atomic_int counter;
    std::string** values;
    char** key_to_addr;
};

class Request {
public:
    Request(){}

    Request(RequestType type):
        __type{type}
    {}

    Request(RequestType type, int key):
        __type{type},
        __key{key}
    {}

    Request(RequestType type, int key, size_t len):
        __type{type},
        __key{key},
        __args_len{len}
    {}

    Request(RequestType type, int key, std::string *args):
        __type{type},
        __key{key}
    {
        __args = reinterpret_cast<char*>(args);
        
    }

    ~Request(){}

    void destroy_multi_partition_scan(){
        delete[] reinterpret_cast<scan_data_t*>(__args)->values;
        delete[] reinterpret_cast<scan_data_t*>(__args)->key_to_addr;
        delete reinterpret_cast<scan_data_t*>(__args);
    }

    void destroy_write(){
        delete reinterpret_cast<std::string*>(__args);
    }

    Request(Request& other) {
        __type = other.__type;
        __key = other.__key;
        __args_len = other.__args_len;
        if (__type == WRITE){
            __args = new char[__args_len + 1];
            strcpy(__args, other.__args);
        }
    }
    Request * no_value_copy(){
        return new Request(__type, __key, __args_len);
    }
    Request(Request&& other) = default;

    inline RequestType type() const {return __type;}
    inline int key() const {return __key;}
    inline char* args() const {return __args;}
    inline const size_t & args_len() const {return __args_len;}

    inline void init_barrier(size_t n){
        __args = reinterpret_cast<char*>(new pthread_barrier_t());
        pthread_barrier_init(reinterpret_cast<pthread_barrier_t*>(__args), NULL, n);
    }

    inline void destroy_barrier(){
        pthread_barrier_destroy(reinterpret_cast<pthread_barrier_t*>(__args));
        delete reinterpret_cast<pthread_barrier_t*>(__args);
    }

    inline int barrier_wait(){
        return pthread_barrier_wait(reinterpret_cast<pthread_barrier_t*>(__args));
    }

    inline void init_scan_data(){
        __args = new char[sizeof(scan_data_t)];
        reinterpret_cast<scan_data_t*>(__args)->key_to_addr = new char*[__args_len];
        reinterpret_cast<scan_data_t*>(__args)->values = new std::string*[__args_len];
    }

    inline void init_coordination(int involved_partitions){
        reinterpret_cast<scan_data_t*>(__args)->counter.store(involved_partitions, std::memory_order_relaxed);
    }

    inline void set_single_partition(){}

    inline bool is_multi_partition(){
        return __args != nullptr;
    }

    inline char** get_key_to_addr(){
        return reinterpret_cast<scan_data_t*>(__args)->key_to_addr;
    }
    template<typename PartitionT>
    inline void set_key_to_partition(size_t &idx, PartitionT* &p_addr){
        reinterpret_cast<scan_data_t*>(__args)->key_to_addr[idx] = reinterpret_cast<char*>(p_addr);
    }
    template<typename PartitionT>
    inline bool key_in_partition(size_t &idx, PartitionT* p_addr){
        return reinterpret_cast<scan_data_t*>(__args)->key_to_addr[idx] == reinterpret_cast<char*>(p_addr);
    }

    inline std::string*& get_scanned_values(){
        return *reinterpret_cast<scan_data_t*>(__args)->values;
    }

    inline bool is_coordinator(){
        return 1 == reinterpret_cast<scan_data_t*>(__args)->counter.fetch_add(-1);
    }

    inline void barrier(pthread_barrier_t* barrier){
        __args = reinterpret_cast<char*>(barrier);
    }

    inline const std::string& get_write_value(){
        return *reinterpret_cast<std::string*>(__args);
    }

private:
    RequestType __type;
    int __key;
    size_t __args_len;
    char* __args = nullptr;
};

    void read_request(Request* &request, std::ifstream &file);
}

#endif
