#ifndef MODEL_QUEUE_H
#define MODEL_QUEUE_H


#include <queue>
#include <mutex>
#include <type_traits>
#include "tbb/concurrent_queue.h"
#include <semaphore>


namespace model {

template <typename T, typename U>
class Queue {
    

public:
    Queue(){}

    Queue(size_t distance)
    : Queue(
        static_cast<size_t>(distance/2), 
        static_cast<size_t>(distance/2+(distance & 1))
    ){}

    Queue(size_t ahead_0, size_t ahead_1)
        : semaphores_0(0),
        semaphores_1(0),
        ahead_sems_0(ahead_0),
        ahead_sems_1(ahead_1)
    {}

    ~Queue(){}

    void push(T t_value, U u_value){
        t_queue.push(t_value);
        u_queue.push(u_value);
        semaphores_0.release();
        semaphores_1.release();
    }

    template<typename TorU>
    void pop(TorU &curr_value){
        if constexpr(std::is_same_v<TorU, T>){
            semaphores_1.acquire();
            ahead_sems_0.acquire();
            t_queue.try_pop(curr_value);
            ahead_sems_1.release();
        } else if constexpr(std::is_same_v<TorU, U>){
            semaphores_0.acquire();
            ahead_sems_1.acquire();
            u_queue.try_pop(curr_value);
            ahead_sems_0.release();
        }
    }


private:
    std::counting_semaphore<SEM_VALUE_MAX> semaphores_0, semaphores_1;

    std::counting_semaphore<SEM_VALUE_MAX> ahead_sems_0, ahead_sems_1;

    tbb::concurrent_queue<T> t_queue;
    tbb::concurrent_queue<U> u_queue;
};

}


#endif
