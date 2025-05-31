add_subdirectory(compresser)
add_subdirectory(request)
add_subdirectory(types)
add_subdirectory(utils)
add_subdirectory(storage)
add_subdirectory(scheduler)
add_subdirectory(graph)


add_executable(replica replica.cpp)

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


find_package(Boost REQUIRED)
target_link_libraries(
    replica
        PRIVATE
            pthread
            Boost::boost
            types
            utils
            request
            rocksdb
            scheduler
)

target_include_directories(replica
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    replica
        PUBLIC
            "${CMAKE_SOURCE_DIR}/external/rocksdb/include"
            "${CMAKE_SOURCE_DIR}/external/KaHIP/interface"
)