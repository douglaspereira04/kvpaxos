add_subdirectory(request)
add_subdirectory(types)

add_executable(gen)

target_sources(
    gen
        PRIVATE
            gen.cpp
)

target_link_libraries(
    gen
        PRIVATE
            CONAN_PKG::toml11
            request
            types
)