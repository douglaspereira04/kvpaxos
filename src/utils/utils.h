#ifndef _KVPAXOS_UTILS_H_
#define _KVPAXOS_UTILS_H_

#include <thread>
#include <chrono>
#include <fstream>
#include <unistd.h>
#include <filesystem>

#include "types.h"

namespace utils{ 

inline time_point now(){
    return std::chrono::_V2::system_clock::now();
}

inline duration to_us(duration t) {
    return std::chrono::duration_cast<std::chrono::microseconds>(t);
}

/// @brief Set affinity of a std::thread
/// @param cpu is the cpu to set the afinitty of a given thread
/// @param thread is a given thread
/// @param cpu_set will be the new cpu set
void set_affinity(size_t cpu, std::thread &thread, cpu_set_t &cpu_set);

void process_mem_usage(double& vm_usage, double& resident_set);




template<typename T>
inline T* unmarked(T* &pointer){
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pointer) & ~0x1);
}

template<typename T>
inline T* marked(T* &pointer){
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pointer) | 0x1);
}

template<typename T>
inline bool is_marked(T* &pointer){
    return reinterpret_cast<uintptr_t>(pointer) & 0x1;
}

std::uintmax_t used_disk(const char* path);

#if defined(INFO)
	static const bool ENABLE_INFO = true;
#else
    static const bool ENABLE_INFO = false;
#endif

#if defined(ANSWER)
	static const bool ENABLE_ANSWER = true;
#else
    static const bool ENABLE_ANSWER = false;
#endif

#if defined(LINEARIZABLE)
	static const bool ENABLE_LINEARIZABLE = true;
#else
    static const bool ENABLE_LINEARIZABLE = false;
#endif

}

#endif
