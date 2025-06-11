#ifndef WORKLOAD_CALLBACK_REQUEST_H
#define WORKLOAD_CALLBACK_REQUEST_H
#include "request.hpp"

namespace workload {

template<typename T>
class CallbackRequest: public Request<T>{
public:
    CallbackRequest(){}

    CallbackRequest(OperationType type, T key, void (*callback_function)(T key, std::string*))
        : Request<T>(type, key){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    CallbackRequest(OperationType type, T key, size_t len, void (*callback_function)(T key, std::string*))
        : Request<T>(type, key, len){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    CallbackRequest(OperationType type, T key, std::string *value, void (*callback_function)(T key, std::string*))
        : Request<T>(type, key, value){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    CallbackRequest(OperationType type, T key, void (*callback_function)(T key))
        : Request<T>(type, key){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    inline void callback_function(std::string* value){
        reinterpret_cast<void (*)(T, std::string*)>(__callback_function)(this->__key, value);
    }

    inline void callback_function(){
        reinterpret_cast<void (*)(T)>(__callback_function)(this->__key);
    }
private:
    void *__callback_function;
};

}

#endif