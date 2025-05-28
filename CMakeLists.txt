cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

include(cmake/base.cmake)
include(cmake/project_options.cmake)
include(cmake/conan.cmake)

option(INF "description" ON)
option(ANSWER "description" ON)
set(Q_SIZE "0" CACHE STRING "WORKER THREADS QUEUE SIZE (0 for INFINITY)")

conan(
    PACKAGES
        zlib/1.2.11
        toml11/3.4.0
        tbb/2020.1
)
add_subdirectory(deps/rocksdb)
add_subdirectory(src)
