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

    size_t read(int key, char* &value);
    void write(int key, const char *value, size_t len);
    void del(int key);


private:
    storage_t storage_ = storage_t();

};

};

#endif
