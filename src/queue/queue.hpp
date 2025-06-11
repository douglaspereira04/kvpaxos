#ifndef MODEL_QUEUE_H
#define MODEL_QUEUE_H


#include <queue>
#include <mutex>
#include <semaphore.h>
#include <type_traits>


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

        t_queue = std::queue<T>();
        u_queue = std::queue<U>();
        q_mutex[0] = new std::mutex();
        q_mutex[1] = new std::mutex();
        
        
    }
    ~Queue(){}

    void push(T t_value, U u_value){
        q_mutex[0]->lock();
        t_queue.push(t_value);
        q_mutex[0]->unlock();

        q_mutex[1]->lock();
        u_queue.push(u_value);
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

    template<typename TorU>
    TorU pop(){
        if constexpr(std::is_same_v<TorU, T>){
            q_mutex[0]->lock();
                T curr_value = t_queue.front();
                t_queue.pop();
            q_mutex[0]->unlock();
            sem_post(&ahead_sems[1]);
            return curr_value;
        } else if constexpr(std::is_same_v<TorU, U>){
            q_mutex[1]->lock();
                U curr_value = u_queue.front();
                u_queue.pop();
            q_mutex[1]->unlock();
            sem_post(&ahead_sems[0]);
            return curr_value;
        } else {
            abort();
        }
    }


private:
    sem_t semaphores[2];

    sem_t ahead_sems[2];

    std::queue<T> t_queue;
    std::queue<U> u_queue;

    std::mutex* q_mutex[2];
};

}


#endif
