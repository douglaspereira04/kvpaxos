add_library(storage
    rocks_db_storage.cpp
    storage.cpp
    tbb_storage.cpp
)

target_include_directories(storage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link to RocksDB if only this module needs it
target_link_libraries(storage
    PUBLIC
        compresser
        rocksdb
        TBB::tbb
)