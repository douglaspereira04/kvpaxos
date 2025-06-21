#ifndef _KVPAXOS_UTILS_H_
#define _KVPAXOS_UTILS_H_

#include <thread>
#include <chrono>
#include <unistd.h>
#include <filesystem>
#include <fstream>

#include "types.h"

namespace utils{ 

inline types::time_point now(){
    return std::chrono::_V2::system_clock::now();
}

inline types::duration to_us(types::duration t) {
    return std::chrono::duration_cast<std::chrono::microseconds>(t);
}

/// @brief Set affinity of a std::thread
/// @param cpu is the cpu to set the afinitty of a given thread
/// @param thread is a given thread
/// @param cpu_set will be the new cpu set
void set_affinity(size_t cpu, std::thread &thread, cpu_set_t &cpu_set);

void process_mem_usage(double& vm_usage, double& resident_set);

std::uintmax_t used_disk(const char* path);

#if defined(ANSWER)
	static const bool ENABLE_ANSWER = true;
#else
    static const bool ENABLE_ANSWER = false;
#endif


void read_operation(types::RequestType &type, std::string &key, size_t &len, std::string &value, std::ifstream &file);

}

#endif
