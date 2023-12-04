#ifndef _KVPAXOS_SCHEDULER_H_
#define _KVPAXOS_SCHEDULER_H_


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


namespace kvpaxos {

template <typename T, size_t WorkerCapacity = 0>
class Scheduler {
public:
    Scheduler(){}
    Scheduler(int n_requests,
                int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) : n_partitions_{n_partitions},
        repartition_interval_{repartition_interval},
        repartition_method_{repartition_method}
    {
        for (auto i = 0; i < n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            partitions_.emplace(i, partition);
        }
        data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        sem_init(&graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&repartition_barrier_, NULL, 2);
        graph_thread_ = std::thread(&Scheduler<T, WorkerCapacity>::update_graph_loop, this);
    }

    ~Scheduler() {
        for (auto partition: partitions_) {
            delete partition.second;
        }
        delete data_to_partition_;
    }

    void process_populate_request(struct client_message& request) {
        add_key(request.key);

        if (repartition_method_ != model::ROUND_ROBIN) {
            graph_requests_mutex_.lock();
                graph_requests_queue_.push_back(request);
            graph_requests_mutex_.unlock();
            sem_post(&graph_requests_semaphore_);
        }

        Partition<T, WorkerCapacity>::populate_storage(request);
    }

    void wait_populate(){
        notify_graph(SYNC);
        pthread_barrier_wait(&repartition_barrier_);
    }

    void run() {
        for (auto& kv : partitions_) {
            kv.second->start_worker_thread();
        }
    }

    int n_executed_requests() {
        auto n_executed_requests = 0;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            n_executed_requests += partition->n_executed_requests();
        }
        return n_executed_requests;
    }

    int graph_vertices(){
        graph_requests_mutex_.lock();
        int size = workload_graph_.n_vertex();

        graph_requests_mutex_.unlock();
        return size;
    }

    int graph_edges(){
        graph_requests_mutex_.lock();
        int size = workload_graph_.n_edges();

        graph_requests_mutex_.unlock();
        return size;
    }

    int n_dispatched_requests(){
        return n_dispatched_requests_;
    }

    const std::vector<time_point>& repartition_timestamps() const {
        return repartition_timestamps_;
    }

    const std::vector<duration>& graph_copy_duration() const {
        return graph_copy_duration_;
    }

    const std::vector<time_point>& repartition_end_timestamps() const {
        return repartition_end_timestamps_;
    }
    
    const std::vector<time_point>& repartition_notify_timestamp() const {
        return repartition_notify_timestamp_;
    }
    
    void dispatch(struct client_message& request){

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
            partitions_.at(0)->push_request(request);
        }else{
            auto arbitrary_partition = *begin(partitions);
            if (partitions.size() > 1) {
                sync_partitions(partitions);
                arbitrary_partition->push_request(request);
                sync_partitions(partitions);
            } else {
                arbitrary_partition->push_request(request);
            }
        }
        
        if (repartition_method_ != model::ROUND_ROBIN) {
            graph_requests_mutex_.lock();
                graph_requests_queue_.push_back(request);
            graph_requests_mutex_.unlock();
            sem_post(&graph_requests_semaphore_);
        
        }
    }
    
    void schedule_and_answer(struct client_message& request) {

        dispatch(request);
        n_dispatched_requests_++;

        if (repartition_method_ != model::ROUND_ROBIN) {
            if (
                n_dispatched_requests_ % repartition_interval_ == 0
            ) {
                repartition_notify_timestamp_.push_back(std::chrono::system_clock::now());

                notify_graph(SYNC);
                pthread_barrier_wait(&repartition_barrier_);

                auto begin = std::chrono::system_clock::now();
                auto input_graph = new InputGraph<T>(workload_graph_);
                graph_copy_duration_.push_back(std::chrono::system_clock::now() - begin);

                auto temp = partitioning(input_graph);
                delete data_to_partition_;
                data_to_partition_ = temp;

                sync_all_partitions();
            }
        }
    }

