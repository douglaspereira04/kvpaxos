add_library(graph
    graph.cpp
    partitioning.cpp
)

target_include_directories(graph
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ${GKLIB_PATH}
    ${CMAKE_SOURCE_DIR}/external/METIS/build/xinclude
)

# Link to RocksDB if only this module needs it
target_link_libraries(graph
    PUBLIC
        metis
        interface_static
)