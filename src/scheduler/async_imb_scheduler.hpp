#ifndef _KVPAXOS_ASYNC_IMB_SCHEDULER_H_
#define _KVPAXOS_ASYNC_IMB_SCHEDULER_H_


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
#include "linked_queue/linked_queue.hpp"


namespace kvpaxos {


template <typename T, size_t TL = 0, size_t WorkerCapacity = 0, interval_type IntervalType = interval_type::OPERATIONS, size_t MaxSucessiveImbalances = 5>
class AsyncImbScheduler : public FreeScheduler<T, TL, WorkerCapacity, IntervalType> {

public:

    AsyncImbScheduler() {}
    AsyncImbScheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method,
                size_t queue_head_distance,
                float balance_threshold
    ) {
        //this->prev_executed = 0;
        //this->prev_throughput_ = 0;
        this->n_partitions_ = n_partitions;
        this->sucessive_imbalance_ = new uint32_t[n_partitions];
        this->in_queue_amount_ = new size_t[this->n_partitions_];
        this->scheduling_queue_ = model::LinkedQueue<client_message>(queue_head_distance);
        if constexpr(IntervalType == interval_type::MICROSECONDS){
            this->time_start_ = utils::now();
            this->time_interval_ = std::chrono::microseconds(repartition_interval);
        } else if constexpr(IntervalType == interval_type::OPERATIONS){
            this->operation_start_ = 0;
            this->operation_interval_ = repartition_interval;
        }
        this->repartition_method_ = repartition_method;

        this->round_robin_counter_ = 0;
        this->sync_counter_ = 0;
        this->n_dispatched_requests_ = 0;

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            this->partitions_.emplace(i, partition);
            this->sucessive_imbalance_[i] = 0b1;
        }
        this->data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();
        this->updated_data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        this->repartitioning_.store(false, std::memory_order_seq_cst);
        this->update_.store(false, std::memory_order_seq_cst);
        this->repartition_.store(false, std::memory_order_seq_cst);

        this->scheduling_thread_ = std::thread(&AsyncImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::scheduling_loop, this);
        utils::set_affinity(2,this->scheduling_thread_, this->scheduler_cpu_set_);

        this->graph_thread_ = std::thread(&AsyncImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::update_graph_loop, this);
	    utils::set_affinity(3, this->graph_thread_, this->graph_cpu_set_);

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

    inline void clear_imbalance_count(){
        for (int i = 0; i < this->n_partitions_; i++) {
            this->sucessive_imbalance_[i] = 0b1;
        }
    }

    bool imbalance() const{
        size_t sum = 0;
        int i = 0;
        for (auto& kv: this->partitions_) {
            auto* partition = kv.second;
            size_t si = partition->request_queue_size();
            this->in_queue_amount_[i] = si;
            sum += si;
            i++;
        }

        float avg = static_cast<float>(sum)/this->n_partitions_;
        float threshold = avg * this->balance_threshold_;
        for (i = 0; i < this->n_partitions_; i++) {
            if (std::abs(this->in_queue_amount_[i] - avg) > threshold){
                this->sucessive_imbalance_[i] = this->sucessive_imbalance_[i] << 1;
                if (this->sucessive_imbalance_[i] & (0b1 << MaxSucessiveImbalances)){
                    return true;
                }
            } else {
                this->sucessive_imbalance_[i] = (this->sucessive_imbalance_[i] >> 1) | 0b1;
            }
        }
        return false;

    }

    void set_balance_threshold(float balance_threshold){
        this->balance_threshold_ = balance_threshold;
    }

    void scheduling_loop() {
        while(true){
            this->scheduling_queue_.template wait<0>();
            client_message message = this->scheduling_queue_.template pop<0>();
            if (message.type == END){
                break;
            }
            AsyncImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::schedule_and_answer(message);
        }
        
        this->schedule_end_ = utils::now();
    }

    void schedule_and_answer(struct client_message& request) {
        Scheduler<T, TL, WorkerCapacity, IntervalType>::dispatch(request);
        this->n_dispatched_requests_++;

        if (this->repartition_method_ != model::ROUND_ROBIN) {

            if(this->update_.load(std::memory_order_acquire) == true){
                FreeScheduler<T, TL, WorkerCapacity, IntervalType>::change_partition_scheme();

                if constexpr(utils::ENABLE_INFO){
                    this->repartition_apply_timestamp_.push_back(utils::now());
                }
                this->update_.store(false, std::memory_order_relaxed);


                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    this->time_start_ = utils::now();
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    this->operation_start_ = this->n_dispatched_requests_;
                }
                this->repartitioning_.store(false, std::memory_order_release);
                
            }
        }
    }

    void update_graph_loop() {
        while(true) {
            this->scheduling_queue_.template wait<1>();
            client_message request = this->scheduling_queue_.template pop<1>();

            Scheduler<T, TL, WorkerCapacity, IntervalType>::update_graph(request);

            if constexpr(TL > 0){
                this->graph_deletion_queue_.push_back(request);

                auto expired_request = std::move(this->graph_deletion_queue_.front());
                this->graph_deletion_queue_.pop_front();
                Scheduler<T, TL, WorkerCapacity, IntervalType>::expire(expired_request);
            }
            n_processed_requests_++;

            if(!this->repartitioning_.load(std::memory_order_acquire)){
                bool interval_achieved;
                time_point now_ = utils::now();
                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    interval_achieved = utils::to_us(now_ - this->time_start_) >= this->time_interval_;
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    interval_achieved = this->n_processed_requests_ - operation_start_ >= this->operation_interval_;
                }
                if (interval_achieved) {
                    //size_t executed = Scheduler<T, TL, WorkerCapacity, IntervalType>::n_executed_requests();
                    //now_ = utils::now();
                    bool repartition = AsyncImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::imbalance();

                    //float throughput = (executed - prev_executed)/utils::to_us(now_ - this->time_start_);
                    //this->prev_throughput_ = throughput;
                    //prev_executed = executed;
                    if (repartition) {
                        this->repartitioning_.store(true, std::memory_order_relaxed);
                        if(this->workload_graph_.n_vertex() > 0){
                            FreeScheduler<T, TL, WorkerCapacity, IntervalType>::order_partitioning();
                            AsyncImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::clear_imbalance_count();
                        }
                    } else {
                        if constexpr(IntervalType == interval_type::MICROSECONDS){
                            this->time_start_ = utils::now();
                        } else if constexpr(IntervalType == interval_type::OPERATIONS){
                            this->operation_start_ = this->n_processed_requests_;
                        }
                    }
                }
            }

        }
    }
    size_t n_processed_requests_ = 0;
    std::atomic_bool repartition_;
    int operation_start_ = 0;

    float balance_threshold_;
    size_t *in_queue_amount_;

    size_t prev_executed;
    float prev_throughput_;

    uint32_t* sucessive_imbalance_;
};

};


#endif
