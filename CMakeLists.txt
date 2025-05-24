cmake_minimum_required(VERSION 3.15)
project(load_gen CXX)

include(cmake/base.cmake)
include(cmake/project_options.cmake)
include(cmake/conan.cmake)

conan(
    PACKAGES
        toml11/3.4.0
)

add_subdirectory(src)
