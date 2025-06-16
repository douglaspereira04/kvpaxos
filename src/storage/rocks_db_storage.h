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
    void init();

    int read(std::string &key, std::string &value);
    void write(std::string &key, const std::string &value);
    void del(std::string &key);
    std::vector<std::string> scan(std::string &key, size_t len);

private:
    rocksdb::DB* __storage;
    static std::atomic_int db_counter;
    static std::string id;

};

inline int RocksDBStorage::read(std::string &key, std::string &value) {
    rocksdb::Status status;
    status = __storage->Get(rocksdb::ReadOptions(), key, &value);
    if (status.IsNotFound()) {
        return -1;
    }
    return value.size();
}

inline std::vector<std::string> RocksDBStorage::scan(std::string &key, size_t len) {
    rocksdb::Status status;
    rocksdb::ReadOptions read_options;
    read_options.snapshot = __storage->GetSnapshot();
    std::unique_ptr<rocksdb::Iterator> it(__storage->NewIterator(read_options));
    std::vector<std::string> values;
    values.reserve(len);
    for (it->Seek(key); it->Valid() && len > 0; it->Next(), len--) {
        values.push_back(it->value().ToString());
    }
    __storage->ReleaseSnapshot(read_options.snapshot);
    if (!it->status().ok()) {
        abort();
    }
    return values;
}

inline void RocksDBStorage::write(std::string &key, const std::string &value) {
    __storage->Put(rocksdb::WriteOptions(), key, value);
}

inline void RocksDBStorage::del(std::string &key) {
    __storage->Delete(rocksdb::WriteOptions(), key);
}

};

#endif
