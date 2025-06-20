add_library(graph
    graph.cpp
    edgeless_graph.cpp
    partitioning.cpp
)

target_include_directories(graph
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(graph PUBLIC INFO)
ENDIF()

IF(${EDGES} STREQUAL "ON")
    target_compile_definitions(graph PUBLIC EDGES)
ENDIF()

target_link_libraries(graph
    PUBLIC
        utils
        metis
        kahip
        unordered_dense
        absl::btree
)