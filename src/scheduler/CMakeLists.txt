add_library(scheduler)

target_sources(
    scheduler
        PUBLIC
            partition.hpp
        PRIVATE
            partition.cpp
)


target_include_directories(
    scheduler
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/deps/rocksdb/include"
)

target_link_libraries(
    scheduler
        PUBLIC
            rocksdb
            storage
            request
            types
            utils
)
