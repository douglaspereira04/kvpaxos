add_library(scheduler partition.cpp)

target_include_directories(scheduler
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    scheduler
        PUBLIC
            types
            utils
            request
            storage
)
