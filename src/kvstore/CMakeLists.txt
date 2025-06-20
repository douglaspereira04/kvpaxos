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
IF(${EDGES} STREQUAL "ON")
    target_compile_definitions(kvstore PUBLIC EDGES)
ENDIF()


target_include_directories(storage
    PUBLIC
        ${TKRZW_INCLUDE_DIRS}
)

target_link_libraries(kvstore
    PUBLIC
        absl::btree
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