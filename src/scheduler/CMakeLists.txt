add_library(scheduler
    scheduler.cpp
    partition.cpp
    input_graph.cpp
)

target_include_directories(scheduler
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(scheduler
    PUBLIC
        queue
        types
        utils
        request
        graph
        storage
)