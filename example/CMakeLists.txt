cmake_minimum_required(VERSION 3.16)
project(example)

set(CMAKE_CXX_STANDARD 17)

#添加 debug
set(CMAKE_BUILD_TYPE "Debug")

include_directories(${CMAKE_SOURCE_DIR}/src/cache)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/cache CACHE_SRC)

add_executable(AppointCache AppointCache.cpp ${CACHE_SRC})

target_link_libraries(AppointCache pthread)