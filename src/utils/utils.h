#ifndef _KVPAXOS_UTILS_H_
#define _KVPAXOS_UTILS_H_

#include <thread>

namespace utils{ 

/// @brief Set affinity of a std::thread
/// @param cpu is the cpu to set the afinitty of a given thread
/// @param thread is a given thread
/// @param cpu_set will be the new cpu set
void set_affinity(size_t cpu, std::thread &thread, cpu_set_t &cpu_set);

#if defined(INFO)
	static const bool ENABLE_INFO = true;
#else
    static const bool ENABLE_INFO = false;
#endif

}

#endif
