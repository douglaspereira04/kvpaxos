#ifndef _KVPAXOS_FREE_SCHEDULER_H_
#define _KVPAXOS_FREE_SCHEDULER_H_


#include <condition_variable>
#include <memory>
#include <netinet/tcp.h>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <shared_mutex>
#include <string>
#include <string.h>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "input_graph.hpp"
#include "graph/graph.hpp"
#include "graph/partitioning.h"
#include "partition.hpp"
#include "request/request.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include "scheduler.hpp"


namespace kvpaxos {

template <typename T>
class FreeScheduler : public Scheduler<T> {

public:

    FreeScheduler() {}
    FreeScheduler(int n_requests,
                int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        this->n_partitions_ = n_partitions;
        this->repartition_interval_ = repartition_interval;
        this->repartition_method_ = repartition_method;

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T>(i);
            this->partitions_.emplace(i, partition);
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T>*>();
        updated_data_to_partition_ = new std::unordered_map<T, Partition<T>*>();

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&this->repartition_barrier_, NULL, 2);
        this->graph_thread_ = std::thread(&FreeScheduler<T>::update_graph_loop, this);
        
        sem_init(&repart_semaphore_, 0, 0);
        sem_init(&schedule_semaphore_, 0, 0);
        sem_init(&update_semaphore_, 0, 0);
        sem_init(&continue_reparting_semaphore_, 0, 0);
        reparting_thread_ = std::thread(&FreeScheduler<T>::partitioning_loop, this);
        
    }
    
    void schedule_and_answer(struct client_message& request) {
        
        Scheduler<T>::dispatch(request);

        if (this->repartition_method_ != model::ROUND_ROBIN) {

            if(sem_trywait(&update_semaphore_) == 0){
                FreeScheduler<T>::change_partition_scheme();
                
                Scheduler<T>::store_q_sizes(this->q_size_repartition_begin_);
                
                sem_post(&continue_reparting_semaphore_);
            }
            
            if(
                this->n_dispatched_requests_ % this->repartition_interval_ == 0
            ) {
                Scheduler<T>::store_q_sizes(this->q_size_repartition_end_);

                Scheduler<T>::notify_graph(REPART);
            }
        }
    }

public:

    void change_partition_scheme(){
        std::unordered_map<T,kvpaxos::Partition<T>*> *temp =  this->data_to_partition_;
        this->data_to_partition_ = updated_data_to_partition_;
        updated_data_to_partition_ = temp;
        
        Scheduler<T>::sync_all_partitions();
    }

    void update_graph_loop() {
        while(true) {
            sem_wait(&this->graph_requests_semaphore_);
            this->graph_requests_mutex_.lock();
                auto request = std::move(this->graph_requests_queue_.front());
                this->graph_requests_queue_.pop();
            this->graph_requests_mutex_.unlock();
            
            if (request.type == SYNC) {
                pthread_barrier_wait(&this->repartition_barrier_);
            }  else if (request.type == REPART) {

                auto begin = std::chrono::system_clock::now();
                input_graph_mutex_.lock();
                    delete input_graph_;
                    input_graph_ = new InputGraph<T>(this->workload_graph_);
                input_graph_mutex_.unlock();
                this->graph_copy_duration_.push_back(std::chrono::system_clock::now() - begin);

                sem_post(&repart_semaphore_);
            } else {
                Scheduler<T>::update_graph(request);
            }
        }
    }

    void partitioning_loop(){
        while(true){
            sem_wait(&repart_semaphore_);
            
            delete updated_data_to_partition_;

            input_graph_mutex_.lock();
                auto temp = repart(input_graph_);
            input_graph_mutex_.unlock();
            
            updated_data_to_partition_ = temp;

            auto end_timestamp = std::chrono::system_clock::now();
            this->repartition_end_timestamps_.push_back(end_timestamp);
            sem_post(&update_semaphore_);

            sem_wait(&continue_reparting_semaphore_);
        }
    }

    std::unordered_map<T, Partition<T>*>* repart(struct InputGraph<T>* graph) {
        auto start_timestamp = std::chrono::system_clock::now();
        this->repartition_timestamps_.emplace_back(start_timestamp);

        auto partition_scheme = std::move(
            model::multilevel_cut(
                graph->vertice_weight, 
                graph->x_edges, 
                graph->edges, 
                graph->edges_weight,
                this->partitions_.size(), 
                this->repartition_method_
            )
        );

        auto data_to_partition = new std::unordered_map<T, Partition<T>*>();
        
        for (auto& it : graph->vertice_to_pos) {
            T key = it.first;
            int position = it.second;
            //position indicates the position of the key in partition scheme
            int partition = partition_scheme[position];  
            if (partition >= this->n_partitions_) {
                printf("ERROR: partition was %d!\n", partition);
                fflush(stdout);
            }
            data_to_partition->emplace(key, this->partitions_.at(partition));
        }
        
        if (this->first_repartition) {
            this->first_repartition = false;
        }
        
        return data_to_partition;
    }
public:
    std::unordered_map<T, Partition<T>*>* updated_data_to_partition_;
    InputGraph<T> *input_graph_ = new InputGraph<T>();

    sem_t repart_semaphore_;
    sem_t schedule_semaphore_;
    sem_t update_semaphore_;
    sem_t continue_reparting_semaphore_;

    std::mutex input_graph_mutex_;
    std::mutex update_mutex_;

    std::thread reparting_thread_;
    
};

};


#endif
