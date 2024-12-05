#ifndef MODEL_QUEUE_H
#define MODEL_QUEUE_H


#include <algorithm>
#include <map>
#include <queue>
#include <atomic>
#include <mutex>
#include <semaphore.h>


namespace model {

template <typename T>
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

        sem_init(&ahead_sems[0], 0, ahead_1);
        sem_init(&ahead_sems[1], 0, ahead_0);

        queues[0] = std::queue<T>();
        queues[1] = std::queue<T>();
        q_mutex[0] = new std::mutex();
        q_mutex[1] = new std::mutex();
        
        
    }
    ~Queue(){}

    void push(T &value){
        q_mutex[0]->lock();
        queues[0].push(value);
        q_mutex[0]->unlock();

        q_mutex[1]->lock();
        queues[1].push(value);
        q_mutex[1]->unlock();
    }

    template <size_t Head>
    void notify(){
        sem_post(&semaphores[Head]);
    }

    template <size_t Head>
    void free(){
        sem_post(&semaphores[Head]);
        sem_post(&ahead_sems[Head]);
    }

    template <size_t Head>
    void wait(){
        sem_wait(&semaphores[Head]);
        sem_wait(&ahead_sems[Head]);
    }

    template <size_t Head>
    T pop(){
        q_mutex[Head]->lock();
        T curr_value = std::move(queues[Head].front());
        queues[Head].pop();
        q_mutex[Head]->unlock();
        sem_post(&ahead_sems[(Head+1)%2]);
        return curr_value;
    }

    template <size_t Head>
    bool is_ahead(){
        int vals[2];
        if(sem_getvalue(&semaphores[0], &vals[0]) != 0){
            return false;
        }
        if(sem_getvalue(&semaphores[1], &vals[1]) != 0){
            return false;
        }
        if constexpr(Head == 0){
            return vals[0] < vals[1];
        }
        return vals[1] < vals[0];
    }


private:
    sem_t semaphores[2];

    sem_t ahead_sems[2];

    std::queue<T> queues[2];

    std::mutex* q_mutex[2];
};

}


#endif
