cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

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
set(TBB_EXAMPLES OFF CACHE BOOL "Build TBB examples")
add_subdirectory(external/oneTBB)

# Disable zlib programs/tests, build static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build zlib")
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip install targets for zlib")

set(GKLIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/GKlib)

set(PARHIP OFF CACHE BOOL "Disable parhip")
set(PARHIP OFF CACHE BOOL "Disable parhip")
set(GKLIB_PATH "${CMAKE_SOURCE_DIR}/external/METIS/GKlib")
add_subdirectory(external/METIS)
add_subdirectory(external/KaHIP)
add_subdirectory(external/zlib)
add_subdirectory(external/rocksdb)

add_subdirectory(src)
