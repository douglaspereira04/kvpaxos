cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)

include(cmake/base.cmake)
include(cmake/project_options.cmake)
include(cmake/conan.cmake)

set(SCHEDULER "OLD" CACHE STRING "OLD, FREE, NON_STOP")
set(TRACK_LENGTH "0" CACHE STRING "PATTERN TRACKER GRAPH SIZE (0 for INFINITY)")
set(Q_SIZE "0" CACHE STRING "WORKER THREADS QUEUE SIZE (0 for INFINITY)")
set(SCHEDULE_QUEUE_SIZE "0" CACHE STRING "SCHEDULE QUEUE SIZE SHOULD BE COMPATIBLE TO WORKLOAD")

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
