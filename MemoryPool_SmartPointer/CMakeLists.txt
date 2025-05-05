set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

# 编译选项
# add_compile_options(-Wall -O2)

# 设置库和可执行文件输出路径 在preject之前
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/output)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.10)


project(CustomDefinedSmartPointer)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MemoryPool)

add_executable(SmartPointer ${CMAKE_CURRENT_SOURCE_DIR}/CustomDefinedSmartPointer/SmartPointer.cpp)
target_link_libraries(SmartPointer PRIVATE MemoryPool)
