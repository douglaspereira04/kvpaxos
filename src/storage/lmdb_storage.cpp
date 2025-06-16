#include "lmdb_storage.h"

namespace kvstorage {
void LMDBStorage::init(){
    std::string path = 
        std::string("/tmp/repart_kv_storage/") +
        id +
        std::string("/");
    std::filesystem::create_directories(path);
    path += std::to_string(db_counter.fetch_add(1, std::memory_order_relaxed));
    int rc;

    rc = mdb_env_create(&__env);
    if (rc != 0) {
        abort();
    }

    mdb_env_set_maxdbs(__env, 1);
    mdb_env_set_mapsize(__env, 50ULL * 1024 * 1024 * 1024);

    rc = mdb_env_open(__env, path.data(), 0, 0664);
    if (rc != 0) {
        abort();
    }
    mdb_txn_begin(__env, nullptr, 0, &__txn);
    rc = mdb_dbi_open(__txn, nullptr, 0, &__dbi);
    if (rc != 0) {
        abort();
    }
    mdb_txn_commit(__txn);
}

std::atomic_int LMDBStorage::db_counter = 0;

std::string LMDBStorage::id = std::to_string(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count()
);

}