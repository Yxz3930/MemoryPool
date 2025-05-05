# ##################################################
# 将这个文件夹中的文件制作称库
# ##################################################
add_library(MemoryPool
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/MemoryPool_CAS.cpp
)

target_include_directories(MemoryPool PUBLIC .)


# ##################################################
# 这个文件夹中的文件进行编译输出 得到可执行文件
# ##################################################


set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/output)

add_compile_options(-Wall -O2)
add_executable(Test_CAS test/Test_CAS.cpp ${CMAKE_CURRENT_SOURCE_DIR}/MemoryPool_CAS.cpp)
target_include_directories(Test_CAS PRIVATE .)



