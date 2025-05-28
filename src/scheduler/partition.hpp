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
        storage = Storage();
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

    void join(){
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

        worker_thread_ = std::thread(&Partition<T, Capacity>::thread_loop, this);
        utils::set_affinity(1, worker_thread_, cpu_set);
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

private:

    inline void scan(Request* request, int &key){
        auto length = request->args_len();

        if constexpr(utils::ENABLE_ANSWER){
            __output_file << "scan( " << key << ", "<< length << " ): [";
        }
        for (auto key_i = key; key_i < key+length; key_i++) {
            std::string *value;
            int len = storage.read(key, value);
            if (len < 0){
                error_count_++;
                continue;
            }

            if constexpr(utils::ENABLE_ANSWER){
                 __output_file << "\"" << *value << "\", ";
            }

            delete value;
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
            switch (type)
            {
            case READ:
            {   
                std::string *value;
                int len = storage.read(key, value);
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "read( " << key << " ): " << *value << "\n";
                }
                if (len >= 0) {
                    delete value;
                }
                delete request;
                __n_executed_requests++;
                break;
            }

            case WRITE:
            {
                std::string *value = request->get_value_string();
                storage.write(key, value);
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "write( " << key << ", " << *value << " )\n";
                }
                delete request;
                __n_executed_requests++;
                break;
            }

            case SCAN:
            {
                scan(request, key);
                delete request;
                __n_executed_requests++;
                break;
            }

            case DEL:
            {
                storage.del(key);
                if constexpr(utils::ENABLE_ANSWER){
                    __output_file << "del( " << key << " )\n";
                }
                delete request;
                __n_executed_requests++;
                break;
            }
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
    static Storage storage;
    cpu_set_t cpu_set;

    std::thread worker_thread_;
    sem_t semaphore_;
    std::queue<Request*> __requests_queue;
    boost::lockfree::spsc_queue<Request*, boost::lockfree::capacity<Capacity>> __bounded_requests_queue;
    std::mutex __queue_mutex;

    sem_t remaining_space_;

    size_t error_count_ = 0;
    std::ofstream __output_file;


};

template<typename T, size_t Capacity>
Storage Partition<T, Capacity>::storage;

}

#endif
