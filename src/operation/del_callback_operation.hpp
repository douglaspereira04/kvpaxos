#ifndef WORKLOAD_DEL_CALLBACK_OPERATION_H
#define WORKLOAD_DEL_CALLBACK_OPERATION_H

#include <string>
#include "del_operation.hpp"
#include "callback_operation.hpp"


namespace workload {

template<typename T>
class DelCallbackOperation: public DelOperation<T>, public CallbackOperation<T> {
public:
    DelCallbackOperation(){}

    DelCallbackOperation(T &key, void (*callback_function)(T &key)){
        this->__type = DEL_CALLBACK;
        this->__key = new T(key);
        CallbackOperation<T>::__set_callback(callback_function);
    }


    inline void callback(std::string* value){
        reinterpret_cast<void (*)(T&, std::string*)>(this->__callback_function)(*this->__key, value);
    }

    inline void callback(){
        reinterpret_cast<void (*)(T&)>(this->__callback_function)(*this->__key);
    }
};

}

#endif
