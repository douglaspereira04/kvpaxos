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
#include <assert.h>
#include "input_graph.hpp"
#include "graph/graph.hpp"
#include "graph/partitioning.h"
#include "partition.hpp"
#include "request/request.hpp"
#include "linked_queue/linked_queue.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include <iostream>
#include "utils/utils.h"


namespace kvpaxos {

template <typename T, size_t TL = 0, size_t WorkerCapacity = 0, interval_type IntervalType = interval_type::OPERATIONS>
class Scheduler {
public:
    Scheduler(){}
    Scheduler(int repartition_interval,
                int n_partitions,
                model::CutMethod repartition_method
    ) : n_partitions_{n_partitions},
        repartition_method_{repartition_method}
    {
        if constexpr(IntervalType == interval_type::MICROSECONDS){
            time_start_ = std::chrono::system_clock::now();
            time_interval_ = std::chrono::microseconds(repartition_interval);
        } else if constexpr(IntervalType == interval_type::OPERATIONS){
            operation_interval_ = repartition_interval;
        }
        round_robin_counter_ = 0;
        sync_counter_ = 0;
        n_dispatched_requests_ = 0;

        for (auto i = 0; i < n_partitions_; i++) {
            auto* partition = new Partition<T, WorkerCapacity>(i);
            partitions_.emplace(i, partition);
        }
        data_to_partition_ = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        pthread_barrier_init(&repartition_barrier_, NULL, 2);

        scheduling_thread_ = std::thread(&Scheduler<T, TL, WorkerCapacity, IntervalType>::scheduling_loop, this);
        utils::set_affinity(2,scheduling_thread_, scheduler_cpu_set_);

        graph_thread_ = std::thread(&Scheduler<T, TL, WorkerCapacity, IntervalType>::update_graph_loop, this);
	    utils::set_affinity(3, graph_thread_, graph_cpu_set_);

        note_ = false;

        client_message dummy;
        dummy.type = DUMMY;

        if constexpr(TL > 0){
            for (size_t i = 0; i < TL; i++)
            {
                graph_deletion_queue_.push_back(dummy);
            }
        }

    }

    ~Scheduler() {
        for (auto partition: partitions_) {
            delete partition.second;
        }
        delete data_to_partition_;
    }

    void process_populate_request(struct client_message& request) {

        map_key(request.key);
        Partition<T, WorkerCapacity>::populate_storage(request);

        if (repartition_method_ != model::ROUND_ROBIN) {
            update_graph(request);

            if constexpr(TL > 0){
                graph_deletion_queue_.push_back(request);

                auto expired_request = std::move(graph_deletion_queue_.front());
                graph_deletion_queue_.pop_front();
                Scheduler<T, TL, WorkerCapacity, IntervalType>::expire(expired_request);
            }
        }
    }

    void run() {
        for (auto& kv : partitions_) {
            kv.second->start_worker_thread();
        }
    }

    void join(){
        scheduling_thread_.join();
    }

    size_t n_executed_requests() {
        size_t n_executed_requests = 0;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            n_executed_requests += partition->n_executed_requests();
        }
        return n_executed_requests;
    }

