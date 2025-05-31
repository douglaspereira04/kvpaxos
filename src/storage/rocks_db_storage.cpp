#include "rocks_db_storage.h"

namespace kvstorage {
    RocksDBStorage::RocksDBStorage(){
        rocksdb::Options options;
        db_counter++;
        options.create_if_missing = true;
        std::string path = std::string("/tmp/kvpaxos_storage_") + std::to_string(db_counter);
        rocksdb::Status status = rocksdb::DB::Open(options, path, &__storage);
        assert(status.ok());
    }
    
    std::atomic_int RocksDBStorage::db_counter = 0;
}