#ifndef WORKLOAD_CALLBACK_OPERATION_H
#define WORKLOAD_CALLBACK_OPERATION_H
#include "operation.hpp"

namespace workload {

template<typename T>
class CallbackOperation: public Operation<T>{
public:
    CallbackOperation(){}

    CallbackOperation(OperationType type, T key, void (*callback_function)(T key, std::string*))
        : Operation<T>(type, key){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    CallbackOperation(OperationType type, T key, size_t len, void (*callback_function)(T key, std::string*))
        : Operation<T>(type, key, len){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    CallbackOperation(OperationType type, T key, std::string *value, void (*callback_function)(T key, std::string*))
        : Operation<T>(type, key, value){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    CallbackOperation(OperationType type, T key, void (*callback_function)(T key))
        : Operation<T>(type, key){
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