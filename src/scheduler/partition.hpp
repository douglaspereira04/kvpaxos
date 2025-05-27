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
#include "request/request.hpp"
#include "storage/storage.h"
#include "types/types.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>
#include "utils/utils.h"
#include <fstream>

#include <csignal>
#include <iostream>

namespace kvpaxos {
using namespace kvstorage;
using namespace workload;

template <typename T, size_t Capacity = 0>
class Partition {
typedef std::unordered_map<T, Partition<T, Capacity>*> partition_map_t;
public:
    Partition(int id)
        : __id{id},
          __n_executed_requests{0}
    {
        storage[__id] = Storage();
        __output_file = std::ofstream("partition_output_" + std::to_string(__id));
    }

    ~Partition() {
        if (worker_thread_.joinable()) {
            sem_post(&semaphore_);
            worker_thread_.join();
        }
        __output_file.flush();
        __output_file.close();
    }

    void start_worker_thread() {
        sem_init(&semaphore_, 0, 0);
        if constexpr(Capacity > 0){
            sem_init(&remaining_space_, 0, Capacity);
        }

        worker_thread_ = std::thread(&Partition<T, Capacity>::thread_loop, this);
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
        int len = storage[__id].read(key, val);
        int len_old = -1;
        Storage* past_storage;
        int past_id;
        if (len < 0){
            for (int i = version_count-1; i >= 0; i--)
            {
                version_maps_mtx.lock_shared();
                partition_map_t *map = version_maps.at(i);
                version_maps_mtx.unlock_shared();
                auto partition = map->find(key);
                if (partition != map->end()){
                    past_id = partition->second->__id;
                    past_storage = previous_storage[i];
                    len_old = past_storage[past_id].read(key, val);
                    if (len_old >= 0){
                        break;
                    }
                }
            }
            
        }

        if (len < 0 && len_old >= 0) {
            past_storage[past_id].del(key);
            storage[__id].write(key, val, len_old);
            return len_old;
        }
        if (len < 0 && len_old < 0){
            exit(EXIT_FAILURE);
        }
        return len;
    }

    inline void scan_some(Request* request, int &key, size_t &length, char** &values, size_t* &values_lengths){
        Partition<T,Capacity>** key_to_partition;
        request->get_key_to_partition(key_to_partition);

        if (key_to_partition[0] == this){
            for (size_t i = 0; i < length; i++)
            {
                if (key_to_partition[i] == this || key_to_partition[i] == nullptr){
                    int key_i = key + i;
                    values_lengths[i] = read(key_i, values[i]);
                }
            }
        } else {
            for (size_t i = 0; i < length; i++)
            {
                if (key_to_partition[i] == this){
                    int key_i = key + i;
                    values_lengths[i] = read(key_i, values[i]);
                }
            }
        }
        

    }
    inline void scan(Request* request, int &key){
        auto length = request->args_len();
        __output_file << "scan( " << key << ", "<< length << " ): [";
        for (auto key_i = key; key_i < key+length; key_i++) {
            char *value;
            size_t len = read(key, value);
            if (len < 0){
                error_count_++;
                continue;
            }
            for (size_t i = 0; i < len; i++)
            {
                __output_file << value[i];
            }
            __output_file << ", ";

            delete[] value;
        }
        __output_file << "]\n";
    }

    void thread_loop() {
        while (true) {

            Request *request = pop_request();

            RequestType type = request->type();
            auto key = request->key();
            int coordinator = 0;
            pthread_barrier_t * barrier;
            switch (type)
            {
            case READ:
            {   
                char *value;
                size_t len = read(key, value);
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "read( " << key << " ): ";
                    for (size_t i = 0; i < len; i++)
                    {
                        __output_file << value[i];
                    }
                    __output_file << "\n";
                }
                if (len >= 0) {
                    delete[] value;
                }
                delete request;
                __n_executed_requests++;
                break;
            }

            case WRITE:
            {
                size_t len = request->args_len();
                char *value = request->args();
                storage[__id].write(key, value, len);
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "write( " << key << ", ";
                    for (size_t i = 0; i < len; i++)
                    {
                        __output_file << value[i];
                    }
                    __output_file << " )\n";
                }
                delete request;
                __n_executed_requests++;
                break;
            }

            case SCAN:
            {
                barrier = request->barrier();
                size_t length = request->args_len();
                char** values;
                size_t* values_lengths;
                request->get_values(values);
                request->get_value_lengths(values_lengths);
                if (barrier != nullptr){
                    scan_some(request, key, length, values, values_lengths);
                    coordinator = pthread_barrier_wait(barrier);
                    if (coordinator) {
                        if constexpr(utils::ENABLE_ANSWER){
                            __output_file << "scan( " << key << ", "<< length << " ): [";
                            for (size_t i = 0; i < length; i++)
                            {
                                __output_file << "\"";
                                for (size_t j = 0; j < values_lengths[i]; j++)
                                {
                                    __output_file << values[i][j];
                                }
                                __output_file << "\",";
                            }
                            __output_file << "]\n";
                        }
                        pthread_barrier_destroy(barrier);
                        delete request;
                    }
                } else {
                    scan(request, key);
                    delete request;
                }
                __n_executed_requests++;
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
                barrier = request->barrier();
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    previous_storage.push_back(storage);
                    version_count++;
                    storage = new Storage[partitions];
                    if constexpr(utils::ENABLE_ANSWER){
                        __output_file << "repartition() \n";
                    }
                }
                coordinator = pthread_barrier_wait(barrier);
                if (coordinator) {
                    pthread_barrier_destroy(barrier);
                    delete request;
                }
                storage[__id] = Storage();
                break;
            case ERROR:
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "err() \n";
                }
                delete request;
                break;
            default:
                delete request;
                return;
                break;
            }
        }
    }

    int __id;
    size_t __n_executed_requests;
    static Storage *storage;
    cpu_set_t cpu_set;

    std::thread worker_thread_;
    sem_t semaphore_;
    std::queue<Request*> __requests_queue;
    boost::lockfree::spsc_queue<Request*, boost::lockfree::capacity<Capacity>> __bounded_requests_queue;
    std::mutex __queue_mutex;

    sem_t remaining_space_;

    size_t error_count_ = 0;
    static size_t partitions;
    static std::vector<Storage*> previous_storage;
    static int version_count;
    static std::vector<partition_map_t*> version_maps;
    static std::shared_mutex version_maps_mtx;

    std::ofstream __output_file;


};
template<typename T, size_t Capacity>
std::vector<Storage*> Partition<T, Capacity>::previous_storage;

template<typename T, size_t Capacity>
int Partition<T, Capacity>::version_count = 0;

template<typename T, size_t Capacity>
size_t Partition<T, Capacity>::partitions = 0;

template<typename T, size_t Capacity>
Storage* Partition<T, Capacity>::storage;

template<typename T, size_t Capacity>
std::vector<std::unordered_map<T, Partition<T, Capacity>*>*> Partition<T, Capacity>::version_maps;

template<typename T, size_t Capacity>
std::shared_mutex Partition<T, Capacity>::version_maps_mtx;

}

#endif
