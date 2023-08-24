#ifndef _KVPAXOS_COPY_SCHEDULER_H_
#define _KVPAXOS_COPY_SCHEDULER_H_


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


namespace kvpaxos {

template <typename T>
class CopyScheduler : Scheduler<T> {
public:

    CopyScheduler(int n_requests,
                int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) : n_partitions_{n_partitions},
        repartition_interval_{repartition_interval},
        repartition_method_{repartition_method}
    {
       std::cout << "COPY_SCHEDULER" << std::endl; 

        for (auto i = 0; i < n_partitions_; i++) {
            auto* partition = new Partition<T>(i);
            partitions_.emplace(i, partition);
        }
        data_to_partition_ = new std::unordered_map<T, Partition<T>*>();

        sem_init(&graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&repartition_barrier_, NULL, 2);
        graph_thread_ = std::thread(&CopyScheduler<T>::update_graph_loop, this);
    }

    void schedule_and_answer(struct client_message& request) {
        
        auto type = static_cast<request_type>(request.type);
        if (type == SYNC) {
            return;
        }

        if (type == WRITE) {
            if (not mapped(request.key)) {
                add_key(request.key);
            }
        }

        auto partitions = std::move(involved_partitions(request));
        if (partitions.empty()) {
            request.type = ERROR;
            return partitions_.at(0)->push_request(request);
        }

        auto arbitrary_partition = *begin(partitions);
        if (
    const std::vector<time_point>& repartition_timestamps() const {
        return repartition_timestamps_;
    }

    const std::vector<duration>& graph_copy_duration() const {
        return graph_copy_duration_;
    }
    const std::vector<duration>& repartition_duration() const {
        return repartition_duration_;
    }partitions.size() > 1) {
            sync_partitions(partitions);
            arbitrary_partition->push_request(request);
            sync_partitions(partitions);
        } else {
            arbitrary_partition->push_request(request);
        }

        if (repartition_method_ != model::ROUND_ROBIN) {
            graph_requests_mutex_.lock();
                graph_requests_queue_.push(request);
            graph_requests_mutex_.unlock();
            sem_post(&graph_requests_semaphore_);

            n_dispatched_requests_++;
            if (
                n_dispatched_requests_ % repartition_interval_ == 0
            ) {
                struct client_message sync_message;
                sync_message.type = SYNC;

                graph_requests_mutex_.lock();
                    graph_requests_queue_.push(sync_message);
                graph_requests_mutex_.unlock();
                sem_post(&graph_requests_semaphore_);

                pthread_barrier_wait(&repartition_barrier_);
                auto start_timestamp = std::chrono::system_clock::now();
                repartition_data();
                duration repartition_duration = std::chrono::system_clock::now()-start_timestamp;
                repartition_duration_.push_back(repartition_duration);
                graph_copy_duration_.push_back(std::chrono::nanoseconds::zero());
                sync_all_partitions();
            }
        }
    }

private:

    void add_key(T key) {
        auto partition_id = round_robin_counter_;
        data_to_partition_->emplace(key, partitions_.at(partition_id));

        round_robin_counter_ = (round_robin_counter_+1) % n_partitions_;

        if (repartition_method_ != model::ROUND_ROBIN) {
            struct client_message write_message;
            write_message.type = WRITE;
            write_message.key = key;
            write_message.s_addr = (unsigned long) partitions_.at(partition_id);
            write_message.sin_port = 1;

            graph_requests_mutex_.lock();
                graph_requests_queue_.push(write_message);
            graph_requests_mutex_.unlock();
            sem_post(&graph_requests_semaphore_);
        }
    }

    void update_graph_loop() {
        while(true) {
            sem_wait(&graph_requests_semaphore_);
            graph_requests_mutex_.lock();
                auto request = std::move(graph_requests_queue_.front());
                graph_requests_queue_.pop();
            graph_requests_mutex_.unlock();

            if (request.type == SYNC) {
                pthread_barrier_wait(&repartition_barrier_);
            } else {
                if (request.type == WRITE and request.sin_port == 1) {
                    auto partition = (Partition<T>*) request.s_addr;
		            data_to_partition_copy_.emplace(request.key, partition);
		            partition->insert_data(request.key);
                }
                update_graph(request);
            }
        }
    }

    void update_graph(const client_message& message) {
        std::vector<int> data{message.key};
        if (message.type == SCAN) {
            for (auto i = 1; i < std::stoi(message.args); i++) {
                data.emplace_back(message.key+i);
            }
        }

        for (auto i = 0; i < data.size(); i++) {
            if (not workload_graph_.vertice_exists(data[i])) {
                workload_graph_.add_vertice(data[i]);
            }

            workload_graph_.increase_vertice_weight(data[i]);
            data_to_partition_copy_.at(data[i])->increase_weight(data[i]);
            for (auto j = i+1; j < data.size(); j++) {
                if (not workload_graph_.vertice_exists(data[j])) {
                    workload_graph_.add_vertice(data[j]);
                }
                if (not workload_graph_.are_connected(data[i], data[j])) {
                    workload_graph_.add_edge(data[i], data[j]);
                }

                workload_graph_.increase_edge_weight(data[i], data[j]);
            }
        }
    }

    void repartition_data() {
        auto start_timestamp = std::chrono::system_clock::now();
        repartition_timestamps_.emplace_back(start_timestamp);

        auto partition_scheme = std::move(
            model::cut_graph(
                workload_graph_,
                partitions_,
                repartition_method_,
                *data_to_partition_,
                first_repartition
            )
        );

        delete data_to_partition_;
        data_to_partition_ = new std::unordered_map<T, Partition<T>*>();
        auto sorted_vertex = std::move(workload_graph_.sorted_vertex());
        for (auto i = 0; i < partition_scheme.size(); i++) {
            auto partition = partition_scheme[i];
            if (partition >= n_partitions_) {
                printf("ERROR: partition was %d!\n", partition);
                fflush(stdout);
            }
            auto data = sorted_vertex[i];
            data_to_partition_->emplace(data, partitions_.at(partition));
        }

        data_to_partition_copy_ = *data_to_partition_;
        if (first_repartition) {
            first_repartition = false;
        }
    }

    int n_partitions_;
    int round_robin_counter_ = 0;
    int sync_counter_ = 0;
    int n_dispatched_requests_ = 0;
    kvstorage::Storage storage_;
    std::unordered_map<int, Partition<T>*> partitions_;
    std::unordered_map<T, Partition<T>*>* data_to_partition_;
    std::unordered_map<T, Partition<T>*> data_to_partition_copy_;

    std::thread graph_thread_;
    std::queue<struct client_message> graph_requests_queue_;
    sem_t graph_requests_semaphore_;
    std::mutex graph_requests_mutex_;

    std::vector<time_point> repartition_timestamps_;
    std::vector<duration> graph_copy_duration_;
    std::vector<duration> repartition_duration_;

    model::Graph<T> workload_graph_;
    model::CutMethod repartition_method_;
    int repartition_interval_;
    bool first_repartition = true;
    pthread_barrier_t repartition_barrier_;
    
};

};


#endif
