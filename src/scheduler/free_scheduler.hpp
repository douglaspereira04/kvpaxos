#ifndef _KVPAXOS_FREE_SCHEDULER_H_
#define _KVPAXOS_FREE_SCHEDULER_H_


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

#include "input_graph.hpp"
#include "graph/graph.hpp"
#include "graph/partitioning.h"
#include "partition.hpp"
#include "request/request.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include "scheduler.hpp"


namespace kvpaxos {

template <typename T, size_t TL = 0, size_t WorkerCapacity = 0>
class FreeScheduler : public Scheduler<T, TL, WorkerCapacity> {

public:

    FreeScheduler() {}
    FreeScheduler(int n_requests,
                int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        this->round_robin_counter_ = 0;
        this->sync_counter_ = 0;
        this->n_dispatched_requests_ = 0;
        
        this->n_partitions_ = n_partitions;
        this->repartition_interval_ = repartition_interval;
        this->repartition_method_ = repartition_method;

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            this->partitions_.emplace(i, partition);
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();
        updated_data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        this->graph_thread_ = std::thread(&FreeScheduler<T, TL, WorkerCapacity>::update_graph_loop, this);
        
        sem_init(&repart_semaphore_, 0, 0);
        sem_init(&update_semaphore_, 0, 0);
        sem_init(&updated_semaphore_, 0, 0);
        reparting_thread_ = std::thread(&FreeScheduler<T, TL, WorkerCapacity>::partitioning_loop, this);

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
        this->n_dispatched_requests_++;

        if (this->repartition_method_ != model::ROUND_ROBIN) {

            if(sem_trywait(&update_semaphore_) == 0){
                FreeScheduler<T, TL, WorkerCapacity>::change_partition_scheme();

                this->repartition_apply_timestamp_.push_back(std::chrono::system_clock::now());
                sem_post(&updated_semaphore_);
            }
        }
    }

public:

    void change_partition_scheme(){
        std::unordered_map<T,kvpaxos::Partition<T, WorkerCapacity>*> *temp =  this->data_to_partition_;
        this->data_to_partition_ = updated_data_to_partition_;
        updated_data_to_partition_ = temp;

        Scheduler<T, TL, WorkerCapacity>::sync_all_partitions();
    }

    void order_partitioning(){
        auto begin = std::chrono::system_clock::now();
        input_graph_mutex_.lock();
            input_graph_ = new InputGraph<T>(this->workload_graph_);
        input_graph_mutex_.unlock();
        this->graph_copy_duration_.push_back(std::chrono::system_clock::now() - begin);

        this->repartition_request_timestamp_.push_back(std::chrono::system_clock::now());
        sem_post(&repart_semaphore_);
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
            n_processed_requests++;

            if( n_processed_requests % this->repartition_interval_ == 0 ) {
                FreeScheduler<T, TL, WorkerCapacity>::order_partitioning();
            }
        }
    }

    void partitioning_loop(){
        while(true){
            sem_wait(&repart_semaphore_);

            delete updated_data_to_partition_;

            input_graph_mutex_.lock();
                auto temp = Scheduler<T, TL, WorkerCapacity>::partitioning(input_graph_);
                delete input_graph_;
            input_graph_mutex_.unlock();

            updated_data_to_partition_ = temp;
            sem_post(&update_semaphore_);

            sem_wait(&updated_semaphore_);
        }
    }

public:

    std::unordered_map<T, Partition<T, WorkerCapacity>*>* updated_data_to_partition_;
    InputGraph<T> *input_graph_;

    int n_processed_requests = 0;

    sem_t repart_semaphore_;
    sem_t update_semaphore_;
    sem_t updated_semaphore_;

    std::mutex input_graph_mutex_;

    std::thread reparting_thread_;

};

};


#endif
