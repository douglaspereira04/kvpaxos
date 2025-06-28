add_library(kvstore
    kvstore.cpp
    worker.cpp
    input_graph.cpp
)

target_include_directories(kvstore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TKRZW REQUIRED tkrzw)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(kvstore PUBLIC INFO)
ENDIF()
IF(${LINEARIZABLE} STREQUAL "ON")
    target_compile_definitions(kvstore PUBLIC LINEARIZABLE)
ENDIF()
IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(kvstore PUBLIC ANSWER)
ENDIF()
IF(${REPARTITIONING} STREQUAL "ON")
    target_compile_definitions(kvstore PUBLIC REPARTITIONING)
ENDIF()
add_definitions( -DPARTITIONS=${PARTITIONS} )

target_include_directories(storage
    PUBLIC
        ${TKRZW_INCLUDE_DIRS}
)

find_package(TBB REQUIRED)
target_link_libraries(kvstore
    PUBLIC
        TBB::tbb
        absl::btree
        absl::flat_hash_set
        unordered_dense
        readerwriterqueue
        queue
        types
        utils
        operation
        graph
        storage
        ${TKRZW_LIBRARIES}
)