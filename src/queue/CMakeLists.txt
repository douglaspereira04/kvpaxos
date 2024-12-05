add_library(queue)

target_sources(
    queue
        PUBLIC
            queue.hpp
        PRIVATE
            queue.cpp
)

target_include_directories(
    queue
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
)

target_compile_options(
    queue
        PRIVATE
            -O3
)
