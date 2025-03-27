cmake_minimum_required(VERSION 3.16)
project(test)

set(CMAKE_CXX_STANDARD 17)

#添加 debug
set(CMAKE_BUILD_TYPE "Debug")

include_directories(${CMAKE_SOURCE_DIR}/src/cache)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/cache CACHE_SRC)

add_executable(LRUTest LRUTest.cpp ${CACHE_SRC})
add_executable(ThreadTest ThreadTest.cpp ${CACHE_SRC})
add_executable(LRUCacheTest LRUCacheTest.cpp ${CACHE_SRC})

target_link_libraries(LRUCacheTest pthread)
target_link_libraries(ThreadTest pthread)