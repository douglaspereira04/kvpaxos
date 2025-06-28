#ifndef MODEL_QUEUE_H
#define MODEL_QUEUE_H


#include <queue>
#include <mutex>
#include <type_traits>
#include "tbb/concurrent_queue.h"
#include <semaphore>


namespace model {

template <typename T>
class Queue {
    

public:
    Queue()
        : __sem(0)
    {}

    inline void push(T &value){
        __queue.push(value);
        __sem.release();
    }

    inline void pop(T &curr_value){
        __sem.acquire();
        __queue.try_pop(curr_value);
    }


private:
    std::counting_semaphore<SEM_VALUE_MAX> __sem;

    tbb::concurrent_queue<T> __queue;
};

}


#endif
