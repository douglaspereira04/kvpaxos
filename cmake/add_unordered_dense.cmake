include(ExternalProject)

set(UNORDERED_DENSE_INSTALL_DIR "${CMAKE_BINARY_DIR}/unordered_dense_install" CACHE PATH "Install path for unordered_dense")

ExternalProject_Add(unordered_dense_project
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/unordered_dense"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${UNORDERED_DENSE_INSTALL_DIR}
    BUILD_COMMAND ""
    INSTALL_COMMAND cmake --install . --prefix ${UNORDERED_DENSE_INSTALL_DIR}
)

add_library(unordered_dense INTERFACE IMPORTED GLOBAL)

# Use source include directory at configure time
set(source_include_dir "${CMAKE_SOURCE_DIR}/external/unordered_dense/include")

set_target_properties(unordered_dense PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${source_include_dir}"
)

add_dependencies(unordered_dense unordered_dense_project)