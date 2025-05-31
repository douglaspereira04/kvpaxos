cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

include(cmake/base.cmake)
include(cmake/project_options.cmake)
include(cmake/conan.cmake)

option(INF "description" ON)
option(ANSWER "description" ON)
set(TRACK_LENGTH "0" CACHE STRING "PATTERN TRACKER GRAPH SIZE (0 for INFINITY)")
set(Q_SIZE "0" CACHE STRING "WORKER THREADS QUEUE SIZE (0 for INFINITY)")
set(MAX_SUCESSIVE_IMBALANCE "100" CACHE STRING "ONLY FOR IMBALANCE DETECTION")
set(SCHEDULE_QUEUE_SIZE "0" CACHE STRING "SCHEDULE QUEUE SIZE SHOULD BE COMPATIBLE TO WORKLOAD")


# Disable RocksDB extra stuff
set(WITH_TESTS OFF CACHE BOOL "Disable RocksDB tests")
set(WITH_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_CORE_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_BENCHMARK OFF CACHE BOOL "Disable RocksDB bench")
set(WITH_EXAMPLES OFF CACHE BOOL "Disable RocksDB samplkes")
set(WITH_RUNTIME_DEBUG OFF CACHE BOOL "Disable RocksDB runtime debug")
set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "Disable RocksDB shared")

# Disable oneTBB tests, build static
set(TBB_TEST OFF CACHE BOOL "Disable TBB tests")
set(TBB_EXAMPLES OFF CACHE BOOL "Build TBB as a static library")
set(TBB_INSTALL OFF CACHE BOOL "Enable TBB static library")

# Disable zlib programs/tests, build static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build zlib, TBB static")
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip install targets for zlib")

add_subdirectory(external/zlib)
add_subdirectory(external/oneTBB)
add_subdirectory(external/rocksdb)

include(cmake/add_libkahip.cmake)
include(cmake/add_libmetis.cmake)

add_subdirectory(src)
