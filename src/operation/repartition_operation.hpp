#ifndef WORKLOAD_REPARTITION_OPERATION_H
#define WORKLOAD_REPARTITION_OPERATION_H

#include <absl/synchronization/barrier.h>
#include "operation.hpp"


namespace workload {


template<typename T>
class RepartitionOperation: public Operation<T> {
public:
    template<typename Storage_T>
    RepartitionOperation(size_t partitions, Storage_T* storages) : Operation<T>(REPARTITION){
        __barrier = new absl::Barrier(partitions);
        this->template storage<Storage_T>(storages);
    }

    ~RepartitionOperation(){
        delete __barrier;
    }

    inline bool barrier_wait(){
        return __barrier->Block();
    }

protected:
    absl::Barrier* __barrier;
};

}

#endif
