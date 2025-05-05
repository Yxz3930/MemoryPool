#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <thread>
#include <unordered_map>
#include <map>
#include <iomanip>
#include <vector>
#include <iostream>
#include <utility>
#include <sys/mman.h>
#include <cstring>
#include <mutex>
#include <array>
#include <assert.h>
#include <atomic>

namespace memory_pool
{

// 内存槽对齐大小
constexpr size_t ALIGNMENT = 8;

// 最大的内存块
constexpr size_t MAX_BYTES = 256 * 1024;

// 自由链表数组大小
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;

// 每次分配的内存页面数量
constexpr size_t SPAN_PAGE = 4;

// 内存页大小
constexpr size_t PAGE_SIZE = 4 * 1024;


class SizeClass
{
public:
    static size_t getIndex(size_t size)
    {
        return (size - 1) / ALIGNMENT;
    }

    static size_t getBlockSize(size_t index)
    {
        return (index + 1) * ALIGNMENT;
    }

};

}


#endif // COMMON_H