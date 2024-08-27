#ifndef _KVPAXOS_BATCH_SCHEDULER_H_
#define _KVPAXOS_BATCH_SCHEDULER_H_


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
#include "utils/utils.h"


namespace kvpaxos {


template <typename T, size_t TL = 0, size_t WorkerCapacity = 0, interval_type IntervalType = interval_type::MICROSECONDS>
class BatchScheduler : public Scheduler<T, TL, WorkerCapacity, IntervalType> {

public:

    BatchScheduler() {}
    BatchScheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) {
        if constexpr(IntervalType == interval_type::MICROSECONDS){
            this->time_interval_ = std::chrono::microseconds(repartition_interval);
        }
        static_assert(IntervalType == interval_type::MICROSECONDS, "Não pensei com ops");

        this->n_partitions_ = n_partitions;
        this->repartition_method_ = repartition_method;

        this->round_robin_counter_ = 0;
        this->n_dispatched_requests_ = 0;

        
        this->data_to_partition_ = nullptr;

        this->graph_thread_ = std::thread(&BatchScheduler<T, TL, WorkerCapacity, IntervalType>::update_graph_loop, this);
	    utils::set_affinity(3, this->graph_thread_, this->graph_cpu_set);

        for (auto i = 0; i < this->n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            this->partitions_.emplace(i, partition);
        }
    }

    void process_populate_request(struct client_message& request) {
        Partition<T, WorkerCapacity>::populate_storage(request);
    }

    std::unordered_set<Partition<T, WorkerCapacity>*> involved_partitions(
        const struct client_message& request)
    {
        std::unordered_set<Partition<T, WorkerCapacity>*> partitions;
        auto type = static_cast<request_type>(request.type);

        auto range = 1;
        if (type == SCAN) {
            range = std::stoi(request.args);
        }

        for (auto i = 0; i < range; i++) {
            partitions.insert(this->data_to_partition_->at(request.key + i));
        }

        return partitions;
    }

    void dispatch(struct client_message& request){

        auto partitions = std::move(BatchScheduler<T, TL, WorkerCapacity, IntervalType>::involved_partitions(request));
        auto arbitrary_partition = *begin(partitions);
        if (partitions.size() > 1) {
            Scheduler<T, TL, WorkerCapacity, IntervalType>::sync_partitions(partitions);
            arbitrary_partition->push_request(request);
            Scheduler<T, TL, WorkerCapacity, IntervalType>::sync_partitions(partitions);
        } else {
            arbitrary_partition->push_request(request);
        }
    }

    void schedule_and_answer(struct client_message& request) {
        this->graph_requests_mutex_.lock();
        Scheduler<T, TL, WorkerCapacity, IntervalType>::update_graph(request);
        requests_batch_.push_back(request);
        this->n_dispatched_requests_++;
        this->graph_requests_mutex_.unlock();
    }

    void update_graph_loop() {
        std::chrono::_V2::system_clock::time_point begin;
        if constexpr(IntervalType == interval_type::MICROSECONDS){
            begin = std::chrono::high_resolution_clock::now();
        } 
        while(true){
            if constexpr(IntervalType == interval_type::MICROSECONDS){
                auto now = std::chrono::high_resolution_clock::now();
                while(now < begin + this->time_interval_){now = std::chrono::high_resolution_clock::now();}
                begin = now;
            } 
            static_assert(IntervalType == interval_type::MICROSECONDS, "Não pensei com ops");
            this->graph_requests_mutex_.lock();
            if(requests_batch_.size() > 0){
                if(requests_batch_.size() > 1){
                    auto input_graph = InputGraph<T>(this->workload_graph_);
                    this->data_to_partition_ = Scheduler<T, TL, WorkerCapacity, IntervalType>::partitioning(input_graph);
                    this->workload_graph_ = model::Graph<T>();
                    for(client_message request: requests_batch_){
                        BatchScheduler<T, TL, WorkerCapacity, IntervalType>::dispatch(request);
                    }
                    delete this->data_to_partition_;
                } else{
                    this->partitions_.begin()->second->push_request(requests_batch_.front());
                }
                Scheduler<T, TL, WorkerCapacity, IntervalType>::sync_all_partitions();
                requests_batch_.clear();
            }
            this->graph_requests_mutex_.unlock();
        }
    }

    std::vector<client_message> requests_batch_;

    int operation_start_;

};

};


#endif
