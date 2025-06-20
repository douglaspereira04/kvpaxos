add_library(storage
    rocks_db_storage.cpp
    storage.cpp
    stlmap_storage.cpp
    lmdb_storage.cpp
    tkrzw_storage.cpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LMDB REQUIRED lmdb)
pkg_check_modules(TKRZW REQUIRED tkrzw)
target_include_directories(storage
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LMDB_INCLUDE_DIRS}
        ${TKRZW_INCLUDE_DIRS}
)

target_link_libraries(storage
    PUBLIC
        compresser
        rocksdb
        ${LMDB_LIBRARIES}
        ${TKRZW_LIBRARIES}
)