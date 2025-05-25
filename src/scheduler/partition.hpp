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
#include <fstream>

namespace kvpaxos {
using namespace kvstorage;
using namespace std;
using namespace workload;

template <typename T, size_t Capacity = 0>
class Partition {
typedef unordered_map<T, Partition<T, Capacity>*> partition_map_t;
public:
    Partition(int id)
        : __id{id},
          __n_executed_requests{0},
          __executing{true}
    {
        storage[__id] = Storage();
        output_file = ofstream("partition_output_" + to_string(__id));
    }

    ~Partition() {
        __executing = false;
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

    void push_request(Request *request) {
        if constexpr(Capacity > 0){
            sem_wait(&remaining_space_);
            __bounded_requests_queue.push(request);
        }else{
            __queue_mutex.lock();
                __requests_queue.push(request);
            __queue_mutex.unlock();
        }
        sem_post(&semaphore_);
    }

    Request * pop_request() {
        Request *request;
        sem_wait(&semaphore_);

        if constexpr(Capacity > 0){
            request = __bounded_requests_queue.front();
            __bounded_requests_queue.pop();
            sem_post(&remaining_space_);
        }else{
            __queue_mutex.lock();
                request = __requests_queue.front();
                __requests_queue.pop();
            __queue_mutex.unlock();
        }
        return request;
    }


    int id() const {
        return __id;
    }

    size_t n_executed_requests() const {
        return __n_executed_requests;
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


    size_t read(int key, char* &val){
        val = nullptr;
        size_t len = storage[__id].read(key, val);
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
                    len = past_storage->read(key, val);
                    if (val != nullptr){
                        break;
                    }
                }
                version_maps_mtx.unlock_shared();
            }
            
        }
        if (val != nullptr && past_storage != nullptr) {
            past_storage->del(key);
            storage[__id].write(key, val, len);
        }
        return len;
    }

    void thread_loop() {
        while (__executing) {

            Request *request = pop_request();
            if (!__executing) {
                return;
            }

            RequestType type = request->type();
            auto key = request->key();
            int coordinator = 0;
            pthread_barrier_t * barrier;
            string answer;
            switch (type)
            {
            case READ:
            {   
                char *value;
                size_t len = read(key, value);
                output_file << "read( " << key << " ): ";
                output_file.write(value, len);
                output_file << "\n";
                delete[] value;
                break;
            }

            case WRITE:
            {
                storage[__id].write(key, request->args(), request->args_len());
                output_file << "write( " << key << ", ";
                output_file.write(request->args(), request->args_len());
                output_file << " ) " << "\n";
                break;
            }

            case SCAN:
            {
                barrier = request->barrier();
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    auto length = request->args_len();
                    output_file << "scan( " << key << ", "<< length << " ): [";
                    for (auto key_i = key; key_i < key+length; key_i++) {
                        char *value;
                        size_t len = read(key, value);
                        if (value == nullptr){
                            error_count_++;
                            break;
                        }
                        output_file.write(value, len);
                        output_file << ", ";

                        delete[] value;
                    }
                    output_file << "]\n";
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
                output_file << "del( " << key << " )\n";
                break;
            }
            case REPARTITION:
                barrier = request->barrier();
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    previous_storage.push_back(storage);
                    storage = new Storage[partitions];
                    output_file << "repartition()\n";
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

            __n_executed_requests++;
            
            output_file.flush();
            delete request;
        }
    }

    int __id;
    size_t __n_executed_requests;
    static Storage *storage;
    cpu_set_t cpu_set;

    bool __executing;
    thread worker_thread_;
    sem_t semaphore_;
    queue<Request*> __requests_queue;
    boost::lockfree::spsc_queue<Request, boost::lockfree::capacity<Capacity>> __bounded_requests_queue;
    mutex __queue_mutex;

    sem_t remaining_space_;

    size_t error_count_ = 0;
    static size_t partitions;
    static vector<Storage*> previous_storage;
    size_t version_count;
    static vector<partition_map_t*> version_maps;
    static shared_mutex version_maps_mtx;

    ofstream output_file;


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
