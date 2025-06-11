#ifndef WORKLOAD_OPERATION_H
#define WORKLOAD_OPERATION_H

#include <string>
#include <cstring>
#include <pthread.h>
#include <atomic>

#include "utils.h"


namespace workload {


enum OperationType
{
	GET = 0x1 << 1,
	GET_CALLBACK = (0x1 << 1) | 1,
	SET = 0x1 << 2,
	SET_CALLBACK = (0x1 << 2) | 1,
	SCAN = 0x1 << 3,
	SCAN_CALLBACK = (0x1 << 3) | 1,
	DEL = 0x1 << 4,
	DEL_CALLBACK = (0x1 << 4) | 1,
	REPARTITION = 0x1 << 5,
	END = 0x1 << 6,
	DUMMY = 0x1 << 7,
};

template<typename T>
class Operation {
public:
    Operation(){}

    Operation(OperationType type):
        __type{type}
    {}

    Operation(OperationType type, T key):
        __type{type},
        __key{key}
    {}

    ~Operation(){}


    inline OperationType type() const {return __type;}
    inline OperationType clean_type() const {
        return static_cast<OperationType>(
            static_cast<int>(__type) & ~0x1
        );
    }

    inline T key() const {return __key;}

protected:
    OperationType __type;
    T __key;
};

}

#endif
