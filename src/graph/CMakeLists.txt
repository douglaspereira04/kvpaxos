add_library(graph
    graph.cpp
    partitioning.cpp
)

target_include_directories(graph
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(utils PUBLIC INFO)
ENDIF()

target_link_libraries(graph
    PUBLIC
        utils
        metis
        kahip
        unordered_dense
        absl::btree
)