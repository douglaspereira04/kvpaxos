#ifndef WORKLOAD_GET_OPERATION_H
#define WORKLOAD_GET_OPERATION_H

#include "operation.hpp"


namespace workload {


template<typename T>
class GetOperation: public Operation<T> {
public:
    GetOperation(){}

    GetOperation(T key)
        : Operation<T>(GET, key){}

};

}

#endif
