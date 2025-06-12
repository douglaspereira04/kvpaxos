#ifndef MODEL_QUEUE_H
#define MODEL_QUEUE_H


#include <queue>
#include <mutex>
#include <semaphore.h>
#include <type_traits>
#include "tbb/concurrent_queue.h"


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

    Queue(size_t ahead_0, size_t ahead_1){
        semaphores[0] = sem_t();
        semaphores[1] = sem_t();
        sem_init(&semaphores[0], 0, 0);
        sem_init(&semaphores[1], 0, 0);

        sem_init(&ahead_sems[0], 0, ahead_0);
        sem_init(&ahead_sems[1], 0, ahead_1);
        
        
    }
    ~Queue(){}

    void push(T t_value, U u_value){
        t_queue.push(t_value);
        u_queue.push(u_value);
        sem_post(&semaphores[0]);
        sem_post(&semaphores[1]);
    }

    template<typename TorU>
    void pop(TorU &curr_value){
        if constexpr(std::is_same_v<TorU, T>){
            sem_wait(&semaphores[1]);
            t_queue.try_pop(curr_value);
            sem_post(&ahead_sems[1]);
        } else if constexpr(std::is_same_v<TorU, U>){
            sem_wait(&semaphores[0]);
            u_queue.try_pop(curr_value);
            sem_post(&ahead_sems[0]);
        }
    }


private:
    sem_t semaphores[2];

    sem_t ahead_sems[2];

    tbb::concurrent_queue<T> t_queue;
    tbb::concurrent_queue<U> u_queue;
};

}


#endif
