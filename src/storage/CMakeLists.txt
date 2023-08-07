add_library(storage)

IF(${STORAGE} STREQUAL "FELDMAN")
    message ("Compiling with Feldman Hashmap")
    target_compile_definitions(storage PUBLIC FELDMAN)
ELSEIF(${STORAGE} STREQUAL "MICHAEL")
    message ("Compiling with Michael Hashmap")
    target_compile_definitions(storage PUBLIC MICHAEL)
ELSEIF(${STORAGE} STREQUAL "TBB")
    message ("Compiling with TBB Hashmap")
    target_compile_definitions(storage PUBLIC TBB)
ELSE()
    message ("Compiling with other Hashmap")
ENDIF()

target_sources(
    storage
        PUBLIC
            storage.h
        PRIVATE
            storage.cpp
)

target_include_directories(
    storage
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(
    storage
        PRIVATE
            compresser
            types
            CONAN_PKG::tbb
)
