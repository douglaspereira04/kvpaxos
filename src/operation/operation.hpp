#ifndef WORKLOAD_OPERATION_H
#define WORKLOAD_OPERATION_H

#include <string>
#include <cstring>
#include <pthread.h>
#include <atomic>

#include "utils.h"


namespace workload {

typedef unsigned int OperationType;

const OperationType GET =            1 << 2;
const OperationType GET_CALLBACK =  (1 << 2) | 1;
const OperationType GET_FUTURE =    (1 << 2) | 2;
const OperationType SET =            2 << 2;
const OperationType SET_CALLBACK =  (2 << 2) | 1;
const OperationType SET_FUTURE =    (2 << 2) | 2;
const OperationType SCAN =           3 << 2;
const OperationType SCAN_CALLBACK = (3 << 2) | 1;
const OperationType SCAN_FUTURE =   (3 << 2) | 2;
const OperationType DEL =            4 << 2;
const OperationType DEL_CALLBACK =  (4 << 2) | 1;
const OperationType DEL_FUTURE =    (4 << 2) | 2;
const OperationType REPARTITION =    5 << 2;
const OperationType END =            6 << 2;
const OperationType DUMMY =          7 << 2;


template<OperationType TYPE>
inline constexpr OperationType generic_type() {
    return TYPE & ~0b11;
}

inline OperationType generic_type(OperationType type) {
    return type & ~0b11;
}

template<typename T>
class Operation {
public:
    Operation(){}

    Operation(OperationType type):
        __type{type}
    {}

    Operation(OperationType type, T key):
        __type{type},
        __key{key}
    {}

    ~Operation(){}


    inline OperationType type() const {return __type;}
    inline bool is_scan() const {
        return generic_type(__type) == SCAN;
    }

    inline T key() const {return __key;}

    template<typename Storage_T>
    inline void storage(Storage_T *storage_){
        __storage = reinterpret_cast<char**>(storage_);
    }
    
    template<typename Storage_T>
    inline Storage_T* storage(){
        return reinterpret_cast<Storage_T*>(__storage);
    }

    template<typename Storage_T>
    inline void storage(size_t idx, Storage_T *storage_){
        __storage[idx] = reinterpret_cast<char*>(storage_);
    }

    template<typename Storage_T>
    inline Storage_T* storage(size_t idx){
        return reinterpret_cast<Storage_T**>(__storage)[idx];
    }

protected:
    OperationType __type;
    T __key;
    char**__storage;
};

}

#endif
