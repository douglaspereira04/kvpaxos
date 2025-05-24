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
#include <unordered_set>
#include <tbb/concurrent_unordered_map.h>


typedef std::chrono::_V2::system_clock::time_point time_point;
typedef std::chrono::_V2::system_clock::duration duration;

enum RequestType
{
	READ,
	WRITE,
	SCAN,
	DEL,
	REPARTITION,
	END,
	DUMMY,
	ERROR
};


enum interval_type
{
	MICROSECONDS,
	OPERATIONS
	
};

#endif
