add_library(scheduler)

target_sources(
    scheduler
        PUBLIC
            scheduler.hpp
            partition.hpp
        PRIVATE
            scheduler.cpp
            partition.cpp
)


IF(${PENDING_KEYS} STREQUAL "MAP")
    message ("Compiling with pending keys in a MAP")
    target_compile_definitions(scheduler PUBLIC MAP_PENDING)
ELSE()
    message ("Compiling with pending keys in a VECTOR")
ENDIF()

target_include_directories(
    scheduler
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(
    scheduler
        PUBLIC
            graph
            storage
            request
            types
)
