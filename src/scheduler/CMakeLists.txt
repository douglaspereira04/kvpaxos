add_library(scheduler
    scheduler.cpp
    partition.cpp
    input_graph.cpp
)

target_include_directories(scheduler
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)


find_package(Boost REQUIRED)
target_link_libraries(scheduler
    PUBLIC
        Boost::boost
        queue
        types
        utils
        request
        graph
        storage
)