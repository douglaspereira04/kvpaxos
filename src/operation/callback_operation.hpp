#ifndef WORKLOAD_CALLBACK_OPERATION_H
#define WORKLOAD_CALLBACK_OPERATION_H

#include <string>

namespace workload {

template<typename T>
class CallbackOperation{
public:
    CallbackOperation(){}

    inline void __set_callback(void (*callback_function)(T key, std::string* value)){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }

    inline void __set_callback(void (*callback_function)(T key)){
        __callback_function = reinterpret_cast<void*>(callback_function);
    }
    
    inline void callback(std::string* value);

    inline void callback();
protected:
    void *__callback_function;
};

}

#endif