#include "rocks_db_storage.h"

namespace kvstorage {
void RocksDBStorage::init(){
    rocksdb::Options options;
    options.create_if_missing = true;
    std::string path = 
        std::string("/tmp/repart_kv_storage/") +
        id +
        std::string("/");
    std::filesystem::create_directories(path);
    path += std::to_string(db_counter.fetch_add(1, std::memory_order_relaxed));
    rocksdb::Status status;
    size_t i = 0;
    do {
        status = rocksdb::DB::Open(options, path, &__storage);
    } while(!status.ok() && 10 > i++);
    assert(status.ok());
}

std::atomic_int RocksDBStorage::db_counter = 0;

std::string RocksDBStorage::id = std::to_string(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count()
);
}