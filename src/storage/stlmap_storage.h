#ifndef _KVPAXOS_STLMAP_STORAGE_H_
#define _KVPAXOS_STLMAP_STORAGE_H_


#include <string>
#include <vector>

#include "compresser.h"
#include "storage.h"


namespace kvstorage {
template<template<typename, typename> class Map_T>
class STLMapStorage : public Storage {

typedef Map_T<std::string, std::string> storage_t;
public:
    STLMapStorage() {}

    void init(){
        __storage = storage_t();
    };

    int read(std::string &key, std::string &value);
    void write(std::string &key, const std::string &value);
    void del(std::string &key);
    std::vector<std::string> scan(std::string &key, size_t len);


private:
    storage_t __storage;

};

template<template<typename, typename> class Map_T>
inline int STLMapStorage<Map_T>::read(std::string &key, std::string &value) {
    std::string compressed;
    auto it = __storage.find(key);
    if (it != __storage.end()){
        compressed = it->second;
    }
    value = std::move(decompress(compressed));
    return value.length();
}

template<template<typename, typename> class Map_T>
inline void STLMapStorage<Map_T>::write(std::string &key, const std::string &value) {
    auto compressed_value = compress(value);
    __storage.emplace(key, compressed_value);
}

template<template<typename, typename> class Map_T>
inline std::vector<std::string> STLMapStorage<Map_T>::scan(std::string &key, size_t len) {
    std::vector<std::string> values;

    auto it = __storage.lower_bound(key);
    size_t count = 0;

    while (it != __storage.end() && count < len) {
        values.push_back(it->second);
        ++it;
        ++count;
    }
    
    return values;
}

template<template<typename, typename> class Map_T>
inline void STLMapStorage<Map_T>::del(std::string &key) {
    auto it = __storage.find(key);
    if (it != __storage.end()){
       __storage.erase(it);
    }
}

};

#endif
