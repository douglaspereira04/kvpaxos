#ifndef _KVPAXOS_OLD_IMB_SCHEDULER_H_
#define _KVPAXOS_OLD_IMB_SCHEDULER_H_


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

template <typename T, size_t TL = 0, size_t WorkerCapacity = 0, interval_type IntervalType = interval_type::OPERATIONS, size_t MaxSucessiveImbalances = 100>
class OldImbScheduler : public Scheduler<T, TL, WorkerCapacity, IntervalType> {

public:

    OldImbScheduler() {}
    OldImbScheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method,
                size_t queue_head_distance
    ) {
        this->n_partitions_ = n_partitions;
        this->repartition_method_ = repartition_method;
        

        this->sucessive_imbalance_ = new uint32_t[n_partitions];
        this->in_queue_amount_ = new size_t[this->n_partitions_];
        this->scheduling_queue_ = model::Queue<client_message>(queue_head_distance);
        if constexpr(IntervalType == interval_type::MICROSECONDS){
            this->time_start_ = utils::now();
            this->time_interval_ = std::chrono::microseconds(repartition_interval);
        } else if constexpr(IntervalType == interval_type::OPERATIONS){
            this->operation_start_ = 0;
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
        OldImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::clear_imbalance_count();

        pthread_barrier_init(&this->repartition_barrier_, NULL, 2);

        this->scheduling_thread_ = std::thread(&OldImbScheduler<T, TL, WorkerCapacity, IntervalType>::scheduling_loop, this);
        utils::set_affinity(2,this->scheduling_thread_, this->scheduler_cpu_set_);

        this->graph_thread_ = std::thread(&Scheduler<T, TL, WorkerCapacity, IntervalType>::update_graph_loop, this);
	    utils::set_affinity(3, this->graph_thread_, this->graph_cpu_set_);

        this->note_ = false;

        client_message dummy;
        dummy.type = DUMMY;

        if constexpr(TL > 0){
            for (size_t i = 0; i < TL; i++)
            {
                this->graph_deletion_queue_.push_back(dummy);
            }
        }

    }

    inline void clear_imbalance_count() const{
        for (int i = 0; i < this->n_partitions_; i++) {
            this->sucessive_imbalance_[i] = 0;//0b1;
        }
    }

    bool imbalance() const{
        bool imbalance = false;

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
                this->sucessive_imbalance_[i] = this->sucessive_imbalance_[i] + 1; //<< 1;
                if (this->sucessive_imbalance_[i] > MaxSucessiveImbalances){//& (0b1 << MaxSucessiveImbalances)){
                    imbalance = true;
                    OldImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::clear_imbalance_count();
                    break;
                }
            } else {
                this->sucessive_imbalance_[i] = 0; // (this->sucessive_imbalance_[i] >> 1) | 0b1;
            }
        }

        return imbalance;
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
            OldImbScheduler<T, TL, WorkerCapacity, IntervalType>::schedule_and_answer(message);
        }
        
        this->schedule_end_ = utils::now();
    }

    void schedule_and_answer(struct client_message& request) {
        Scheduler<T, TL, WorkerCapacity, IntervalType>::dispatch(request);
        this->n_dispatched_requests_++;

        if (this->repartition_method_ != model::ROUND_ROBIN) {
            bool interval_achieved;
            time_point now_ = utils::now();
            if constexpr(IntervalType == interval_type::MICROSECONDS){
                interval_achieved = utils::to_us(now_ - this->time_start_) >= this->time_interval_;
            } else if constexpr(IntervalType == interval_type::OPERATIONS){
                interval_achieved = this->n_dispatched_requests_ - this->operation_start_ >= this->operation_interval_;
            }
            if (interval_achieved) {
                bool start_repartitioning = OldImbScheduler<T, TL, WorkerCapacity, IntervalType, MaxSucessiveImbalances>::imbalance();

                if (start_repartitioning) {
                    if constexpr(utils::ENABLE_INFO){
                        this->repartition_request_timestamp_.push_back(utils::now());
                    }
                    
                    this->note_ = true;
                    this->scheduling_queue_.template free<1>();
                    pthread_barrier_wait(&this->repartition_barrier_);

                    time_point begin;
                    if constexpr(utils::ENABLE_INFO){
                        begin = utils::now();
                    }
                    auto input_graph = InputGraph<T>(this->workload_graph_);
                    pthread_barrier_wait(&this->repartition_barrier_);


                    if constexpr(utils::ENABLE_INFO){
                        this->graph_copy_duration_.push_back(utils::now() - begin);
                    }

                    auto temp = Scheduler<T, TL, WorkerCapacity, IntervalType>::partitioning(input_graph);

                    delete this->data_to_partition_;
                    this->data_to_partition_ = temp;

                    Scheduler<T, TL, WorkerCapacity, IntervalType>::sync_all_partitions();

                    if constexpr(utils::ENABLE_INFO){
                        this->repartition_apply_timestamp_.push_back(utils::now());
                    }
                }
                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    this->time_start_ = utils::now();
                } else if constexpr(IntervalType == interval_type::OPERATIONS){
                    this->operation_start_ = this->n_dispatched_requests_;
                }
            }
        }
    }


public:
    int operation_start_ = 0;

    float balance_threshold_;
    size_t *in_queue_amount_;
    uint32_t* sucessive_imbalance_;
};

};


#endif
