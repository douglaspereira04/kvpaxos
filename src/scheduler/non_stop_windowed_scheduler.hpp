#ifndef _KVPAXOS_NON_STOP_WINDOWED_SCHEDULER_H_
#define _KVPAXOS_NON_STOP_WINDOWED_SCHEDULER_H_


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
#include "non_stop_scheduler.hpp"


namespace kvpaxos {


template <typename T, size_t TL, size_t WorkerCapacity = 0>/*TL = Track Length*/
class NonStopWindowedScheduler : public NonStopScheduler<T, WorkerCapacity> {

public:

    NonStopWindowedScheduler() {}
    NonStopWindowedScheduler(int n_requests,
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
        this->graph_thread_ = std::thread(&NonStopWindowedScheduler<T, TL, WorkerCapacity>::update_graph_loop, this);
        
        sem_init(&this->repart_semaphore_, 0, 0);
        sem_init(&this->schedule_semaphore_, 0, 0);
        sem_init(&this->update_semaphore_, 0, 0);
        sem_init(&this->updated_semaphore_, 0, 0);
        this->reparting_thread_ = std::thread(&FreeScheduler<T, WorkerCapacity>::partitioning_loop, this);
        this->reparting_ = false;

        client_message dummy;
        dummy.type = DUMMY;
        for (size_t i = 0; i < TL; i++)
        {
            graph_deletion_queue_.push_back(dummy);
        }

    }



    void update_graph_loop() {
        while(true) {
            sem_wait(&this->graph_requests_semaphore_);
            this->graph_requests_mutex_.lock();
                auto request = std::move(this->graph_requests_queue_.front());
                this->graph_requests_queue_.pop_front();
            this->graph_requests_mutex_.unlock();

            if(request.type != DUMMY){
                Scheduler<T, WorkerCapacity>::update_graph(request);
                this->graph_deletion_queue_.push_back(request);

                auto expired_request = std::move(this->graph_deletion_queue_.front());
                this->graph_deletion_queue_.pop_front();
                NonStopWindowedScheduler<T, TL, WorkerCapacity>::expire(expired_request);
            }

            if(!this->reparting_) {
                this->reparting_ = true;
                FreeScheduler<T, WorkerCapacity>::order_partitioning();
            }


        }
    }



    void expire(const client_message& message) {
        std::vector<int> data{message.key};
        size_t data_size = 1;
        if (message.type == SCAN) {
            data_size == std::stoi(message.args);
        }

        for (auto i = 0; i < data_size; i++) {
            this->workload_graph_.increase_vertice_weight(message.key+i, -1);

            if (this->workload_graph_.vertice_weight(message.key+i) == 0) {
                this->workload_graph_.remove_vertice(message.key+i);
            }

            for (auto j = i+1; j < data_size; j++) {

                this->workload_graph_.increase_edge_weight(message.key+i, message.key+j, -1);
                if(this->workload_graph_.edge_weight(message.key+i, message.key+j) == 0){
                    this->workload_graph_.remove_edge(message.key+i, message.key+j);
                }
            }
        }
    }

public:
    std::deque<struct client_message> graph_deletion_queue_;

};

};


#endif
