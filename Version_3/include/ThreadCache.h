#ifndef THREAD_CACHE_H
#define THREAD_CACHE_H

#include "Common.h"
#include <assert.h>


namespace memory_pool
{


class ThreadCache
{
public:
    static ThreadCache* Instance()
    {
        static thread_local ThreadCache instance;
        return &instance;
    }

    /// @brief 线程缓存申请内存
    /// @param size 申请的内存大小
    /// @return void* 类型指针
    void* allocate(size_t size);

    /// @brief 线程缓存释放内存
    /// @param ptr 要释放的内存首地址
    /// @param size 释放的对象大小
    void deallocate(void* ptr, size_t size);

private:
    ThreadCache()
    {
        this->m_freeList.fill(nullptr);
        this->m_freeListSize.fill(0);
    }

    /// @brief 从中心缓存中批量申请内存块
    /// @param index 申请的内存块在数组中的索引
    /// @return void* 返回来的内存块链表以及链表大小
    void* fetchFromCentralCache(size_t index);

    /// @brief 根据索引对应的内存块大小确定批量获取的内存块数量
    /// @param index 申请的内存块在数组中的索引
    /// @return size_t 返回来对应的批次数量
    size_t getBatchNum(size_t index);

    /// @brief 是否应该归还给中心缓存
    /// @param index 数组链表对应的索引位置
    /// @return bool
    bool shouldReturntoCentralCache(size_t index);

    /// @brief 将index索引位置上的链表进行归还
    /// @param index 
    void returnToCentralCache(size_t index);


private:    

    // 通过数组记录不同大小内存块的链表 
    std::array<void*, FREE_LIST_SIZE> m_freeList;

    // 通过数组记录对应链表中内存块的数量
    std::array<size_t, FREE_LIST_SIZE> m_freeListSize;

};








}








#endif // THREAD_CACHE_H