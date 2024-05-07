add_library(utils)

target_sources(
    utils
        PUBLIC
            utils.h
        PRIVATE
        utils.cpp
)

target_include_directories(
    utils
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
)
