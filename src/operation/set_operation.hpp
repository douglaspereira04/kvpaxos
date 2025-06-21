#ifndef WORKLOAD_SET_OPERATION_H
#define WORKLOAD_SET_OPERATION_H

#include <string>
#include "operation.hpp"


namespace workload {

template<typename T>
class SetOperation: public Operation<T> {
public:
    SetOperation(){}

    SetOperation(T &key)
        : Operation<T>(SET, key){}

    SetOperation(T &key, std::string *value)
        : Operation<T>(SET, key){
        __value = value;
    }

    ~SetOperation(){
        delete __value;
    }

    inline std::string& value() const {return *__value;}

protected:
    std::string* __value;
};

}

#endif
