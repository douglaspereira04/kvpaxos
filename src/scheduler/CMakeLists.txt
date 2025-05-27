add_library(scheduler)

target_sources(
    scheduler
        PUBLIC
            partition.hpp
        PRIVATE
            partition.cpp
)


target_include_directories(
    scheduler
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(
    scheduler
        PUBLIC
            storage
            request
            types
            utils
)
