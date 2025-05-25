/*
    Most of LibPaxos structs are forward declarated in header files and
    defined in private cpp, which means we can't access its members.
    Here the same types are declared again so we can access its members.
	There are also new structs designated to the KV application and message
	passing.
*/

#include <chrono>
#ifndef _GEN_TYPES_H_
#define _GEN_TYPES_H_


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

enum request_type
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
