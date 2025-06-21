#ifndef _KVPAXOS_TKRZW_STORAGE_H_
#define _KVPAXOS_TKRZW_STORAGE_H_


#include <string>
#include <atomic>
#include <filesystem>
#include <cassert>
#include "tkrzw_dbm_tree.h"

#include "storage.h"

namespace kvstorage {

template<typename T>
class TKRZWStorage : public Storage<T> {
public:
    TKRZWStorage(){}
    void init();

    int read(const T &key, std::string &value);
    void write(const T &key, const std::string &value);
    void del(const T &key);
    std::vector<T> scan(const T &key, size_t len);


private:
    tkrzw::TreeDBM* __storage;
    static std::atomic_int db_counter;
    static std::string id;

};

template<typename T>
inline int TKRZWStorage<T>::read(const T &key, std::string &value) {
    tkrzw::Status status = __storage->Get(key, &value);
    if (!status.IsOK()) {
        abort();
        return -1;
    }
    return value.size();
}

template<typename T>
std::vector<T> TKRZWStorage<T>::scan(const T &key, size_t len) {
    std::vector<T> result;
    auto iter = __storage->MakeIterator();
    iter->Jump(key);

    std::string found_key;
    std::string found_value;
    size_t count = 0;

    while (count < len) {
        tkrzw::Status status = iter->Get(&found_key, &found_value);
        if (!status.IsOK()) {
            break;
        }
        result.push_back(found_value);
        count++;

        status = iter->Next();
        if (!status.IsOK()) {
            break;
        }
    }

    return result;
}

template<typename T>
inline void TKRZWStorage<T>::write(const T &key, const std::string &value) {
    __storage->Set(key, value);
}

template<typename T>
inline void TKRZWStorage<T>::del(const T &key) {
    __storage->Remove(key);
}

template class kvstorage::TKRZWStorage<std::string>;

};

#endif
