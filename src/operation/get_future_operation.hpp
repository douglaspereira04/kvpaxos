#ifndef WORKLOAD_GET_FUTURE_OPERATION_H
#define WORKLOAD_GET_FUTURE_OPERATION_H

#include <string>
#include "get_operation.hpp"
#include "utils.h"
#include "semaphore.h"


namespace workload {

template<typename T>
class GetFutureOperation: public GetOperation<T> {
public:
    GetFutureOperation(){}

    GetFutureOperation(T &key, std::string *value){
        this->__type = GET_FUTURE;
        this->__key = key;
        this->__value = value;
        sem_init(&__sem, 0, 0);
    }

    ~GetFutureOperation(){
        sem_destroy(&__sem);
    }

    inline void wait(){
        sem_wait(&__sem);
    }

    inline void notify(){
        sem_post(&__sem);
    }

    std::string *value(){
        return __value;
    }
protected:
    sem_t __sem;
    std::string* __value;
};

}

#endif
