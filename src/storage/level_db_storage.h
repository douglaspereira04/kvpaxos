#ifndef _KVPAXOS_LEVEL_DB_STORAGE_H_
#define _KVPAXOS_LEVEL_DB_STORAGE_H_

#include <string>
#include <vector>
#include <atomic>
#include <filesystem>
#include <leveldb/db.h>

#include "storage.h"

namespace kvstorage {

template<typename T>
class LevelDBStorage : public Storage<T> {
public:
    LevelDBStorage(){}
    void init();

    int read(const T &key, std::string &value);
    void write(const T &key, const std::string &value);
    void del(const T &key);
    std::vector<T> scan(const T &key, size_t len);

private:
    leveldb::DB* __storage;
    static std::atomic_int db_counter;
    static std::string id;
};

template<typename T>
inline int LevelDBStorage<T>::read(const T &key, std::string &value) {
    leveldb::Status status;
    status = __storage->Get(leveldb::ReadOptions(), key, &value);
    if (status.IsNotFound()) {
        return -1;
    }
    return value.size();
}


template<typename T>
inline std::vector<T> LevelDBStorage<T>::scan(const T &key, size_t len) {
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    read_options.snapshot = __storage->GetSnapshot();
    std::unique_ptr<leveldb::Iterator> it(__storage->NewIterator(read_options));
    std::vector<T> values;
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

template<typename T>
inline void LevelDBStorage<T>::write(const T &key, const std::string &value) {
    __storage->Put(leveldb::WriteOptions(), key, value);
}

template<typename T>
inline void LevelDBStorage<T>::del(const T &key) {
    __storage->Delete(leveldb::WriteOptions(), key);
}

template class kvstorage::LevelDBStorage<std::string>;

};

#endif
