add_library(queue queue.cpp)
target_include_directories(queue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(TBB REQUIRED)
target_link_libraries(queue PUBLIC TBB::tbb)