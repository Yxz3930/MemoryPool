set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

# 编译选项
add_compile_options(-Wall -O2)
# add_compile_options(-Wall)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output)

cmake_minimum_required(VERSION 3.10)

project(MemoryPool)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB src_files ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(memoryPool_test
    ${CMAKE_SOURCE_DIR}/test/memoryPool_test.cpp
    ${src_files}
)


