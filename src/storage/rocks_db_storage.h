#ifndef _KVPAXOS_ROCKS_DB_STORAGE_H_
#define _KVPAXOS_ROCKS_DB_STORAGE_H_


#include <string>
#include <atomic>
#include <filesystem>
#include <rocksdb/db.h>

#include "storage.h"

namespace kvstorage {

template<typename T>
class RocksDBStorage : public Storage<T> {
public:
    RocksDBStorage(){}
    void init();

    int read(const T &key, std::string &value);
    void write(const T &key, const std::string &value);
    void del(const T &key);


private:
    rocksdb::DB* __storage;
    static std::atomic_int db_counter;
    static std::string id;

};

template<typename T>
inline int RocksDBStorage<T>::read(const T &key, std::string &value) {
    rocksdb::Status status;
    status = __storage->Get(rocksdb::ReadOptions(), std::to_string(key), &value);
    if (status.IsNotFound()) {
        return -1;
    }
    return value.size();
}

template<typename T>
inline void RocksDBStorage<T>::write(const T &key, const std::string &value) {
    __storage->Put(rocksdb::WriteOptions(), std::to_string(key), value);
}

template<typename T>
inline void RocksDBStorage<T>::del(const T &key) {
    __storage->Delete(rocksdb::WriteOptions(), std::to_string(key));
}

template class kvstorage::RocksDBStorage<int>;

};

#endif
