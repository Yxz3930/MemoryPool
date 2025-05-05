set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_BUILD_TYPE Debug)

# 编译选项
add_compile_options(-Wall -O2)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/output)

cmake_minimum_required(VERSION 3.10)

project(MemoryPool)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

add_executable(MemoryPool_mutex src/Test_mutex.cpp src/MemoryPool_mutex.cpp)

add_executable(MemoryPool_CAS src/Test_CAS.cpp src/MemoryPool_CAS.cpp)

add_executable(MemoryPool_comprate src/Test_compare.cpp src/MemoryPool_CAS.cpp src/MemoryPool_mutex.cpp)

