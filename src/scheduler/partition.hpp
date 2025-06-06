#ifndef KVPAXOS_PARTITION_H
#define KVPAXOS_PARTITION_H


#include <pthread.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <csignal>
#include <iostream>
#include "types.h"
#include "utils.h"
#include "request.hpp"
#include "rocks_db_storage.h"


static const int VALUE_SIZE = 1024;
static const std::string template_value(VALUE_SIZE, '*');

namespace kvpaxos {
using namespace kvstorage;
using namespace workload;

typedef RocksDBStorage storage_t;

template <typename T, size_t QSize = 0>
class Partition {

typedef Partition<T, QSize> partition_t;
typedef std::unordered_map<T, partition_t*> partition_map_t;
public:
    Partition(int id)
        : __id{id},
          __n_executed_requests{0}
    {
        storage[__id] = storage_t(0);
        __output_file = std::ofstream("partition_output_" + std::to_string(__id));
    }
    
    ~Partition() {
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        __output_file.flush();
        __output_file.close();
    }

    void start_worker_thread() {
        sem_init(&semaphore_, 0, 0);
        if constexpr(QSize > 0){
            sem_init(&remaining_space_, 0, QSize);
        }

        worker_thread_ = std::thread(&partition_t::thread_loop, this);
        utils::set_affinity(__id+5, worker_thread_, cpu_set);
    }

    size_t request_queue_size() const {
        return __requests_queue.size();
    }

    size_t error_count() {
        return error_count_;
    }

    void push_request(Request *request) {
        if constexpr(QSize > 0){
            sem_wait(&remaining_space_);
        }
        __queue_mutex.lock();
            __requests_queue.push(request);
        __queue_mutex.unlock();
        sem_post(&semaphore_);
    }

    Request * pop_request() {
        Request *request;
        sem_wait(&semaphore_);

        __queue_mutex.lock();
            request = __requests_queue.front();
            __requests_queue.pop();
        __queue_mutex.unlock();
        if constexpr(QSize > 0){
            sem_post(&remaining_space_);
        }
        return request;
    }


    int id() const {
        return __id;
    }

    size_t n_executed_requests() const {
        return __n_executed_requests;
    }


    static void add_old_partition_map(std::unordered_map<int, partition_t*>* version_map){
        version_maps_mtx.lock();
        version_maps.push_back(version_map);
        version_maps_mtx.unlock();
    }

    static void create_storage(size_t partitions_){
        partitions = partitions_;
        storage = new storage_t[partitions];
    }
private:

    int read(int key, std::string& val){
        int len = storage[__id].read(key, val);
        int len_old = -1;
        storage_t* past_storage;
        int past_id;
        if (len < 0){
            for (int i = version_count-1; i >= 0; i--)
            {
                version_maps_mtx.lock_shared();
                std::unordered_map<int, partition_t*> *map = version_maps.at(i);
                version_maps_mtx.unlock_shared();
                auto partition = map->find(key);
                if (partition != map->end()){
                    past_id = partition->second->__id;
                    past_storage = previous_storage[i];
                    len_old = past_storage[past_id].read(key, val);
                    if (len_old >= 0){
                        past_storage[past_id].del(key);
                        storage[__id].write(key, val);
                        return len_old;
                    }
                }
            }
            
        }
        if (len < 0 && len_old < 0){
            exit(EXIT_FAILURE);
        }
        return len;
    }

