#ifndef _KVPAXOS_ASYNC_SCHEDULER_H_
#define _KVPAXOS_ASYNC_SCHEDULER_H_


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
#include <assert.h>
#include "graph/graph.hpp"
#include "graph/partitioning.h"
#include "partition.hpp"
#include "request/request.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include "scheduler.hpp"
#include "free_scheduler.hpp"
#include "utils/utils.h"


namespace kvpaxos {


template <typename T, size_t TL = 0, size_t WorkerCapacity = 0, interval_type IntervalType = interval_type::OPERATIONS>
class AsyncScheduler : public FreeScheduler<T, TL, WorkerCapacity, IntervalType> {

public:

    AsyncScheduler() {}
    AsyncScheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        if constexpr(IntervalType == interval_type::MICROSECONDS){
            this->time_start_ = std::chrono::system_clock::now();
            this->time_interval_ = std::chrono::microseconds(repartition_interval);
        } else if constexpr(IntervalType == interval_type::OPERATIONS){
            operation_start_ = 0;
            this->operation_interval_ = repartition_interval;
        }
        this->n_partitions_ = n_partitions;
        this->repartition_method_ = repartition_method;

        this->round_robin_counter_ = 0;
        this->sync_counter_ = 0;
        this->n_dispatched_requests_ = 0;

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            this->partitions_.emplace(i, partition);
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();
        this->updated_data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        repartitioning_ = false;
        this->update_.store(false, std::memory_order_seq_cst);
        this->repartition_.store(false, std::memory_order_seq_cst);


        sem_init(&this->graph_requests_semaphore_, 0, 0);
        this->graph_thread_ = std::thread(&AsyncScheduler<T, TL, WorkerCapacity, IntervalType>::update_graph_loop, this);
	    utils::set_affinity(3, this->graph_thread_, this->graph_cpu_set);

        sem_init(&this->repart_semaphore_, 0, 0);
        this->reparting_thread_ = std::thread(&FreeScheduler<T, TL, WorkerCapacity, IntervalType>::partitioning_loop, this);
	    utils::set_affinity(4, this->reparting_thread_, this->reparting_cpu_set);

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
        Scheduler<T, TL, WorkerCapacity, IntervalType>::dispatch(request);
        this->n_dispatched_requests_++;

        if (this->repartition_method_ != model::ROUND_ROBIN) {

            if (repartitioning_ == false){
                bool interval_achieved;
                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    interval_achieved = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - this->time_start_) >= this->time_interval_;
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    interval_achieved = this->n_dispatched_requests_ - operation_start_ >= this->operation_interval_;
                }
                if (interval_achieved) {
                    repartition_.store(true, std::memory_order_release);
                }
                repartitioning_ = true;
            } else if(this->update_.load(std::memory_order_acquire) == true){
                FreeScheduler<T, TL, WorkerCapacity, IntervalType>::change_partition_scheme();
                this->repartition_apply_timestamp_.push_back(std::chrono::system_clock::now());

                this->update_.store(false, std::memory_order_relaxed);
                repartitioning_ = false;

                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    this->time_start_ = std::chrono::system_clock::now();
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    operation_start_ = this->n_dispatched_requests_;
                }
                
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

            if(repartition_.load(std::memory_order_acquire) == true) {
                repartition_.store(false, std::memory_order_relaxed);

                if(this->workload_graph_.n_vertex() > 0){
                    FreeScheduler<T, TL, WorkerCapacity, IntervalType>::order_partitioning();
                }
            } 

        }
    }

    std::atomic_bool repartition_;
    int repartitioning_;
    int operation_start_ = 0;

};

};


#endif
