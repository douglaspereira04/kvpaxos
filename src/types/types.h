/*
    Most of LibPaxos structs are forward declarated in header files and
    defined in private cpp, which means we can't access its members.
    Here the same types are declared again so we can access its members.
	There are also new structs designated to the KV application and message
	passing.
*/

#ifndef _KVPAXOS_TYPES_H_
#define _KVPAXOS_TYPES_H_


#include <chrono>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <tbb/concurrent_unordered_map.h>
#include <semaphore.h>


typedef std::chrono::_V2::system_clock::time_point time_point;
typedef std::chrono::_V2::system_clock::duration duration;

struct reply_message {
	int id;
	char answer[1031];
};

struct client_message {
	int id;
	unsigned long s_addr;
	unsigned short sin_port;
	int key;
	int type;
	bool record_timestamp;
	char args[4];
	size_t size;
};
typedef struct client_message client_message;

template<typename key_type, typename value_type>
struct sync_manager_t {
	sync_manager_t(size_t partitions_, size_t size, std::unordered_map<int, std::vector<key_type>> *partition_to_keys_){
		partitions = partitions;
		reached.store(partitions, std::memory_order_relaxed);
		synched.store(partitions, std::memory_order_relaxed);
		sem_init(&sync_sem, 0, 0);
		values = std::vector<value_type>(size);
		partition_to_keys = partition_to_keys_;
	}
    std::atomic_uint_fast32_t reached;
    std::atomic_uint_fast32_t synched;
    sem_t sync_sem;
	std::vector<value_type> values;
	std::unordered_map<int, std::vector<key_type>> *partition_to_keys;
	int partitions;
};
typedef struct sync_manager_t sync_manager_t;


enum request_type
{
	READ,
	WRITE,
	SCAN,
	SYNC,
	ERROR,
	UPDATE,
	DUMMY
};

enum interval_type
{
	MICROSECONDS,
	OPERATIONS
};

#endif
