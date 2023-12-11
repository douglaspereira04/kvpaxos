#ifndef _KVPAXOS_NON_STOP_SCHEDULER_H_
#define _KVPAXOS_NON_STOP_SCHEDULER_H_


#include <condition_variable>
#include <memory>
#include <netinet/tcp.h>
#include <pthread.h>
#include <queue>
#include <deque>
#include <semaphore.h>
#include <shared_mutex>
#include <string>
#include <string.h>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <atomic>

#include "graph/graph.hpp"
#include "graph/partitioning.h"
#include "partition.hpp"
#include "request/request.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include "scheduler.hpp"
#include "free_scheduler.hpp"


namespace kvpaxos {


template <typename T, size_t TL = 0, size_t WorkerCapacity = 0>
class NonStopScheduler : public FreeScheduler<T, TL, WorkerCapacity> {

public:

    NonStopScheduler() {}
    NonStopScheduler(int n_requests,
                int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        this->n_partitions_ = n_partitions;
        this->repartition_interval_ = repartition_interval;
        this->repartition_method_ = repartition_method;

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            this->partitions_.emplace(i, partition);
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();
        this->updated_data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&this->repartition_barrier_, NULL, 2);
        this->graph_thread_ = std::thread(&NonStopScheduler<T, TL, WorkerCapacity>::update_graph_loop, this);
        
        sem_init(&this->repart_semaphore_, 0, 0);
        sem_init(&this->schedule_semaphore_, 0, 0);
        sem_init(&this->update_semaphore_, 0, 0);
        sem_init(&this->updated_semaphore_, 0, 0);
        this->reparting_thread_ = std::thread(&NonStopScheduler<T, TL, WorkerCapacity>::partitioning_loop, this);
        reparting_ = false;

        client_message dummy;
        dummy.type = DUMMY;

        if constexpr(TL > 0){
            for (size_t i = 0; i < TL; i++)
            {
                this->graph_deletion_queue_.push_back(dummy);
            }
        }

    }
    
    void schedule_and_answer(struct client_message& request) {
         Scheduler<T, TL, WorkerCapacity>::dispatch(request);

        if (this->repartition_method_ != model::ROUND_ROBIN) {

            if(sem_trywait(&this->update_semaphore_) == 0){
                FreeScheduler<T, TL, WorkerCapacity>::change_partition_scheme();
                reparting_ = false;
            }
        }
    }



    void update_graph_loop() {
        while(true) {
            sem_wait(&this->graph_requests_semaphore_);
            this->graph_requests_mutex_.lock();
                auto request = std::move(this->graph_requests_queue_.front());
                this->graph_requests_queue_.pop_front();
            this->graph_requests_mutex_.unlock();

            Scheduler<T, TL, WorkerCapacity>::update_graph(request);

            if constexpr(TL > 0){
                this->graph_deletion_queue_.push_back(request);

                auto expired_request = std::move(this->graph_deletion_queue_.front());
                this->graph_deletion_queue_.pop_front();
                Scheduler<T, TL, WorkerCapacity>::expire(expired_request);
            }

            if(!reparting_) {
                reparting_ = true;
                FreeScheduler<T, TL, WorkerCapacity>::order_partitioning();
            } 

        }
    }

    void partitioning_loop(){
        while(true){
            sem_wait(&this->repart_semaphore_);
            
            delete this->updated_data_to_partition_;

            this->input_graph_mutex_.lock();
                auto temp = Scheduler<T, TL, WorkerCapacity>::partitioning(this->input_graph_);
            this->input_graph_mutex_.unlock();
            
            this->updated_data_to_partition_ = temp;
            sem_post(&this->update_semaphore_);
        }
    }

public:

    std::atomic_bool reparting_;

};

};


#endif
