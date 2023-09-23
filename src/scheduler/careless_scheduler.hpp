#ifndef _KVPAXOS_CARELESS_SCHEDULER_H_
#define _KVPAXOS_CARELESS_SCHEDULER_H_


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

#include "graph/graph.hpp"
#include "graph/partitioning.h"
#include "partition.hpp"
#include "request/request.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include "scheduler.hpp"
#include "free_scheduler.hpp"


namespace kvpaxos {


template <typename T>
class CarelessScheduler : public FreeScheduler<T> {

public:

    CarelessScheduler(int n_requests,
                int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        this->n_partitions_ = n_partitions;
        this->repartition_interval_ = repartition_interval;
        this->repartition_method_ = repartition_method;
        this->store_keys_ = false;

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T>(i);
            this->partitions_.emplace(i, partition);
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T>*>();
        this->updated_data_to_partition_ = new std::unordered_map<T, Partition<T>*>();

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&this->repartition_barrier_, NULL, 2);
        this->graph_thread_ = std::thread(&Scheduler<T>::update_graph_loop, this);
        
        sem_init(&this->repart_semaphore_, 0, 0);
        sem_init(&this->schedule_semaphore_, 0, 0);
        sem_init(&this->update_semaphore_, 0, 0);
        sem_init(&this->continue_reparting_semaphore_, 0, 0);
        this->reparting_thread_ = std::thread(&CarelessScheduler<T>::reparting_loop, this);
        
    }
    
    void schedule_and_answer(struct client_message& request) {

        auto type = static_cast<request_type>(request.type);
        if (this->store_keys_ && type == WRITE && !Scheduler<T>::mapped(request.key)){
            auto partition_id = this->round_robin_counter_;
            this->pending_keys_.push_back(std::make_pair(request.key, partition_id));
        }
        
        Scheduler<T>::dispatch(request);

        if (this->repartition_method_ != model::ROUND_ROBIN) {
            if(sem_trywait(&this->update_semaphore_) == 0){
                FreeScheduler<T>::change_partition_scheme();

                Scheduler<T>::store_q_sizes(this->q_size_repartition_end_);
                
                sem_post(&this->continue_reparting_semaphore_);
            }
            
            if(
                this->n_dispatched_requests_ % this->repartition_interval_ == 0
            ) {
                Scheduler<T>::store_q_sizes(this->q_size_repartition_begin_);

                this->store_keys_ = true;
                sem_post(&this->repart_semaphore_);
            }
        }
    }

public:

    void reparting_loop(){
        while(true){
            sem_wait(&this->repart_semaphore_);
            
            auto begin = std::chrono::system_clock::now();

            delete this->input_graph_;
            this->input_graph_ = new InputGraph<T>(this->workload_graph_);

            this->graph_copy_duration_.push_back(std::chrono::system_clock::now() - begin);

            auto temp = FreeScheduler<T>::repart(this->input_graph_);

            delete this->updated_data_to_partition_;
            this->updated_data_to_partition_ = temp;

            auto end_timestamp = std::chrono::system_clock::now();
            this->repartition_end_timestamps_.push_back(end_timestamp);
            this->graph_copy_duration_.push_back(std::chrono::nanoseconds::zero());
            sem_post(&this->update_semaphore_);

            sem_wait(&this->continue_reparting_semaphore_);
        }
    }

};

};


#endif
