#ifndef WORKLOAD_REQUEST_H
#define WORKLOAD_REQUEST_H

#include <string>
#include <unordered_set>

#include "types/types.h"
#include <fstream>


namespace workload {
using namespace std;

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

    Request(RequestType type, int key, const string &args):
        __type{type},
        __key{key},
        __args_len{args.length()}
    {
        __args = new char[args.length() + 1];
        strcpy(__args, args.c_str());
        
    }

    ~Request(){
        if (__type == WRITE){
            delete[] __args;
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

    inline void barrier(pthread_barrier_t* barrier){
        __args = reinterpret_cast<char*>(barrier);
    }

    inline pthread_barrier_t* barrier(){
        return reinterpret_cast<pthread_barrier_t*>(__args);
    }

private:
    RequestType __type;
    int __key;
    size_t __args_len;
    char* __args = nullptr;
};

    void read_request(Request* &request, ifstream &file);
}

#endif
