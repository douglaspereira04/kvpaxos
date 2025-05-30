#ifndef _KVPAXOS_ROCKS_DB_STORAGE_H_
#define _KVPAXOS_ROCKS_DB_STORAGE_H_


#include <string>
#include <atomic>
#include <rocksdb/db.h>

#include "storage.h"

namespace kvstorage {

class RocksStorage : public Storage {
public:
    RocksStorage(){
        rocksdb::Options options;
        db_counter++;
        options.create_if_missing = true;
        std::string path = std::string("/tmp/kvpaxos_storage_") + std::to_string(db_counter);
        rocksdb::Status status = rocksdb::DB::Open(options, path, &__storage);
        assert(status.ok());
    }

    int read(int key, std::string* &value);
    void write(int key, std::string *value);
    void del(int key);


private:
    rocksdb::DB* __storage;

    static std::atomic_int db_counter;

};

std::atomic_int RocksStorage::db_counter = 0;

inline int RocksStorage::read(int key, std::string* &value) {
    rocksdb::Status status = __storage->Get(rocksdb::ReadOptions(), std::to_string(key), value);
    if (status.IsNotFound()) {
        value = nullptr;
        return -1;
    }
    return value->size();
}

inline void RocksStorage::write(int key, std::string *value) {
    rocksdb::Status status = __storage->Put(rocksdb::WriteOptions(), std::to_string(key), *value);
    assert(status.ok());
}

inline void RocksStorage::del(int key) {
    rocksdb::Status status = __storage->Delete(rocksdb::WriteOptions(), std::to_string(key));
    assert(status.ok());
}

};

#endif
