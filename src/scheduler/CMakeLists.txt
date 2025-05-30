add_library(scheduler partition.cpp)

target_include_directories(scheduler
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Boost REQUIRED)
target_link_libraries(
    scheduler
        PUBLIC
            Boost::boost
            types
            utils
            request
            storage
)
