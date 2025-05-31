add_library(compresser
    compresser.cpp
)

target_include_directories(compresser
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link zlib statically
target_link_libraries(compresser
    PUBLIC zlibstatic
)