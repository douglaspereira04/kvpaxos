#ifndef WORKLOAD_SET_CALLBACK_OPERATION_H
#define WORKLOAD_SET_CALLBACK_OPERATION_H

#include <string>
#include "set_operation.hpp"
#include "callback_operation.hpp"


namespace workload {

template<typename T>
class SetCallbackOperation: public SetOperation<T>, public CallbackOperation<T> {
public:
    SetCallbackOperation(){}

    SetCallbackOperation(T &key, std::string* value, void (*callback_function)(T &key, std::string* value)){
        this->__type = SET_CALLBACK;
        this->__key = key;
        this->__value = value;
        CallbackOperation<T>::__set_callback(callback_function);
    }

    inline void callback(std::string* value){
        reinterpret_cast<void (*)(T&, std::string*)>(this->__callback_function)(this->__key, value);
    }

    inline void callback(){
        reinterpret_cast<void (*)(T&)>(this->__callback_function)(this->__key);
    }
};

}

#endif
