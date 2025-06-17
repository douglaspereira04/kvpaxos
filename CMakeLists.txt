cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

set (CMAKE_CXX_STANDARD 17)

option(ANSWER "description" ON)


# Disable zlib programs/tests, build static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static")
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Build static zlib")
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip install targets for zlib")

include(cmake/add_unordered_dense.cmake)
include(cmake/add_abseil.cmake)

add_subdirectory(external/zlib)

add_subdirectory(src)
