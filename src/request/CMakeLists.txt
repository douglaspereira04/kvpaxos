add_library(request request.cpp)

target_include_directories(request
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(request PUBLIC INFO)
ENDIF()
IF(${LINEARIZABLE} STREQUAL "ON")
    target_compile_definitions(request PUBLIC LINEARIZABLE)
ENDIF()
IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(request PUBLIC ANSWER)
ENDIF()
IF(${REPARTITIONING} STREQUAL "ON")
    target_compile_definitions(request PUBLIC REPARTITIONING)
ENDIF()

target_link_libraries(
    request
        PUBLIC
            types
            utils
)
