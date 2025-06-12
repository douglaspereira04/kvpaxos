#ifndef WORKLOAD_REPARTITION_OPERATION_H
#define WORKLOAD_REPARTITION_OPERATION_H

#include <pthread.h>
#include "operation.hpp"


namespace workload {


template<typename T>
class RepartitionOperation: public Operation<T> {
public:
    template<typename Storage_T>
    RepartitionOperation(size_t partitions, Storage_T* storages) : Operation<T>(REPARTITION){
        pthread_barrier_init(&__barrier, NULL, partitions);
        this->template storage<Storage_T>(storages);
    }

    ~RepartitionOperation(){
        pthread_barrier_destroy(&__barrier);
    }

    inline int barrier_wait(){
        return pthread_barrier_wait(&__barrier);
    }

protected:
    pthread_barrier_t __barrier;
};

}

#endif
