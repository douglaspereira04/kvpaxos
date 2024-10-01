#ifndef MODEL_LINKED_QUEUE_H
#define MODEL_LINKED_QUEUE_H


#include <algorithm>
#include <map>
#include <vector>
#include <atomic>
#include <semaphore.h>


namespace model {

template <typename T>
struct  Node {
    T value;
    std::atomic_bool passed;
    Node<T>* next;
    Node(){}
    Node(T &value_): value{value_}{
        passed.store(false, std::memory_order_relaxed);
    }
};

template <typename T, size_t Window>
class LinkedQueue {
    

public:
    LinkedQueue(){
        model::Node<T>* sentinel = new model::Node<T>();
        semaphores[0] = sem_t();
        semaphores[1] = sem_t();
        heads[0] = sentinel;
        heads[1] = sentinel;
        free_head = sentinel;
        tail = sentinel;
        sem_init(&semaphores[0], 0, 0);
        sem_init(&semaphores[1], 0, 0);

        sem_init(&ahead_sems[0], 0, Window);
        sem_init(&ahead_sems[1], 0, Window);
        
        
    }

    void push(T &value){
        model::Node<T>* new_node = new model::Node<T>(value);
        tail->next = new_node;
        tail = new_node;
        
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
        model::Node<T>* head = heads[Head]->next;
        T curr_value = head->value;
        heads[Head] = head;

        if (head->passed.load(std::memory_order_acquire) == true){
            while(free_head != head){
                model::Node<T>* next = free_head->next;
                delete free_head;
                free_head = next;
            }
        } else {
            head->passed.store(true, std::memory_order_release);
        }
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
            return vals[0] <= vals[1];
        }
        return vals[1] <= vals[0];
    }


private:
    model::Node<T>* tail = nullptr;
    model::Node<T>* heads[2];
    model::Node<T>* free_head;
    sem_t semaphores[2];

    sem_t ahead_sems[2];
};

}


#endif
