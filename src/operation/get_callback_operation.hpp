#ifndef WORKLOAD_GET_CALLBACK_OPERATION_H
#define WORKLOAD_GET_CALLBACK_OPERATION_H

#include <string>
#include "get_operation.hpp"
#include "callback_operation.hpp"


namespace workload {

template<typename T>
class GetCallbackOperation: public GetOperation<T>, public CallbackOperation<T> {
public:
    GetCallbackOperation(){}

    GetCallbackOperation(T key, void (*callback_function)(T key, std::string* value)){
        this->__type = GET_CALLBACK;
        this->__key = key;
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
