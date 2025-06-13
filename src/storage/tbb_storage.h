#ifndef _KVPAXOS_TBB_STORAGE_H_
#define _KVPAXOS_TBB_STORAGE_H_


#include <string>
#include "tbb/concurrent_unordered_map.h"

#include "compresser.h"
#include "storage.h"


namespace kvstorage {

class TBBStorage : public Storage {

typedef tbb::concurrent_unordered_map<std::string, std::string> storage_t;
public:
    TBBStorage() {}

    void init(){
        storage_ = storage_t();
    }

    int read(std::string &key, std::string &value);
    void write(std::string &key, const std::string &value);
    void del(std::string &key);
    std::vector<std::string> scan(std::string &key, size_t len);


private:
    storage_t storage_;

};

inline int TBBStorage::read(std::string &key, std::string &value) {
    std::string compressed;
    try {
         compressed = storage_.at(key);
    } catch(...) {
        return -1;
    }
    value = std::move(decompress(compressed));
    return value.length();
}

inline void TBBStorage::write(std::string &key, const std::string &value) {
    auto compressed_value = compress(value);
    storage_[key] = compressed_value;
}

inline void TBBStorage::del(std::string &key) {
    std::string null_value("null");
    write(key, null_value);
}

};

#endif
