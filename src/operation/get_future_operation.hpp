#ifndef WORKLOAD_GET_FUTURE_OPERATION_H
#define WORKLOAD_GET_FUTURE_OPERATION_H

#include <string>
#include "get_operation.hpp"
#include "utils.h"
#include "absl/synchronization/notification.h"


namespace workload {

template<typename T>
class GetFutureOperation: public GetOperation<T> {
public:
    GetFutureOperation(){}

    GetFutureOperation(T &key, std::string *value){
        this->__type = GET_FUTURE;
        this->__key = &key;
        this->__value = value;
    }

    ~GetFutureOperation(){}

    inline void wait(){
        __sem.WaitForNotification();
    }

    inline void notify(){
        __sem.Notify();
    }

    std::string *value(){
        return __value;
    }
protected:
    absl::Notification __sem;
    std::string* __value;
};

}

#endif
