#ifndef _KVPAXOS_LMDB_STORAGE_H_
#define _KVPAXOS_LMDB_STORAGE_H_


#include <string>
#include <atomic>
#include <vector>
#include <filesystem>
#include <lmdb.h>

#include "storage.h"

namespace kvstorage {

class LMDBStorage : public Storage {
public:
    LMDBStorage(){}
    void init();

    int read(std::string &key, std::string &value);
    void write(std::string &key, const std::string &value);
    void del(std::string &key);
    std::vector<std::string> scan(std::string &key, size_t len);


private:
    MDB_env* __env;
    MDB_dbi __dbi;
    MDB_txn* __txn;
    MDB_val __key;
    MDB_val __value;

    static std::atomic_int db_counter;
    static std::string id;
};



inline int LMDBStorage::read(std::string &key, std::string &value) {

    int rc = mdb_txn_begin(__env, nullptr, MDB_RDONLY, &__txn);
    if (rc != 0) {
        abort();
    }
    __key.mv_size = key.size();
    __key.mv_data = const_cast<void*>(static_cast<const void*>(key.data()));

    rc = mdb_get(__txn, __dbi, &__key, &__value);
    if (rc == MDB_SUCCESS) {
        value = std::string(reinterpret_cast<char*>(__value.mv_data), __value.mv_size);
    } else {
        return -1;
    }

    mdb_txn_abort(__txn);
    return value.size();
}

inline std::vector<std::string> LMDBStorage::scan(std::string &key, size_t len) {
    std::vector<std::string> result;

    int rc = mdb_txn_begin(__env, nullptr, MDB_RDONLY, &__txn);
    if (rc != 0) {
        abort();
    }

    MDB_cursor* cursor;
    rc = mdb_cursor_open(__txn, __dbi, &cursor);
    if (rc != 0) {
        mdb_txn_abort(__txn);
        abort();
    }

    __key.mv_size = key.size();
    __key.mv_data = const_cast<void*>(static_cast<const void*>(key.data()));

    rc = mdb_cursor_get(cursor, &__key, &__value, MDB_SET_RANGE);

    size_t count = 0;
    while (rc == MDB_SUCCESS && count < len) {
        std::string value(reinterpret_cast<char*>(__value.mv_data), __value.mv_size);
        result.push_back(value);

        rc = mdb_cursor_get(cursor, &__key, &__value, MDB_NEXT);
        count++;
    }

    mdb_cursor_close(cursor);
    mdb_txn_abort(__txn);

    return result;
}

inline void LMDBStorage::write(std::string &key, const std::string &value) {
    int rc = mdb_txn_begin(__env, nullptr, 0, &__txn);
    if (rc != 0) {
        abort();
    }

    __key.mv_size = key.size();
    __key.mv_data = const_cast<void*>(static_cast<const void*>(key.data()));
    __value.mv_size = value.size();
    __value.mv_data = const_cast<void*>(static_cast<const void*>(value.data()));

    rc = mdb_put(__txn, __dbi, &__key, &__value, 0);

    mdb_txn_commit(__txn);
}

inline void LMDBStorage::del(std::string &key) {
    int rc = mdb_txn_begin(__env, nullptr, 0, &__txn);
    if (rc != 0) {
        abort();
    }
    __key.mv_size = key.size();
    __key.mv_data = const_cast<void*>(static_cast<const void*>(key.data()));

    rc = mdb_del(__txn, __dbi, &__key, nullptr);

    mdb_txn_commit(__txn);
}

};

#endif
