#ifndef WORKLOAD_SCAN_FUTURE_OPERATION_H
#define WORKLOAD_SCAN_FUTURE_OPERATION_H

#include <string>
#include <vector>
#include "scan_operation.hpp"
#include "utils.h"
#include "semaphore.h"


namespace workload {

template<typename T>
class ScanFutureOperation: public ScanOperation<T> {
public:
    ScanFutureOperation(){}

    ScanFutureOperation(T key, size_t len, std::string *values){
        this->__type = SCAN_FUTURE;
        this->__key = key;
        this->__len = len;
        this->__values = values;
        sem_init(&__sem, 0, 0);
    }

    ~ScanFutureOperation(){
        sem_destroy(&__sem);
    }

    inline void wait(){
        sem_wait(&__sem);
    }

    inline void notify(){
        sem_post(&__sem);
    }
protected:
    sem_t __sem;
};

}

#endif
