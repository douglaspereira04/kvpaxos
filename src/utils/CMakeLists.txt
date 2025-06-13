add_library(utils utils.cpp)
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(utils PUBLIC types)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(utils PUBLIC INFO)
ENDIF()

IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(utils PUBLIC ANSWER)
ENDIF()