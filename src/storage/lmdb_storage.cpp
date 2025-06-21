#include "lmdb_storage.h"

namespace kvstorage {
template<typename T>
void LMDBStorage<T>::init(){
    std::string path = 
        std::string("/tmp/repart_kv_storage/") +
        id +
        std::string("/") +
        std::to_string(db_counter.fetch_add(1, std::memory_order_relaxed));
    std::filesystem::create_directories(path);
    int rc;

    rc = mdb_env_create(&__env);
    if (rc != 0) {
        abort();
    }

    mdb_env_set_maxdbs(__env, 1);
    mdb_env_set_mapsize(__env, 50ULL * 1024 * 1024 * 1024);

    rc = mdb_env_open(__env, path.data(), MDB_NOSYNC | MDB_NOMETASYNC, 0664);
    if (rc != 0) {
        abort();
    }
    MDB_txn* __txn;
    mdb_txn_begin(__env, nullptr, 0, &__txn);
    rc = mdb_dbi_open(__txn, nullptr, 0, &__dbi);
    if (rc != 0) {
        abort();
    }
    mdb_txn_commit(__txn);
}

template<typename T>
std::atomic_int LMDBStorage<T>::db_counter = 0;

template<typename T>
std::string LMDBStorage<T>::id = std::to_string(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count()
);

template<typename T>
inline int LMDBStorage<T>::read(const T &key, std::string &value) {
    MDB_txn* __txn;
    MDB_val __key;
    MDB_val __value;

    int rc = mdb_txn_begin(__env, nullptr, MDB_RDONLY, &__txn);
    if (rc != 0) {
        abort();
    }
    __key.mv_size = sizeof(T);
    __key.mv_data = const_cast<void*>(static_cast<const void*>(&key));

    rc = mdb_get(__txn, __dbi, &__key, &__value);
    if (rc == MDB_SUCCESS) {
        value = std::string(reinterpret_cast<char*>(__value.mv_data), __value.mv_size);
    } else {
        return -1;
    }

    mdb_txn_abort(__txn);
    return value.size();
}

template<typename T>
inline void LMDBStorage<T>::write(const T &key, const std::string &value) {
    MDB_txn* __txn;
    MDB_val __key;
    MDB_val __value;
    int rc = mdb_txn_begin(__env, nullptr, 0, &__txn);
    if (rc != 0) {
        abort();
    }

    __key.mv_size = sizeof(T);
    __key.mv_data = const_cast<void*>(static_cast<const void*>(&key));
    __value.mv_size = value.size();
    __value.mv_data = const_cast<void*>(static_cast<const void*>(value.c_str()));

    rc = mdb_put(__txn, __dbi, &__key, &__value, 0);
    if (rc != 0){
        abort();
    }

    rc = mdb_txn_commit(__txn);
    if (rc != 0){
        abort();
    }
}

template<typename T>
inline void LMDBStorage<T>::del(const T &key) {
    MDB_txn* __txn;
    MDB_val __key;
    MDB_val __value;
    int rc = mdb_txn_begin(__env, nullptr, 0, &__txn);
    if (rc != 0) {
        abort();
    }
    __key.mv_size = sizeof(T);
    __key.mv_data = const_cast<void*>(static_cast<const void*>(&key));

    rc = mdb_del(__txn, __dbi, &__key, nullptr);
    if (rc != 0){
        abort();
    }

    rc = mdb_txn_commit(__txn);
    if (rc != 0){
        abort();
    }
}
}