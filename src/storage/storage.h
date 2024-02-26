#ifndef _KVPAXOS_STORAGE_H_
#define _KVPAXOS_STORAGE_H_


#include <string>
#include <unordered_map>
#include <vector>

#include "compresser/compresser.h"
#include "types/types.h"
#include "tbb/concurrent_unordered_map.h"

typedef tbb::concurrent_unordered_map<int, std::string> storage_t;

namespace kvstorage {

class Storage {
public:
    Storage() = default;

    std::string read(int key);
    void write(int key, const std::string& value);
    std::vector<std::string> scan(int start, int length);

private:
    storage_t storage_ = storage_t();

};

};

#endif
