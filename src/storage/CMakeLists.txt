add_library(storage)

target_sources(
    storage
        PUBLIC
            storage.h
            rocks_db_storage.h
        PRIVATE
            storage.cpp
            rocks_db_storage.cpp
)

target_include_directories(
    storage
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/deps/rocksdb/include"
)

target_link_libraries(
    storage
        PRIVATE
            compresser
            types
            CONAN_PKG::tbb
            rocksdb
)
