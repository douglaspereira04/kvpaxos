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

template <typename T>
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
        
    }

    void push(T &value){
        model::Node<T>* new_node = new model::Node<T>(value);
        tail->next = new_node;
        tail = new_node;
        sem_post(&semaphores[0]);
        sem_post(&semaphores[1]);
        
    }

    template <size_t Head>
    T pop(){
        sem_wait(&semaphores[Head]);
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
        return curr_value;
    }


private:
    model::Node<T>* tail = nullptr;
    model::Node<T>* heads[2];
    model::Node<T>* free_head;
    sem_t semaphores[2];
};

}


#endif
