#include "utils.h"
#include <thread>
#include <assert.h>

namespace utils{ 

void set_affinity(size_t cpu, std::thread &thread, cpu_set_t &cpu_set){
	//CPU_ZERO(&cpu_set);
	//CPU_SET(cpu, &cpu_set);
	//assert(pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpu_set) == 0);
}

std::uintmax_t used_disk(const char* path) {
    std::filesystem::space_info si = std::filesystem::space(path);
	return si.capacity - si.free;
}

void process_mem_usage(double& vm_usage, double& resident_set)
{
	// by https://gist.github.com/thirdwing/da4621eb163a886a03c5
    vm_usage     = 0.0;
    resident_set = 0.0;

    // the two fields we want
    unsigned long vsize;
    long rss;
    {
        std::string ignore;
        std::ifstream ifs("/proc/self/stat", std::ios_base::in);
        ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
                >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
                >> ignore >> ignore >> vsize >> rss;
    }

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;
}

}