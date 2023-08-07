add_subdirectory(compresser)
add_subdirectory(request)
#add_subdirectory(evclient)
add_subdirectory(types)
add_subdirectory(storage)
add_subdirectory(scheduler)
add_subdirectory(graph)

find_package(LibCDS REQUIRED)

add_executable(replica)

IF(${STORAGE} STREQUAL "FELDMAN")
    message ("Compiling with Feldman Hashmap")
    target_compile_definitions(replica PUBLIC FELDMAN)
ELSEIF(${STORAGE} STREQUAL "MICHAEL")
    message ("Compiling with Michael Hashmap")
    target_compile_definitions(replica PUBLIC MICHAEL)
ELSEIF(${STORAGE} STREQUAL "TBB")
    message ("Compiling with TBB Hashmap")
    target_compile_definitions(replica PUBLIC TBB)
ELSE()
    message ("Compiling with other Hashmap")
ENDIF()


IF(${SCHEDULER} STREQUAL "NEW")
    message ("New scheduler")
    target_compile_definitions(replica PUBLIC NEW_SCHEDULER)
ELSE()
    message ("Old Scheduler")
    target_compile_definitions(replica PUBLIC OLD_SCHEDULER)
ENDIF()

target_sources(
    replica
        PRIVATE
            replica.cpp
)

target_link_libraries(
    replica
        PRIVATE
            LibCDS::cds
            CONAN_PKG::toml11
            request
            types
            scheduler
)
