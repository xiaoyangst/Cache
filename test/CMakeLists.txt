cmake_minimum_required(VERSION 3.16)
project(test)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/src/cache)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/cache CACHE_SRC)

add_executable(LRUTest LRUTest.cpp ${CACHE_SRC})