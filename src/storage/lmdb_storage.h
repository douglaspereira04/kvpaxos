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

    int read(T &key, std::string &value);
    void write(T &key, const std::string &value);
    void del(T &key);


private:
    MDB_env* __env;
    MDB_dbi __dbi;
    MDB_txn* __txn;
    MDB_val __key;
    MDB_val __value;

    static std::atomic_int db_counter;
    static std::string id;
};

template class kvstorage::LMDBStorage<int>;

};

#endif