    inline void scan_some(Request* request, int &key){
        for (size_t i = 0; i < request->args_len(); i++)
        {
            if (request->key_in_partition(i, this)){
                int key_i = key + i;
                int len = read(key_i, request->get_scaned_value(i));
                if (len < 0){
                    error_count_++;
                    continue;
                }
            }
        }
        

    }
    inline void scan(Request* request, int &key){
        auto length = request->args_len();

        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "scan( " << key << ", "<< length << " ): [";
        }
        for (auto key_i = key; key_i < key+length; key_i++) {
            std::string value;
            int len = read(key, value);
            if (len < 0){
                error_count_++;
                continue;
            }

            if constexpr(utils::ENABLE_ANSWER){
                 __output_file << "\"" << value << "\", ";
            }
        }

        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "]\n";
        }
    }

    void thread_loop() {
        while (true) {

            Request *request = pop_request();

            RequestType type = request->type();
            auto key = request->key();
            int coordinator = 0;
            switch (type)
            {
            case READ:
            {   
                std::string value;
                int len = read(key, value);
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "read( " << key << " ): " << value << "\n";
                }
                if (len < 0) {
                    error_count_++;
                    continue;
                }
                delete request;
                __n_executed_requests++;
                break;
            }

            case WRITE:
            {
                if constexpr(utils::ENABLE_ANSWER){
                    const std::string value = request->get_write_value();
                    storage[__id].write(key, value);
                    __output_file << "write( " << key << ", " << value << " )\n";
                    request->destroy_write();
                } else {
                    storage[__id].write(key, template_value);
                }
                delete request;
                __n_executed_requests++;
                break;
            }

            case SCAN:
            {
                if (request->is_multi_partition()){
                    scan_some(request, key);
                    if (request->is_coordinator()) {
                        if constexpr(utils::ENABLE_ANSWER){
                            __output_file << "scan( " << key << ", "<< request->args_len() << " ): [";
                            for (size_t i = 0; i < request->args_len(); i++)
                            {
                                __output_file << "\"" << request->get_scaned_value(i) << "\",";
                            }
                            __output_file << "]\n";
                        }
                        request->destroy_multi_partition_scan();
                        delete request;
                        __n_executed_requests++;
                    }
                } else {
                    scan(request, key);
                    delete request;
                    __n_executed_requests++;
                }
                break;
            }

            case DEL:
            {
                storage[__id].del(key);
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "del( " << key << " )\n";
                }
                delete request;
                __n_executed_requests++;
                break;
            }
            case REPARTITION:
                coordinator = request->barrier_wait();
                if (coordinator == PTHREAD_BARRIER_SERIAL_THREAD) {
                    previous_storage.push_back(storage);
                    version_count++;
                    storage = new storage_t[partitions];
                    if constexpr(utils::ENABLE_ANSWER){
                        __output_file << "repartition() \n";
                    }
                }
                coordinator = request->barrier_wait();
                if (coordinator == PTHREAD_BARRIER_SERIAL_THREAD) {
                    request->destroy_barrier();
                    delete request;
                }
                storage[__id] = storage_t(version_count);
                break;
            case ERROR:
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "err() \n";
                }
                delete request;
                break;
            case END:
                __output_file << "end() \n";
                delete request;
                return;
            default:
                delete request;
                std::raise(SIGINT);
                return;
                break;
            }
        }
    }

    int __id;
    size_t __n_executed_requests;
    static storage_t *storage;
    cpu_set_t cpu_set;

    std::thread worker_thread_;
    sem_t semaphore_;
    std::queue<Request*> __requests_queue;
    std::mutex __queue_mutex;

    sem_t remaining_space_;

    size_t error_count_ = 0;
    static size_t partitions;
    static std::vector<storage_t*> previous_storage;
    static int version_count;
    static std::vector<partition_map_t*> version_maps;
    static std::shared_mutex version_maps_mtx;

    std::ofstream __output_file;


};
template<typename T, size_t QSize>
std::vector<storage_t*> Partition<T, QSize>::previous_storage;

template<typename T, size_t QSize>
int Partition<T, QSize>::version_count = 0;

template<typename T, size_t QSize>
size_t Partition<T, QSize>::partitions = 0;

template<typename T, size_t QSize>
storage_t* Partition<T, QSize>::storage;

template<typename T, size_t QSize>
std::vector<std::unordered_map<T, Partition<T, QSize>*>*> Partition<T, QSize>::version_maps;

template<typename T, size_t QSize>
std::shared_mutex Partition<T, QSize>::version_maps_mtx;

}

#endif