    std::vector<size_t> in_queue_amount() {
        std::vector<size_t> in_queue;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            size_t amount = partition->request_queue_size();
            in_queue.push_back(amount);
        }
        return in_queue;
    }

    size_t graph_vertices(){
        return workload_graph_.n_vertex();
    }

    size_t graph_edges(){
        return workload_graph_.n_edges();
    }

    time_point schedule_end(){
        return schedule_end_;
    }

    int n_dispatched_requests(){
        return n_dispatched_requests_;
    }

    int error_count(){
        int count = 0;
        for (auto& kv: partitions_) {
            auto* partition = kv.second;
            count += partition->error_count();
        }
        return count;
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

    const std::vector<time_point>& repartition_apply_timestamp() const {
        return repartition_apply_timestamp_;
    }

    const std::vector<time_point>& repartition_request_timestamp() const {
        return repartition_request_timestamp_;
    }

    const std::vector<duration>& reconstruction_duration() const {
        return reconstruction_duration_;
    }

    void dispatch(struct client_message& request){

        auto partitions = std::move(involved_partitions(request));
        auto arbitrary_partition = *begin(partitions);
        if (partitions.size() > 1) {
            sync_partitions(partitions);
            arbitrary_partition->push_request(request);
            sync_partitions(partitions);
        } else {
            arbitrary_partition->push_request(request);
        }
    }

    void schedule_and_answer(struct client_message& request) {

        dispatch(request);
        n_dispatched_requests_++;

        if (repartition_method_ != model::ROUND_ROBIN) {
            bool start_repartitioning = false;
            if constexpr(IntervalType == interval_type::MICROSECONDS){
                auto interval = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start_);
                if(interval >= time_interval_){
                    start_repartitioning = true;
                }
            } else if constexpr(IntervalType == interval_type::OPERATIONS){
                start_repartitioning = n_dispatched_requests_ % operation_interval_ == 0;
            }
            if (start_repartitioning) {
                if constexpr(utils::ENABLE_INFO){
                    repartition_request_timestamp_.push_back(std::chrono::system_clock::now());
                }
                
                note_ = true;
                scheduling_queue_.template free<1>();
                pthread_barrier_wait(&repartition_barrier_);

                time_point begin;
                if constexpr(utils::ENABLE_INFO){
                    begin = std::chrono::system_clock::now();
                }
                auto input_graph = InputGraph<T>(workload_graph_);
                pthread_barrier_wait(&repartition_barrier_);


                if constexpr(utils::ENABLE_INFO){
                    graph_copy_duration_.push_back(std::chrono::system_clock::now() - begin);
                }

                auto temp = partitioning(input_graph);

                delete data_to_partition_;
                data_to_partition_ = temp;

                sync_all_partitions();

                if constexpr(utils::ENABLE_INFO){
                    repartition_apply_timestamp_.push_back(std::chrono::system_clock::now());
                }
                if constexpr(IntervalType == interval_type::MICROSECONDS){
                    time_start_ = std::chrono::system_clock::now();
                }
            }
        }
    }

