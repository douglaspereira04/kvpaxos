#ifndef WORKLOAD_DEL_OPERATION_H
#define WORKLOAD_DEL_OPERATION_H

#include "operation.hpp"


namespace workload {

template<typename T>
class DelOperation: public Operation<T> {
public:
    DelOperation(){}

    DelOperation(T key)
        : Operation<T>(DEL, key){}


};

}

#endif
