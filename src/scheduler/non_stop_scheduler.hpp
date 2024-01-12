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
#include <assert.h>
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
        this->updated_data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        reparting_.store(false, std::memory_order_seq_cst);
        update_.store(false, std::memory_order_seq_cst);

        //create cpu set for threads
        cpu_set_t graph_cpu_set;
        cpu_set_t part_cpu_set;
        CPU_ZERO(&graph_cpu_set);
        CPU_ZERO(&part_cpu_set);
        CPU_SET(2, &graph_cpu_set);
        CPU_SET(3, &part_cpu_set);

        sem_init(&this->graph_requests_semaphore_, 0, 0);
        this->graph_thread_ = std::thread(&NonStopScheduler<T, TL, WorkerCapacity>::update_graph_loop, this);

        //assign graph thread affinity
        assert(pthread_setaffinity_np(this->graph_thread_.native_handle(), sizeof(cpu_set_t), &graph_cpu_set) == 0);

        sem_init(&this->repart_semaphore_, 0, 0);
        this->reparting_thread_ = std::thread(&NonStopScheduler<T, TL, WorkerCapacity>::partitioning_loop, this);

        //assign partition thread affinity
        assert(pthread_setaffinity_np(this->reparting_thread_.native_handle(), sizeof(cpu_set_t), &part_cpu_set) == 0);

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

            if(update_.load(std::memory_order_acquire) == true){
                FreeScheduler<T, TL, WorkerCapacity>::change_partition_scheme();
                this->repartition_apply_timestamp_.push_back(std::chrono::system_clock::now());

                update_.store(false, std::memory_order_release);
                reparting_.store(false, std::memory_order_release);
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

            if(reparting_.load(std::memory_order_acquire) == false) {
                if(this->workload_graph_.n_vertex() > 0){
                    reparting_.store(true, std::memory_order_release);
                    NonStopScheduler<T, TL, WorkerCapacity>::order_partitioning();
                }
            } 

        }
    }

    void order_partitioning(){
        auto begin = std::chrono::system_clock::now();
        this->input_graph_ = InputGraph<T>(this->workload_graph_);
        this->graph_copy_duration_.push_back(std::chrono::system_clock::now() - begin);

        this->repartition_request_timestamp_.push_back(std::chrono::system_clock::now());
        sem_post(&this->repart_semaphore_);
    }

    void partitioning_loop(){
        while(true){
            sem_wait(&this->repart_semaphore_);

            delete this->updated_data_to_partition_;

            auto temp = Scheduler<T, TL, WorkerCapacity>::partitioning(this->input_graph_);

            this->updated_data_to_partition_ = temp;
            update_.store(true, std::memory_order_release);
        }
    }

public:

    std::atomic_bool reparting_;
    std::atomic_bool update_;

};

};


#endif
