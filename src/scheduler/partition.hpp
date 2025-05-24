#ifndef KVPAXOS_PARTITION_H
#define KVPAXOS_PARTITION_H


#include <arpa/inet.h>
#include <chrono>
#include <pthread.h>
#include <queue>
#include <iterator>
#include <mutex>
#include <numeric>
#include <semaphore.h>
#include <sstream>
#include <shared_mutex>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <assert.h>
#include "graph/graph.hpp"
#include "request/request.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>
#include "utils/utils.h"


namespace kvpaxos {
using namespace kvstorage;
using namespace std;

template <typename T, size_t Capacity = 0>
class Partition {
typedef unordered_map<T, Partition<T, Capacity>*> partition_map_t;
public:
    Partition(int id)
        : __id{id},
          n_executed_requests_{0},
          executing_{true}
    {
        storage[__id] = Storage();
        socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    }

    ~Partition() {
        executing_ = false;
        if (worker_thread_.joinable()) {
            sem_post(&semaphore_);
            worker_thread_.join();
        }
    }

    void start_worker_thread() {
        sem_init(&semaphore_, 0, 0);
        if constexpr(Capacity > 0){
            sem_init(&remaining_space_, 0, Capacity);
        }

        worker_thread_ = thread(&Partition<T, Capacity>::thread_loop, this);
        utils::set_affinity(__id+5, worker_thread_, cpu_set);
    }

    size_t request_queue_size() const {
        if constexpr(Capacity > 0){
            return __bounded_requests_queue.read_available();
        } else {
            size_t size = __requests_queue.size();
            return size;
        }
    }

    size_t error_count() {
        return error_count_;
    }

    void push_request(struct client_message request) {
        if constexpr(Capacity > 0){
            sem_wait(&remaining_space_);
            __bounded_requests_queue.push(request);
        }else{
            __queue_mutex.lock();
                __requests_queue.push(move(request));
            __queue_mutex.unlock();
        }
        sem_post(&semaphore_);
    }

    struct client_message pop_request() {
        struct client_message request;
        sem_wait(&semaphore_);

        if constexpr(Capacity > 0){
            request = move(__bounded_requests_queue.front());
            __bounded_requests_queue.pop();
            sem_post(&remaining_space_);
        }else{
            __queue_mutex.lock();
                request = move(__requests_queue.front());
                __requests_queue.pop();
            __queue_mutex.unlock();
        }
        return request;
    }

    void insert_data(const T& data, int weight = 0) {
        weight_[data] = weight;
        total_weight_ += weight;
    }

    void remove_data(const T& data) {
        total_weight_ -= weight_.at(data);
        weight_.erase(data);
    }

    void increase_weight(const T& data, int weight = 1) {
        weight_[data] += weight;
        total_weight_ += weight;
    }

    int weight() const {
        return total_weight_;
    }

    int id() const {
        return __id;
    }

    size_t n_executed_requests() const {
        return n_executed_requests_;
    }


    static void add_old_partition_map(partition_map_t* version_map){
        version_maps_mtx.lock();
        version_maps.push_back(version_map);
        version_maps_mtx.unlock();
    }

    static void create_storage(size_t partitions_){
        partitions = partitions_;
        storage = new Storage[partitions];
    }
private:


    string *read(int key){
        string* val = storage[__id].read(key);
        Storage* past_storage = nullptr;
        if (val == nullptr){
            for (size_t i = version_count-1; i >= 0; i--)
            {
                version_maps_mtx.lock_shared();
                partition_map_t *map = version_maps.at(i);
                auto partition = map->find(key);
                if (partition != map->end()){
                    int past_id = partition->second->__id;
                    past_storage = &previous_storage[i][past_id];
                    val = past_storage->read(key);
                    break;
                }
                version_maps_mtx.unlock_shared();
            }
            
        }
        if (val != nullptr && past_storage != nullptr) {
            past_storage->del(key);
            storage[__id].write(key, *val);
        }
        return val;
    }

    void thread_loop() {
        while (executing_) {

            struct client_message request = pop_request();
            if (!executing_) {
                return;
            }

            auto type = static_cast<request_type>(request.type);
            auto key = request.key;
            auto request_args = string(request.args);
            int coordinator = 0;
            pthread_barrier_t * barrier;
            string answer;
            switch (type)
            {
            case READ:
            {   
                read(key);
                break;
            }

            case WRITE:
            {
                storage[__id].write(key, request_args);
                break;
            }

            case SCAN:
            {
                barrier = (pthread_barrier_t*) request.s_addr;
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    auto length = stoi(request_args);
                    vector<string> values;
                    for (auto key_i = key; key_i < key+length; key_i++) {
                        auto value = read(key_i);
                        if (value == nullptr){
                            error_count_++;
                            break;
                        }
                        values.push_back(string(*value));
                        delete value;
                    }
                }
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    pthread_barrier_destroy(barrier);
                    delete barrier;
                }
                break;
            }

            case DEL:
            {
                storage[__id].del(key);
                break;
            }

            case SYNC:
            {
                barrier = (pthread_barrier_t*) request.s_addr;
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    pthread_barrier_destroy(barrier);
                    delete barrier;
                }
                continue;
            }
            case REPARTITION:
                barrier = (pthread_barrier_t*) request.s_addr;
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    previous_storage.push_back(storage);
                    storage = new Storage[partitions];
                }
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    pthread_barrier_destroy(barrier);
                    delete barrier;
                }
                storage[__id] = Storage();
                version_count++;
                break;
            case ERROR:
                answer = "ERROR";
                break;
            default:
                break;
            }

            n_executed_requests_++;
        }
    }

    int __id, socket_fd_;
    size_t n_executed_requests_;
    static Storage *storage;
    cpu_set_t cpu_set;

    bool executing_;
    thread worker_thread_;
    sem_t semaphore_;
    queue<struct client_message> __requests_queue;
    boost::lockfree::spsc_queue<struct client_message, boost::lockfree::capacity<Capacity>> __bounded_requests_queue;
    mutex __queue_mutex;

    int total_weight_ = 0;
    unordered_map<T, int> weight_;

    sem_t remaining_space_;

    size_t error_count_ = 0;
    static size_t partitions;
    static vector<Storage*> previous_storage;
    size_t version_count;
    static vector<partition_map_t*> version_maps;
    static shared_mutex version_maps_mtx;


};
template<typename T, size_t Capacity>
vector<Storage*> Partition<T, Capacity>::previous_storage;

template<typename T, size_t Capacity>
size_t Partition<T, Capacity>::partitions = 0;

template<typename T, size_t Capacity>
Storage* Partition<T, Capacity>::storage;

template<typename T, size_t Capacity>
vector<unordered_map<T, Partition<T, Capacity>*>*> Partition<T, Capacity>::version_maps;

template<typename T, size_t Capacity>
shared_mutex Partition<T, Capacity>::version_maps_mtx;

}

#endif
