#ifndef _KVPAXOS_TBB_STORAGE_H_
#define _KVPAXOS_TBB_STORAGE_H_


#include <string>
#include "tbb/concurrent_unordered_map.h"

#include "compresser.h"
#include "storage.h"


namespace kvstorage {
template<typename T>
class TBBStorage : public Storage<T> {

typedef tbb::concurrent_unordered_map<T, std::string> storage_t;
public:
    TBBStorage() {}

    void init(){
        storage_ = storage_t();
    };

    int read(T &key, std::string &value);
    void write(T &key, const std::string &value);
    void del(T &key);


private:
    storage_t storage_;

};

template<typename T>
inline int TBBStorage<T>::read(T &key, std::string &value) {
    std::string compressed;
    try {
         compressed = storage_.at(key);
    } catch(...) {
        return -1;
    }
    value = std::move(decompress(compressed));
    return value.length();
}

template<typename T>
inline void TBBStorage<T>::write(T &key, const std::string &value) {
    auto compressed_value = compress(value);
    storage_[key] = compressed_value;
}

template<typename T>
inline void TBBStorage<T>::del(T &key) {
    std::string null_value("null");
    write(key, null_value);
}

};

#endif
