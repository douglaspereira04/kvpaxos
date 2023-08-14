#ifndef _KVPAXOS_NEW_SCHEDULER_H_
#define _KVPAXOS_NEW_SCHEDULER_H_



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
#include <iostream>


namespace kvpaxos {

template <typename T>
class NewScheduler {
public:

    NewScheduler(int n_requests,
                int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) : n_partitions_{n_partitions},
        repartition_interval_{repartition_interval},
        repartition_method_{repartition_method},
        reparting_{false}
    {
        for (auto i = 0; i < n_partitions_; i++) {
            auto* partition = new Partition<T>(i);
            partitions_.emplace(i, partition);
        }
        data_to_partition_ = new std::unordered_map<T, Partition<T>*>();

        sem_init(&graph_requests_semaphore_, 0, 0);
        pthread_barrier_init(&repartition_barrier_, NULL, 2);
        graph_thread_ = std::thread(&NewScheduler<T>::update_graph_loop, this);

        sem_init(&repartition_semaphore_, 0, 0);
        repartition_thread_ = std::thread(&NewScheduler<T>::repartition_loop, this);

        sem_init(&swap_maps_semaphore_, 0, 0);
        sem_init(&continue_reparting_semaphore_, 0, 0);

        #if !defined(CONCURRENT_COPY)
            sem_init(&copy_semaphore_, 0, 0);
        #endif
    }

    ~NewScheduler() {
        for (auto partition: partitions_) {
            delete partition.second;
        }
        delete data_to_partition_;
    }

