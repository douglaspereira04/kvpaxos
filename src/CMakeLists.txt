add_subdirectory(compresser)
add_subdirectory(queue)
add_subdirectory(types)
add_subdirectory(utils)
add_subdirectory(operation)
add_subdirectory(storage)
add_subdirectory(graph)
add_subdirectory(kvstore)

add_executable(main main.cpp)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(main PUBLIC INFO)
ENDIF()
IF(${LINEARIZABLE} STREQUAL "ON")
    target_compile_definitions(main PUBLIC LINEARIZABLE)
ENDIF()
IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(main PUBLIC ANSWER)
ENDIF()
IF(${REPARTITIONING} STREQUAL "ON")
    target_compile_definitions(main PUBLIC REPARTITIONING)
ENDIF()
add_definitions( -DTRACK_LENGTH=${TRACK_LENGTH} )
add_definitions( -DQ_SIZE=${Q_SIZE} )

target_link_libraries(
    main
        PRIVATE
            absl::btree
            pthread
            types
            utils
            operation
            rocksdb
            kvstore
)

target_include_directories(main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    main
        PUBLIC
            "${CMAKE_SOURCE_DIR}/external/KaHIP/interface"
            "${CMAKE_SOURCE_DIR}/external/unordered_dense/include"
)