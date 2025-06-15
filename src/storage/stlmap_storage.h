#ifndef _KVPAXOS_STLMAP_STORAGE_H_
#define _KVPAXOS_STLMAP_STORAGE_H_


#include <string>

#include "compresser.h"
#include "storage.h"


namespace kvstorage {
template<typename T, template<typename, typename> class Map_T>
class STLMapStorage : public Storage<T> {

typedef Map_T<T, std::string> storage_t;
public:
    STLMapStorage() {}

    void init(){
        __storage = storage_t();
    };

    int read(T &key, std::string &value);
    void write(T &key, const std::string &value);
    void del(T &key);


private:
    storage_t __storage;

};

template<typename T, template<typename, typename> class Map_T>
inline int STLMapStorage<T, Map_T>::read(T &key, std::string &value) {
    std::string compressed;
    auto it = __storage.find(key);
    if (it != __storage.end()){
        compressed = it->second;
    }
    value = std::move(decompress(compressed));
    return value.length();
}

template<typename T, template<typename, typename> class Map_T>
inline void STLMapStorage<T, Map_T>::write(T &key, const std::string &value) {
    auto compressed_value = compress(value);
    __storage.emplace(key, compressed_value);
}

template<typename T, template<typename, typename> class Map_T>
inline void STLMapStorage<T, Map_T>::del(T &key) {
    auto it = __storage.find(key);
    if (it != __storage.end()){
       __storage.erase(it);
    }
}

};

#endif
