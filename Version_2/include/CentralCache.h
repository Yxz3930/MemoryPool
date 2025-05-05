#ifndef CENTRAL_CACHE_H
#define CENTRAL_CACHE_H

#include "Common.h"

namespace memory_pool{


class CentralCache
{
public:
    static CentralCache* Instance()
    {
        static CentralCache instance;
        return &instance;
    }

    /// @brief 从中心缓存中获取内存 如果没有内存则向PageCache申请内存
    /// @param index 
    /// @return 
    void* fetchRange(size_t index);

    /// @brief 线程缓存将内存归还给中心缓存 将原链表链接到归还的链表的尾部
    /// @param startAddr 内存起始地址
    /// @param blockNums 归还的内存块数量 这个数量用于找到归还链表的最后一个内存块地址 
    /// @param index freeListArray数组对应的链表索引
    void returnRange(void* startAddr, size_t blockNums, size_t index);

private:
    CentralCache()
    {
        // 初始化中心缓存自由链表
        for(auto& listPtr : this->m_centralFreeList)
            listPtr.store(nullptr, std::memory_order_relaxed);

        // 初始化自由链表锁
        for(auto& lock : this->m_centralFreeListLock)
            lock.clear();
    }

    /// @brief 从页面缓存申请内存
    /// @param size 申请的每个内存块的大小 可以申请多个这么大的内存块然后返回首地址
    /// @return pair<void*, size_t> 返回获取的首地址以及申请的内存页数量(用于计算整个内存块的大小)
    std::pair<void*, size_t> fetchFromPageCache(size_t size);

private:

    // 中心缓存中维护内存链表数组 加上atomic保证原子性
    std::array<std::atomic<void*>, FREE_LIST_SIZE> m_centralFreeList;

    // 每个链表对应一个自旋锁进行保护
    std::array<std::atomic_flag, FREE_LIST_SIZE> m_centralFreeListLock;

};




}





#endif // CENTRAL_CACHE_H