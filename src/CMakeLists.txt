add_subdirectory(compresser)
add_subdirectory(types)
add_subdirectory(utils)
add_subdirectory(storage)

add_executable(main main.cpp)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(main PUBLIC INFO)
ENDIF()
IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(main PUBLIC ANSWER)
ENDIF()

target_link_libraries(
    main
        PRIVATE
            pthread
            types
            utils
            rocksdb
            storage
)

target_include_directories(main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)