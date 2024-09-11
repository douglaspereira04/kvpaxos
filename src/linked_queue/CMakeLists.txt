add_library(linked_queue)

target_sources(
    linked_queue
        PUBLIC
            linked_queue.hpp
        PRIVATE
            linked_queue.cpp
)

target_include_directories(
    linked_queue
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
)

target_compile_options(
    linked_queue
        PRIVATE
            -O3
)
