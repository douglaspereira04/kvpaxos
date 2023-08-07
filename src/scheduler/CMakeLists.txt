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



IF(${COPY} STREQUAL "CONCURRENT")
    message ("Compiling with workload graph and partition map copied concurrently")
    target_compile_definitions(scheduler PUBLIC CONCURRENT_COPY)
ELSE()
    message ("Compiling with workload graph and partition map copied while waiting")
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