public:

    std::unordered_set<Partition<T, WorkerCapacity>*> involved_partitions(
        const struct client_message& request)
    {
        std::unordered_set<Partition<T, WorkerCapacity>*> partitions;
        auto type = static_cast<request_type>(request.type);

        auto range = 1;
        if (type == SCAN) {
            range = std::stoi(request.args);
        }


        bool new_mapping = false;
        for (auto i = 0; i < range; i++) {

            if(!Scheduler<T, TL, WorkerCapacity, IntervalType>::mapped(request.key + i)){
                map_key(request.key + i, round_robin_counter_);
                new_mapping = true;
            } else {
                partitions.insert(data_to_partition_->at(request.key + i));
            }
        }

        if(new_mapping){
            partitions.insert(partitions_.at(round_robin_counter_));
            round_robin_counter_ = (round_robin_counter_+1) % n_partitions_;
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

    void map_key(T key) {
        auto partition_id = round_robin_counter_;
        data_to_partition_->emplace(key, partitions_.at(partition_id));

        round_robin_counter_ = (round_robin_counter_+1) % n_partitions_;
    }

    void map_key(T key, int partition_id) {
        data_to_partition_->emplace(key, partitions_.at(partition_id));
    }

    bool mapped(T key) const {
        return data_to_partition_->find(key) != data_to_partition_->end();
    }


    void submit(client_message &message){
        scheduling_queue_.push(message);
        scheduling_queue_.template notify<0>();
        scheduling_queue_.template notify<1>();
    }

    void scheduling_loop() {
        while(true){
            scheduling_queue_.template wait<0>();
            client_message message = scheduling_queue_.template pop<0>();
            if (message.type == END){
                break;
            }
            schedule_and_answer(message);
        }
        
        schedule_end_ = std::chrono::system_clock::now();
    }

    void update_graph_loop() {

        while(true) {
            scheduling_queue_.template wait<1>();
            if (note_ && scheduling_queue_.template is_ahead<1>()){
                note_ = false;
                pthread_barrier_wait(&repartition_barrier_);
                pthread_barrier_wait(&repartition_barrier_);
            } else {
                client_message request = scheduling_queue_.template pop<1>();

                update_graph(request);

                if constexpr(TL > 0){
                    graph_deletion_queue_.push_back(request);

                    auto expired_request = std::move(graph_deletion_queue_.front());
                    graph_deletion_queue_.pop_front();
                    Scheduler<T, TL, WorkerCapacity, IntervalType>::expire(expired_request);
                }
            }
        }
    }

    void update_graph(const client_message& message) {
        size_t data_size = 1;
        if (message.type == SCAN) {
            data_size = std::stoi(message.args);
        }

        for (auto i = 0; i < data_size; i++) {
            workload_graph_.add_vertice(message.key+i);
            workload_graph_.increment_vertice_weight(message.key+i, 1);

            for (auto j = i+1; j < data_size; j++) {
                workload_graph_.add_vertice(message.key+j);
                workload_graph_.add_edge(message.key+i, message.key+j);
                workload_graph_.increment_edge_weight(message.key+i, message.key+j, 1);
            }
        }
    }

    void expire(const client_message& message) {
        if(message.type != DUMMY){
            int data_size = 1;
            if (message.type == SCAN) {
                data_size = std::stoi(message.args);
            }

            for (int i = data_size-1; i >= 0; i--) {
                for (int j = data_size-1; j >= i+1; j--) {
                    workload_graph_.increment_edge_weight(message.key+i, message.key+j, -1);
                    workload_graph_.remove_weightless_edge(message.key+i, message.key+j);
                    workload_graph_.remove_weightless_vertice(message.key+j);
                }
                workload_graph_.increment_vertice_weight(message.key+i, -1);
                workload_graph_.remove_weightless_vertice(message.key+i);
            }
        }
    }


    std::unordered_map<T, Partition<T, WorkerCapacity>*>* partitioning(InputGraph<T> &graph) {

        if constexpr(utils::ENABLE_INFO){
            repartition_timestamps_.push_back(std::chrono::system_clock::now());
        }

        auto partition_scheme = std::move(
            model::multilevel_cut(
                graph.vertice_weight, 
                graph.x_edges, 
                graph.edges, 
                graph.edges_weight,
                partitions_.size(), 
                repartition_method_
            )
        );

        time_point reconstruction_begin;
        if constexpr(utils::ENABLE_INFO){
            repartition_end_timestamps_.push_back(std::chrono::system_clock::now());
            reconstruction_begin = std::chrono::system_clock::now();
        }
        auto data_to_partition = new std::unordered_map<T, Partition<T, WorkerCapacity>*>();

        for (auto& it : graph.vertice_to_pos) {
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

        if constexpr(utils::ENABLE_INFO){
            reconstruction_duration_.push_back(std::chrono::system_clock::now() - reconstruction_begin);
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
    cpu_set_t graph_cpu_set_;

    std::thread scheduling_thread_;
    cpu_set_t scheduler_cpu_set_;

    std::deque<struct client_message> graph_deletion_queue_;

    model::Graph<T> workload_graph_;
    model::CutMethod repartition_method_;
    pthread_barrier_t repartition_barrier_;

    int operation_interval_;
    std::chrono::microseconds time_interval_;
    std::chrono::_V2::high_resolution_clock::time_point time_start_;


    std::vector<time_point> repartition_timestamps_;
    std::vector<duration> graph_copy_duration_;
    std::vector<time_point> repartition_end_timestamps_;
    std::vector<time_point> repartition_request_timestamp_;
    std::vector<time_point> repartition_apply_timestamp_;
    std::vector<duration> reconstruction_duration_;
    time_point schedule_end_;

    std::vector<std::vector<size_t>> in_queue_amount_;

    model::LinkedQueue<client_message, 100> scheduling_queue_;

    bool note_;

};

};


#endif
