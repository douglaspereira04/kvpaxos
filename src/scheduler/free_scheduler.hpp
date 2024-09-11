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
#include "utils/utils.h"


namespace kvpaxos {

template <typename T, size_t TL = 0, size_t WorkerCapacity = 0, interval_type IntervalType = interval_type::OPERATIONS>
class FreeScheduler : public Scheduler<T, TL, WorkerCapacity, IntervalType> {

public:

    FreeScheduler() {}
    FreeScheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        this->n_partitions_ = n_partitions;
        this->repartition_method_ = repartition_method;

        if constexpr(IntervalType == interval_type::MICROSECONDS){
            this->time_start_ = std::chrono::system_clock::now();
            this->time_interval_ = std::chrono::microseconds(repartition_interval);
        } else if constexpr(IntervalType == interval_type::OPERATIONS){
            this->operation_interval_ = repartition_interval;
        }
        this->round_robin_counter_ = 0;
        this->sync_counter_ = 0;
        this->n_dispatched_requests_ = 0;

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            this->partitions_.emplace(i, partition);
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();
        updated_data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        this->repartitioning_.store(false, std::memory_order_seq_cst);
        this->update_.store(false, std::memory_order_seq_cst);

        this->scheduling_thread_ = std::thread(&FreeScheduler<T, TL, WorkerCapacity, IntervalType>::scheduling_loop, this);
        utils::set_affinity(2,this->scheduling_thread_, this->scheduler_cpu_set_);

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        this->graph_thread_ = std::thread(&FreeScheduler<T, TL, WorkerCapacity, IntervalType>::update_graph_loop, this);
	    utils::set_affinity(3, this->graph_thread_, this->graph_cpu_set_);

        sem_init(&repart_semaphore_, 0, 0);
        reparting_thread_ = std::thread(&FreeScheduler<T, TL, WorkerCapacity, IntervalType>::partitioning_loop, this);
	    utils::set_affinity(4, reparting_thread_, reparting_cpu_set);

        client_message dummy;
        dummy.type = DUMMY;

        if constexpr(TL > 0){
            for (size_t i = 0; i < TL; i++)
            {
                this->graph_deletion_queue_.push_back(dummy);
            }
        }

    }

    void scheduling_loop() {
        while(true){
            client_message message = this->scheduling_queue_.template pop<0>();
            if (message.type == END){
                break;
            }
            FreeScheduler<T, TL, WorkerCapacity, IntervalType>::schedule_and_answer(message);
        }
        
        this->schedule_end_ = std::chrono::system_clock::now();
    }

    void schedule_and_answer(struct client_message& request) {
        Scheduler<T, TL, WorkerCapacity, IntervalType>::dispatch(request);
        this->n_dispatched_requests_++;

        if (this->repartition_method_ != model::ROUND_ROBIN) {

            if(update_.load(std::memory_order_acquire) == true){
                FreeScheduler<T, TL, WorkerCapacity, IntervalType>::change_partition_scheme();
                this->repartition_apply_timestamp_.push_back(std::chrono::system_clock::now());

                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    this->time_start_ = std::chrono::system_clock::now();
                }

                update_.store(false, std::memory_order_release);
                repartitioning_.store(false, std::memory_order_release);
            }
        }
    }

public:

    void change_partition_scheme(){
        std::unordered_map<T,kvpaxos::Partition<T, WorkerCapacity>*> *temp =  this->data_to_partition_;
        this->data_to_partition_ = updated_data_to_partition_;
        updated_data_to_partition_ = temp;

        Scheduler<T, TL, WorkerCapacity, IntervalType>::sync_all_partitions();
    }

    void order_partitioning(){
        auto begin = std::chrono::system_clock::now();
        input_graph_ = InputGraph<T>(this->workload_graph_);
        this->graph_copy_duration_.push_back(std::chrono::system_clock::now() - begin);

        this->repartition_request_timestamp_.push_back(std::chrono::system_clock::now());
        sem_post(&repart_semaphore_);
    }

    void update_graph_loop() {
        while(true) {
            client_message request = this->scheduling_queue_.template pop<1>();
    
            Scheduler<T, TL, WorkerCapacity, IntervalType>::update_graph(request);

            if constexpr(TL > 0){
                this->graph_deletion_queue_.push_back(request);

                auto expired_request = std::move(this->graph_deletion_queue_.front());
                this->graph_deletion_queue_.pop_front();
                Scheduler<T, TL, WorkerCapacity, IntervalType>::expire(expired_request);
            }
            n_processed_requests++;

            if(repartitioning_.load(std::memory_order_acquire) == false){
                bool start_repartitioning = false;
                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    auto interval = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - this->time_start_);
                    if(interval >= this->time_interval_){
                        start_repartitioning = true;
                    }
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    start_repartitioning = this->n_dispatched_requests_ % this->operation_interval_ == 0;
                }
                if (start_repartitioning) {
                    if(this->workload_graph_.n_vertex() > 0){
                        repartitioning_.store(true, std::memory_order_release);
                        FreeScheduler<T, TL, WorkerCapacity, IntervalType>::order_partitioning();
                    }
                }
            }
        }
    }

    void partitioning_loop(){
        while(true){
            sem_wait(&repart_semaphore_);

            delete updated_data_to_partition_;

            auto temp = Scheduler<T, TL, WorkerCapacity, IntervalType>::partitioning(input_graph_);

            updated_data_to_partition_ = temp;
            update_.store(true, std::memory_order_release);
        }
    }

public:

    std::unordered_map<T, Partition<T, WorkerCapacity>*>* updated_data_to_partition_;
    InputGraph<T> input_graph_;

    int n_processed_requests = 0;

    sem_t repart_semaphore_;

    std::thread reparting_thread_;
    cpu_set_t reparting_cpu_set;


    std::atomic_bool repartitioning_;
    std::atomic_bool update_;
};

};


#endif
