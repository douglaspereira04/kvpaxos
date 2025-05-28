add_subdirectory(compresser)
add_subdirectory(request)
add_subdirectory(types)
add_subdirectory(utils)
add_subdirectory(storage)
add_subdirectory(scheduler)
add_subdirectory(graph)

find_package(Boost REQUIRED)

add_executable(replica)


IF(${INFO} STREQUAL "ON")
    target_compile_definitions(replica PUBLIC INFO)
ENDIF()
IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(replica PUBLIC ANSWER)
ENDIF()
add_definitions( -DTRACK_LENGTH=${TRACK_LENGTH} )
add_definitions( -DQ_SIZE=${Q_SIZE} )
add_definitions( -DSCHEDULE_QUEUE_SIZE=${SCHEDULE_QUEUE_SIZE} )
add_definitions( -DMAX_SUCESSIVE_IMBALANCE=${MAX_SUCESSIVE_IMBALANCE} )

target_sources(
    replica
        PRIVATE
            replica.cpp
)

target_link_libraries(
    replica
        PRIVATE
            rocksdb
            Boost::boost
            CONAN_PKG::toml11
            request
            types
            utils
            scheduler
)
target_include_directories(
    replica
        PUBLIC
            "${CMAKE_SOURCE_DIR}/deps/rocksdb/include"
            "${CMAKE_SOURCE_DIR}/deps/KaHIP/interface"
)