#ifndef WORKLOAD_SCAN_FUTURE_OPERATION_H
#define WORKLOAD_SCAN_FUTURE_OPERATION_H

#include <string>
#include <vector>
#include "scan_operation.hpp"
#include "utils.h"
#include "absl/synchronization/notification.h"


namespace workload {

template<typename T>
class ScanFutureOperation: public ScanOperation<T> {
public:
    ScanFutureOperation(){}

    ScanFutureOperation(T &key, size_t len, std::string* values)
        : ScanOperation<T>(key, len, values){
            this->__type = SCAN_FUTURE;
        }

    ~ScanFutureOperation(){}

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
