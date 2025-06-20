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

    int read(T &key, std::string &value);
    void write(T &key, const std::string &value);
    void del(T &key);


private:
    tkrzw::TreeDBM* __storage;
    static std::atomic_int db_counter;
    static std::string id;

};

template<typename T>
inline int TKRZWStorage<T>::read(T &key, std::string &value) {
    tkrzw::Status status = __storage->Get(std::to_string(key), &value);
    if (!status.IsOK()) {
        return -1;
    }
    return value.size();
}

template<typename T>
inline void TKRZWStorage<T>::write(T &key, const std::string &value) {
    __storage->Set(std::to_string(key), value);
}

template<typename T>
inline void TKRZWStorage<T>::del(T &key) {
    __storage->Remove(std::to_string(key));
}

template class kvstorage::TKRZWStorage<int>;

};

#endif
