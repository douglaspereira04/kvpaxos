cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

option(INF "description" ON)
option(ANSWER "description" ON)
set(Q_SIZE "0" CACHE STRING "WORKER THREADS QUEUE SIZE (0 for INFINITY)")

# Disable RocksDB extra stuff
set(WITH_TESTS OFF CACHE BOOL "Disable RocksDB tests")
set(WITH_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_CORE_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_BENCHMARK OFF CACHE BOOL "Disable RocksDB bench")
set(WITH_EXAMPLES OFF CACHE BOOL "Disable RocksDB samplkes")
set(WITH_RUNTIME_DEBUG OFF CACHE BOOL "Disable RocksDB runtime debug")
set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "Disable RocksDB shared")

# Disable RocksDB extra stuff
set(WITH_TESTS OFF CACHE BOOL "Disable RocksDB tests")
set(WITH_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_CORE_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "Disable RocksDB tools")
set(WITH_BENCHMARK OFF CACHE BOOL "Disable RocksDB bench")
set(WITH_EXAMPLES OFF CACHE BOOL "Disable RocksDB samplkes")
set(WITH_RUNTIME_DEBUG OFF CACHE BOOL "Disable RocksDB runtime debug")
set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "Disable RocksDB shared")

# Disable zlib programs/tests, build static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build zlib, TBB static")
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip install targets for zlib")

add_subdirectory(external/zlib)
add_subdirectory(external/rocksdb)

add_subdirectory(src)
