#include "level_db_storage.h"

namespace kvstorage {
template<typename T>
void LevelDBStorage<T>::init() {
    leveldb::Options options;
    options.create_if_missing = true;
    std::string path =
        std::string("/tmp/repart_kv_storage/") +
        id +
        std::string("/");
    std::filesystem::create_directories(path);
    path += std::to_string(db_counter.fetch_add(1, std::memory_order_relaxed));

    leveldb::Status status;
    size_t i = 0;
    do {
        status = leveldb::DB::Open(options, path, &__storage);
    } while(!status.ok() && 10 > i++);
    assert(status.ok());
}

template<typename T>
std::atomic_int LevelDBStorage<T>::db_counter = 0;

template<typename T>
std::string LevelDBStorage<T>::id = std::to_string(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count()
);
}