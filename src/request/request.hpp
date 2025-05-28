#ifndef WORKLOAD_REQUEST_H
#define WORKLOAD_REQUEST_H

#include <string>
#include <unordered_set>
#include <pthread.h>

#include "types/types.h"
#include <fstream>


namespace workload {

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

    ~Request(){
        if (__type == SCAN || REPARTITION){
            delete[] __args;
        } else if (__type == WRITE){
            delete reinterpret_cast<std::string*>(__args);
        }
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
    inline size_t args_len() const {return __args_len;}

    inline void init_barrier(size_t n){
        pthread_barrier_init(reinterpret_cast<pthread_barrier_t*>(__args), NULL, n);
    }

    template<typename T>
    inline void init_scan_data(){
        __args = new char[
            sizeof(pthread_barrier_t)+
            (sizeof(size_t)*__args_len)+
            (sizeof(char*)*__args_len)+
            (sizeof(T*)*__args_len)
        ];
    }

    template<typename T>
    inline void get_key_to_partition(T &data){
        data = reinterpret_cast<T>(__args+
            sizeof(pthread_barrier_t)+
            (sizeof(size_t)*__args_len)+
            (sizeof(char*)*__args_len));
    }

    inline void get_values(char** &data){
        data = reinterpret_cast<char**>(__args+
            sizeof(pthread_barrier_t)+
            (sizeof(size_t)*__args_len));
    }

    inline void get_value_lengths(size_t* &data){
        data = reinterpret_cast<size_t*>(__args+
            sizeof(pthread_barrier_t));
    }

    inline pthread_barrier_t* barrier(){
        return reinterpret_cast<pthread_barrier_t*>(__args);
    }

    inline void barrier(pthread_barrier_t* barrier){
        __args = reinterpret_cast<char*>(barrier);
    }

    inline std::string *get_value_string(){
        return reinterpret_cast<std::string*>(__args);
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
