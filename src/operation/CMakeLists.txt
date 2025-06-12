add_library(operation
    operation.cpp
    get_operation.cpp
    set_operation.cpp
    scan_operation.cpp
    del_operation.cpp
    callback_operation.cpp
    get_callback_operation.cpp
    set_callback_operation.cpp
    scan_callback_operation.cpp
    del_callback_operation.cpp
    get_future_operation.cpp
    scan_future_operation.cpp
    tracking_info.cpp
)

target_include_directories(operation
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(${INFO} STREQUAL "ON")
    target_compile_definitions(operation PUBLIC INFO)
ENDIF()
IF(${LINEARIZABLE} STREQUAL "ON")
    target_compile_definitions(operation PUBLIC LINEARIZABLE)
ENDIF()
IF(${ANSWER} STREQUAL "ON")
    target_compile_definitions(operation PUBLIC ANSWER)
ENDIF()
IF(${REPARTITIONING} STREQUAL "ON")
    target_compile_definitions(operation PUBLIC REPARTITIONING)
ENDIF()

target_link_libraries(
    operation
        PUBLIC
            types
            utils
)
