cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

include(cmake/base.cmake)
include(cmake/project_options.cmake)
include(cmake/conan.cmake)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3") 

set(STORAGE "DEFAULT" CACHE STRING "FELDMAN, MICHAEL, TBB")
set(SCHEDULER "OLD" CACHE STRING "OLD, FREE, NON_STOP")
set(GRAPH "OLD_GRAPH" CACHE STRING "OLD_GRAPH, TBB_GRAPH")
set(TRACK_LENGTH 0 CACHE INTEGER "PATTERN TRACKER GRAPH SIZE (0 for INFINITY)")
set(Q_SIZE 0 CACHE INTEGER "WORKER THREADS QUEUE SIZE (0 for INFINITY)")

conan(
    PACKAGES
        zlib/1.2.11
        toml11/3.4.0
        libevent/2.1.11
        tbb/2020.1
)


include(cmake/add_libpaxos.cmake)
include(cmake/add_libkahip.cmake)
include(cmake/add_libmetis.cmake)

add_subdirectory(src)

add_subdirectory(matplotplusplus)
add_executable(kvpaxos_plot kvpaxos_plot.cpp)
target_link_libraries(kvpaxos_plot PUBLIC matplot)