public:
    void notify_graph(request_type type){
        struct client_message sync_message;
        sync_message.type = type;

        graph_requests_mutex_.lock();
            graph_requests_queue_.push_back(sync_message);
        graph_requests_mutex_.unlock();
        sem_post(&graph_requests_semaphore_);
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
            if (not mapped(request.key + i)) {
                return std::unordered_set<Partition<T,WorkerCapacity>*>();
            }

            partitions.insert(data_to_partition_->at(request.key + i));
        }

        return partitions;
    }

    struct client_message create_sync_request(int n_partitions) {
        struct client_message sync_message;
        sync_message.id = sync_counter_;
        sync_message.type = SYNC;

        // this is a gross workaround to send the barrier to the partitions.
        // a more elegant approach would be appreciated.
        auto* barrier = new pthread_barrier_t();
        pthread_barrier_init(barrier, NULL, n_partitions);
        sync_message.s_addr = (unsigned long) barrier;

        return sync_message;
    }

    void sync_partitions(const std::unordered_set<Partition<T, WorkerCapacity>*>& partitions) {
        auto sync_message = std::move(
            create_sync_request(partitions.size())
        );
        for (auto partition : partitions) {
            partition->push_request(sync_message);
        }
    }

    void sync_all_partitions() {
        std::unordered_set<Partition<T, WorkerCapacity>*> partitions;
        for (auto i = 0; i < partitions_.size(); i++) {
            partitions.insert(partitions_.at(i));
        }
        sync_partitions(partitions);
    }

    void add_key(T key) {
        auto partition_id = round_robin_counter_;
        data_to_partition_->emplace(key, partitions_.at(partition_id));

        round_robin_counter_ = (round_robin_counter_+1) % n_partitions_;
    }

    bool mapped(T key) const {
        return data_to_partition_->find(key) != data_to_partition_->end();
    }

    void update_graph_loop() {
        while(true) {
            sem_wait(&graph_requests_semaphore_);

            graph_requests_mutex_.lock();
                auto request = std::move(graph_requests_queue_.front());
                graph_requests_queue_.pop_front();
            graph_requests_mutex_.unlock();

            if (request.type == SYNC) {
                pthread_barrier_wait(&repartition_barrier_);
            } else {
                update_graph(request);
            }
        }
    }

    void update_graph(const client_message& message) {
        std::vector<int> data{message.key};
        size_t data_size = 1;
        if (message.type == SCAN) {
            data_size == std::stoi(message.args);
        }

        for (auto i = 0; i < data_size; i++) {
            if (not workload_graph_.vertice_exists(message.key+i)) {
                workload_graph_.add_vertice(message.key+i);
            }

            workload_graph_.increase_vertice_weight(message.key+i);

            for (auto j = i+1; j < data_size; j++) {
                if (not workload_graph_.vertice_exists(message.key+j)) {
                    workload_graph_.add_vertice(message.key+j);
                }
                if (not workload_graph_.are_connected(message.key+i, message.key+j)) {
                    workload_graph_.add_edge(message.key+i, message.key+j);
                }

                workload_graph_.increase_edge_weight(message.key+i, message.key+j);
            }
        }
    }


    std::unordered_map<T, Partition<T, WorkerCapacity>*>* partitioning(struct InputGraph<T>* graph) {
        auto start_timestamp = std::chrono::system_clock::now();
        repartition_timestamps_.emplace_back(start_timestamp);

        auto partition_scheme = std::move(
            model::multilevel_cut(
                graph->vertice_weight, 
                graph->x_edges, 
                graph->edges, 
                graph->edges_weight,
                partitions_.size(), 
                repartition_method_
            )
        );
        
        auto end_timestamp = std::chrono::system_clock::now();
        repartition_end_timestamps_.push_back(end_timestamp);


        auto data_to_partition = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();
        
        for (auto& it : graph->vertice_to_pos) {
            T key = it.first;
            int position = it.second;
            //position indicates the position of the key in partition scheme
            int partition = partition_scheme[position];  
            if (partition >= n_partitions_) {
                printf("ERROR: partition was %d!\n", partition);
                fflush(stdout);
            }
            data_to_partition->emplace(key, partitions_.at(partition));
        }

        if (first_repartition) {
            first_repartition = false;
        }
        
        return data_to_partition;
    }

    int n_partitions_;
    int round_robin_counter_ = 0;
    int sync_counter_ = 0;
    int n_dispatched_requests_ = 0;
    kvstorage::Storage storage_;
    std::unordered_map<int, Partition<T, WorkerCapacity>*> partitions_;
    std::unordered_map<T, Partition<T, WorkerCapacity>*>* data_to_partition_;
    
    std::thread graph_thread_;
    std::deque<struct client_message> graph_requests_queue_;
    sem_t graph_requests_semaphore_;
    std::mutex graph_requests_mutex_;

    std::vector<time_point> repartition_timestamps_;
    std::vector<duration> graph_copy_duration_;
    std::vector<time_point> repartition_end_timestamps_;

    model::Graph<T> workload_graph_;
    model::CutMethod repartition_method_;
    int repartition_interval_;
    bool first_repartition = true;
    pthread_barrier_t repartition_barrier_;

    std::vector<time_point> repartition_notify_timestamp_;

};

};


#endif
