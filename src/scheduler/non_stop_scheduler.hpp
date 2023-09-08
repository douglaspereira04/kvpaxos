#ifndef _KVPAXOS_NON_STOP_SCHEDULER_H_
#define _KVPAXOS_NON_STOP_SCHEDULER_H_


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
class NonStopScheduler : public FreeScheduler<T> {

public:

    NonStopScheduler(int n_requests,
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

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&this->repartition_barrier_, NULL, 2);
        this->graph_thread_ = std::thread(&Scheduler<T>::update_graph_loop, this);
        
        sem_init(&this->repart_semaphore_, 0, 0);
        sem_init(&this->schedule_semaphore_, 0, 0);
        sem_init(&this->update_semaphore_, 0, 0);
        this->reparting_thread_ = std::thread(&NonStopScheduler<T>::reparting_loop, this);
        reparting_ = false;
        
    }
    
    void schedule_and_answer(struct client_message& request) {
        
        auto type = static_cast<request_type>(request.type);
        if (type == SYNC) {
            return;
        }

        if (type == WRITE) {
            if (not Scheduler<T>::mapped(request.key)) {
                FreeScheduler<T>::add_key(request.key);
            }
        }

        auto partitions = std::move(Scheduler<T>::involved_partitions(request));
        if (partitions.empty()) {
            request.type = ERROR;
            this->partitions_.at(0)->push_request(request);
        }else{
            auto arbitrary_partition = *begin(partitions);
            if (partitions.size() > 1) {
                Scheduler<T>::sync_partitions(partitions);
                arbitrary_partition->push_request(request);
                Scheduler<T>::sync_partitions(partitions);
            } else {
                arbitrary_partition->push_request(request);
            }
        }

        if (this->repartition_method_ != model::ROUND_ROBIN) {
            this->graph_requests_mutex_.lock();
                this->graph_requests_queue_.push(request);
            this->graph_requests_mutex_.unlock();
            sem_post(&this->graph_requests_semaphore_);

            this->n_dispatched_requests_++;

            if(sem_trywait(&this->update_semaphore_) == 0){
                FreeScheduler<T>::change_partition_scheme();
                reparting_ = false;
            } else if(!reparting_) {
                this->store_keys_ = true;
                sem_post(&this->repart_semaphore_);
                reparting_ = true;
            }
        }
    }

public:

    void reparting_loop(){
        while(true){
            sem_wait(&this->repart_semaphore_);
            delete this->input_graph_;

            this->graph_requests_mutex_.lock();
                this->input_graph_ = new InputGraph<T>(this->workload_graph_);
            this->graph_requests_mutex_.unlock();

            auto temp = FreeScheduler<T>::repart(this->input_graph_);
            
            this->updated_data_to_partition_ = temp;

            auto end_timestamp = std::chrono::system_clock::now();
            this->repartition_end_timestamps_.push_back(end_timestamp);
            this->graph_copy_duration_.push_back(std::chrono::nanoseconds::zero());
            sem_post(&this->update_semaphore_);
        }
    }

public:

    bool reparting_;

};

};


#endif
