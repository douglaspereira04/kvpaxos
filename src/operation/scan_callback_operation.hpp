#ifndef WORKLOAD_SCAN_CALLBACK_OPERATION_H
#define WORKLOAD_SCAN_CALLBACK_OPERATION_H

#include <string>
#include "scan_operation.hpp"
#include "callback_operation.hpp"


namespace workload {

template<typename T>
class ScanCallbackOperation: public ScanOperation<T>, public CallbackOperation<T> {
public:
    ScanCallbackOperation(){}


    ScanCallbackOperation(T key, size_t len, void (*callback_function)(T key, std::string* value)){
        this->__type = SCAN_CALLBACK;
        this->__key = key;
        this->__len = len;
        CallbackOperation<T>::__set_callback(callback_function);
    }


    inline void callback(std::string* value){
        reinterpret_cast<void (*)(T, std::string*)>(this->__callback_function)(this->__key, value);
    }

    inline void callback(){
        reinterpret_cast<void (*)(T)>(this->__callback_function)(this->__key);
    }
};

}

#endif
