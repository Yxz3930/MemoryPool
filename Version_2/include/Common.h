#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <array>
#include <utility>
#include <map>
#include <mutex>
#include <iostream>
#include <atomic>
#include <thread>
#include <assert.h>

namespace memory_pool{

constexpr size_t ALIGNMENT = 8;
constexpr size_t MAX_BYTES = 256 * 1025; //  256kB
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;
constexpr size_t SPAN_PAGES = 8; // 中心缓存一次向页面缓存申请8页
constexpr size_t PAGE_SIZE = 4 * 1024; // 页面内存大小 4k



class SizeClass
{
public:
    /// @brief 根据输入的大小计算出来对应的链表索引位置
    /// @param size 输入的对象大小
    /// @return size_t 返回数组中自由链表的索引位置
    static size_t getIndex(size_t size)
    {
        return (size - 1) / ALIGNMENT;
    }

    /// @brief 根据输入的size获取对应内存块的最大内存 比如输入5字节输出对应的内存块大小8字节
    /// @param index array上的链表索引
    /// @return size_t 返回可以存入size的最小内存块大小
    static size_t getSlotSize(size_t index)
    {
        return (index + 1) * ALIGNMENT;
    }

};

}









#endif // COMMON_H