    void process_populate_requests(const std::vector<workload::Request>& requests) {
        for (auto& request : requests) {
            add_key(request.key());
        }
        Partition<T>::populate_storage(requests);
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

    const std::vector<time_point>& repartition_timestamps() const {
        return repartition_timestamps_;
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
        if (partitions.size() > 1) {
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
            
            
            if (sem_trywait(&swap_maps_semaphore_)==0) {
            //Repartition completed, time to change swap the partition map
                //Wait graph thread to process all requests
                graph_sync();

                reparting_ = false; //When CONCURRENT_COPY is ON "reparting_ is already implicitly locked"

                delete data_to_partition_;
                data_to_partition_ = data_to_partition_2_;
                data_to_partition_copy_ = *data_to_partition_;

                //Restores all the "forgotten" keys when the repartition is completed
                auto it = pending_keys_.begin();
                for (; it < pending_keys_.end(); it++){
                    auto key = (*it).first;
                    if (!mapped(key)) {
                        auto partition_id = (*it).second;
                        add_key(key, partition_id);
                    }
                    pending_keys_.erase(it);
                }

                sync_all_partitions();

                sem_post(&continue_reparting_semaphore_);
            } else if (
                n_dispatched_requests_ % repartition_interval_ == 0
            ) {
                #if !defined(CONCURRENT_COPY)
                    //The partition map and workload graph are copied 
                    // while graph thread has no pending operations,
                    // guaranteeing tha both are consistent in a way that
                    // every key present in the copy of the map 
                    // is also present in the copy of the workload graph
                    graph_sync();
                #endif
                reparting_ = true;
                
                //When copy is done a signal is emited to the repartition thread
                sem_post(&repartition_semaphore_);

                #if !defined(CONCURRENT_COPY)
                    sem_wait(&copy_semaphore_);
                #endif
                //The scheduling can continue while the partitioning is in progress
            }
        }
    }

private:

    void graph_sync(){
        struct client_message sync_message;
        sync_message.type = SYNC;

        graph_requests_mutex_.lock();
            graph_requests_queue_.push(sync_message);
        graph_requests_mutex_.unlock();
        sem_post(&graph_requests_semaphore_);

        pthread_barrier_wait(&repartition_barrier_);
    }

    std::unordered_set<Partition<T>*> involved_partitions(
        const struct client_message& request)
    {
        std::unordered_set<Partition<T>*> partitions;
        auto type = static_cast<request_type>(request.type);

        auto range = 1;
        if (type == SCAN) {
            range = std::stoi(request.args);
        }

        for (auto i = 0; i < range; i++) {
            if (not mapped(request.key + i)) {
                return std::unordered_set<Partition<T>*>();
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

    void sync_partitions(const std::unordered_set<Partition<T>*>& partitions) {
        auto sync_message = std::move(
            create_sync_request(partitions.size())
        );
        for (auto partition : partitions) {
            partition->push_request(sync_message);
        }
    }

    void sync_all_partitions() {
        std::unordered_set<Partition<T>*> partitions;
        for (auto i = 0; i < partitions_.size(); i++) {
            partitions.insert(partitions_.at(i));
        }
        sync_partitions(partitions);
    }

    void add_key(T key) {
        auto partition_id = round_robin_counter_;
        round_robin_counter_ = (round_robin_counter_+1) % n_partitions_;

        if(reparting_){
            //Saves keys that wont be present in the reparted graph
            pending_keys_.push_back(std::make_pair(key,partition_id));
        }

        add_key(key, partition_id);
    }


    void add_key(T key, int partition_id) {
        data_to_partition_->emplace(key, partitions_.at(partition_id));

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

    bool mapped(T key) const {
        return data_to_partition_->find(key) != data_to_partition_->end();
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

    void repartition_loop(){

        while(true){
            sem_wait(&repartition_semaphore_);
            
            #if defined(CONCURRENT_COPY)
                repart_data_to_partition_ = data_to_partition_;
                repart_workload_graph_ = &workload_graph_;
            #else
                //delete copies
                repart_data_to_partition_ = new std::unordered_map<T, Partition<T>*>(*data_to_partition_);
                repart_workload_graph_ = new model::Graph<T>(workload_graph_);

                sem_post(&copy_semaphore_);
            #endif

            //New partition map is created
            data_to_partition_2_ = repart();

            #if !defined(CONCURRENT_COPY)
                delete repart_data_to_partition_;
                delete repart_workload_graph_;
            #endif

            
            //Send a signal to the scheduler
            sem_post(&swap_maps_semaphore_);
            //Waits for the swap completes before another repartition
            sem_wait(&continue_reparting_semaphore_);
        }
    }


    std::unordered_map<T, Partition<T>*> *repart() {
        auto start_timestamp = std::chrono::system_clock::now();
        repartition_timestamps_.emplace_back(start_timestamp);

        std::vector<int> partition_scheme;

        //METIS and KAHIP dont depend on the old partition map
        if(repartition_method_== model::CutMethod::KAHIP 
        || repartition_method_== model::CutMethod::METIS ){
            std::vector<int> x_edges;
            std::vector<int> edges;
            std::vector<int> vertice_weight;
            std::vector<int> edges_weight;

            model::multilevel_cut_data(
                *repart_workload_graph_, 
                x_edges,
                edges,
                vertice_weight,
                edges_weight
            );
            

            partition_scheme = multilevel_cut(
            *repart_workload_graph_, 
            x_edges,
            edges,
            vertice_weight,
            edges_weight,
            partitions_.size(),
            repartition_method_);
        }else{
            partition_scheme = std::move(
                model::cut_graph(
                    *repart_workload_graph_,
                    partitions_,
                    repartition_method_,
                    *repart_data_to_partition_,
                    first_repartition
                )
            );
        }



        auto data_to_partition = new std::unordered_map<T, Partition<T>*>();
        auto sorted_vertex = std::move(repart_workload_graph_->sorted_vertex());
        for (auto i = 0; i < partition_scheme.size(); i++) {
            auto partition = partition_scheme[i];
            if (partition >= n_partitions_) {
                printf("ERROR: partition was %d!\n", partition);
                fflush(stdout);
            }
            auto data = sorted_vertex[i];
            data_to_partition->emplace(data, partitions_.at(partition));
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
    std::unordered_map<int, Partition<T>*> partitions_;
    std::unordered_map<T, Partition<T>*>* data_to_partition_;
    std::unordered_map<T, Partition<T>*> data_to_partition_copy_;


    std::unordered_map<T, Partition<T>*>* data_to_partition_2_;

    sem_t repartition_semaphore_;
    std::thread repartition_thread_;
    model::Graph<T>* repart_workload_graph_;
    std::unordered_map<T, Partition<T>*>* repart_data_to_partition_;


    bool reparting_;
    std::vector<std::pair<T, int>> pending_keys_;
    
    #if !defined(CONCURRENT_COPY)
        sem_t copy_semaphore_;
    #endif

    sem_t swap_maps_semaphore_;
    sem_t continue_reparting_semaphore_;

    std::thread graph_thread_;
    std::queue<struct client_message> graph_requests_queue_;
    sem_t graph_requests_semaphore_;
    std::mutex graph_requests_mutex_;

    std::vector<time_point> repartition_timestamps_;
    model::Graph<T> workload_graph_;
    model::CutMethod repartition_method_;
    int repartition_interval_;
    bool first_repartition = true;
    pthread_barrier_t repartition_barrier_;
};

};



#endif