add_subdirectory(compresser)
add_subdirectory(request)
add_subdirectory(types)
add_subdirectory(utils)
add_subdirectory(storage)
add_subdirectory(scheduler)

add_executable(replica replica.cpp)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(replica PUBLIC INFO)
ENDIF()
IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(replica PUBLIC ANSWER)
ENDIF()
add_definitions( -DQ_SIZE=${Q_SIZE} )

target_link_libraries(
    replica
        PRIVATE
            pthread
            types
            utils
            request
            scheduler
            rocksdb
)

target_include_directories(replica
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)