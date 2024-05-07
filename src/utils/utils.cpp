#include "utils.h"
#include <thread>
#include <assert.h>

namespace utils{ 

void set_affinity(size_t cpu, std::thread &thread, cpu_set_t &cpu_set){
	CPU_ZERO(&cpu_set);
	CPU_SET(cpu, &cpu_set);
	assert(pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpu_set) == 0);
}

}