cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

set (CMAKE_CXX_STANDARD 17)

option(INF "description" ON)
option(EDGES "description" ON)
option(ANSWER "description" ON)
option(LINEARIZABLE "description" ON)
option(REPARTITIONING "ENABLE REPARTITIONING" ON)
set(TRACK_LENGTH "0" CACHE STRING "PATTERN TRACKER GRAPH SIZE (0 for INFINITY)")
set(Q_SIZE "0" CACHE STRING "WORKER THREADS QUEUE SIZE (0 for INFINITY)")
set(ENGINE "TKRZW" CACHE STRING "ANKERL ROCKS_DB LMDB TKRZW")


include(cmake/add_libmetis.cmake)
include(cmake/add_libkahip.cmake)
include(cmake/add_unordered_dense.cmake)
include(cmake/add_readerwriterqueue.cmake)
include(cmake/add_abseil.cmake)

# Disable zlib programs/tests, build static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static")
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Build static zlib")
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip install targets for zlib")

add_subdirectory(external/zlib)

add_subdirectory(src)
