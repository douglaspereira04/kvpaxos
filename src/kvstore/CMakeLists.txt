add_library(kvstore
    kvstore.cpp
    worker.cpp
    input_graph.cpp
)

target_include_directories(kvstore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)


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
target_link_libraries(kvstore
    PUBLIC
        unordered_dense
        readerwriterqueue
        queue
        types
        utils
        operation
        graph
        storage
)