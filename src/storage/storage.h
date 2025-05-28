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

    int read(int key, std::string* &value);
    void write(int key, std::string *value);
    void del(int key);


private:
    storage_t storage_ = storage_t();

};

inline int Storage::read(int key, std::string* &value) {
    std::string compressed;
    try {
         compressed = storage_.at(key);
    } catch(...) {
        value = nullptr;
        return -1;
    }
    value = new std::string(std::move(decompress(compressed)));
    int len = value->length();
    return len;
}

inline void Storage::write(int key, std::string *value) {
    auto compressed_value = compress(*value);
    storage_[key] = compressed_value;
}

inline void Storage::del(int key) {
    std::string null_value("null");
    write(key, &null_value);
}

};

#endif
