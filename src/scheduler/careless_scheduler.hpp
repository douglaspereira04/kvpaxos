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

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T>(i);
            this->partitions_.emplace(i, partition);
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T>*>();
        this->updated_data_to_partition_ = new std::unordered_map<T, Partition<T>*>();

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&this->repartition_barrier_, NULL, 2);
        this->graph_thread_ = std::thread(&Scheduler<T>::update_graph_loop, this);
        
        sem_init(&this->schedule_semaphore_, 0, 0);
        sem_init(&this->update_semaphore_, 0, 0);
        sem_init(&this->continue_reparting_semaphore_, 0, 0);
    }
    
    void schedule_and_answer(struct client_message& request) {
        
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
                this->repartition_notify_timestamp_.push_back(std::chrono::system_clock::now());
                Scheduler<T>::store_q_sizes(this->q_size_repartition_begin_);
            }
        }
    }

public:

};

};


#endif
