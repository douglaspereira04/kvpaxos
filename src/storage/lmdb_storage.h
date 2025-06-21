#ifndef _KVPAXOS_LMDB_STORAGE_H_
#define _KVPAXOS_LMDB_STORAGE_H_


#include <string>
#include <atomic>
#include <filesystem>
#include <lmdb.h>

#include "storage.h"

namespace kvstorage {

template<typename T>
class LMDBStorage : public Storage<T> {
public:
    LMDBStorage(){}
    void init();

    int read(const T &key, std::string &value);
    void write(const T &key, const std::string &value);
    void del(const T &key);


private:
    MDB_env* __env;
    MDB_dbi __dbi;

    static std::atomic_int db_counter;
    static std::string id;
};

template class kvstorage::LMDBStorage<std::string>;

};

#endif
