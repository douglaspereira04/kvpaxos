add_subdirectory(compresser)
add_subdirectory(types)
add_subdirectory(utils)
add_subdirectory(storage)

add_executable(main main.cpp)

IF(${ENGINE} STREQUAL "ANKERL")
    target_compile_definitions(main PUBLIC ANKERL)
ENDIF()
IF(${ENGINE} STREQUAL "ROCKS_DB")
    target_compile_definitions(main PUBLIC ROCKS_DB)
ENDIF()
IF(${ENGINE} STREQUAL "LEVEL_DB")
    target_compile_definitions(main PUBLIC LEVEL_DB)
ENDIF()
IF(${ENGINE} STREQUAL "LMDB")
    target_compile_definitions(main PUBLIC LMDB)
ENDIF()
IF(${ENGINE} STREQUAL "TKRZW")
    target_compile_definitions(main PUBLIC TKRZW)
ENDIF()

IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(main PUBLIC ANSWER)
ENDIF()

target_link_libraries(
    main
        PRIVATE
            absl::btree
            pthread
            types
            utils
            rocksdb
            leveldb
            storage
)

target_include_directories(main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)