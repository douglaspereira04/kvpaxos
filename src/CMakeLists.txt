add_subdirectory(compresser)
add_subdirectory(request)
#add_subdirectory(evclient)
add_subdirectory(types)
add_subdirectory(storage)
add_subdirectory(scheduler)
add_subdirectory(graph)

add_executable(replica)

IF(${STORAGE} STREQUAL "TBB")
    message ("Compiling with TBB Hashmap")
    target_compile_definitions(replica PUBLIC TBB)
ELSE()
    message ("Compiling with other Hashmap")
ENDIF()

target_sources(
    replica
        PRIVATE
            replica.cpp
)

target_link_libraries(
    replica
        PRIVATE
            CONAN_PKG::toml11
            request
            types
            scheduler
)
