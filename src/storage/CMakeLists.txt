add_library(storage)

IF(${STORAGE} STREQUAL "FELDMAN")
    target_compile_definitions(storage PUBLIC FELDMAN)
ELSEIF(${STORAGE} STREQUAL "MICHAEL")
    target_compile_definitions(storage PUBLIC MICHAEL)
ELSEIF(${STORAGE} STREQUAL "TBB")
    target_compile_definitions(storage PUBLIC TBB)
ELSE()
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
