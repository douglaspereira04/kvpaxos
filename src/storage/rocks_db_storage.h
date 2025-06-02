#ifndef _KVPAXOS_ROCKS_DB_STORAGE_H_
#define _KVPAXOS_ROCKS_DB_STORAGE_H_


#include <string>
#include <atomic>
#include <filesystem>
#include <rocksdb/db.h>

#include "storage.h"

namespace kvstorage {

class RocksDBStorage : public Storage {
public:
    RocksDBStorage(){}
    RocksDBStorage(size_t version);

    int read(int key, std::string &value);
    void write(int key, const std::string &value);
    void del(int key);


private:
    rocksdb::DB* __storage;
    static std::atomic_int db_counter;
    static std::string id;

};

inline int RocksDBStorage::read(int key, std::string &value) {
    rocksdb::Status status;
    size_t i = 0;
    do {
        status = __storage->Get(rocksdb::ReadOptions(), std::to_string(key), &value);
    } while(!status.ok() && 10 > i++);
    assert(status.ok());
    if (status.IsNotFound()) {
        return -1;
    }
    return value.size();
}

inline void RocksDBStorage::write(int key, const std::string &value) {
    rocksdb::Status status;
    size_t i = 0;
    do {
        status = __storage->Put(rocksdb::WriteOptions(), std::to_string(key), value);
    } while(!status.ok() && 10 > i++);
    assert(status.ok());
}

inline void RocksDBStorage::del(int key) {
    rocksdb::Status status;
    size_t i = 0;
    do {
        status = __storage->Delete(rocksdb::WriteOptions(), std::to_string(key));
    } while(!status.ok() && 10 > i++);
    assert(status.ok());
}

};

#endif
