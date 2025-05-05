#ifndef CENTRAL_CACHE_H
#define CENTRAL_CACHE_H

#include "Common.h"

namespace memory_pool
{
class CentralCache
{
public:
    static CentralCache* Instance()
    {
        static CentralCache instance;
        return &instance;
    }

    /// @brief 从中心缓存向线程缓存分配内存块链表
    /// @param index 内存块大小对应的索引位置
    /// @return pair<void*, size_t> 返回首地址以及内存块链表的大小
    std::pair<void*, size_t> fetchRange(size_t index);


    /// @brief 线程缓存归还内存给中心缓存
    /// @param ptr 待归还的链表头节点地址
    /// @param blockNums 需要归还的内存块数量
    /// @param index 内存块对应的索引
    void returnRange(void* ptr, size_t blockNums, size_t index);


    void printListSize();
private:

    CentralCache()
    {
        for(auto& ptr : this->m_freeList)
        {
            ptr.store(nullptr, std::memory_order_relaxed);
        }

        for(auto& size : this->m_freeListSize)
        {
            size.store(0, std::memory_order_relaxed);
        }

        for(auto& lock : this->m_freeListLock)
        {
            lock.clear();
        }
    }


    /// @brief 中心缓存向页面缓存申请内存
    /// @param size 申请的内存大小 内部会将size转为内存页数量
    /// @return pair<void*, size_t> 分配的首地址以及对应的内存页数量 
    std::pair<void*, size_t> fetchFromPageCache(size_t size);


    /// @brief 根据索引获取对应的内存块批次大小
    /// @param index 数组链表对应的索引位置
    /// @return size_t 返回批次大小
    size_t getBatchNum(size_t index);

private:

    // 与线程缓存大小一致的数组链表 使用原子操作
    std::array<std::atomic<void*>, FREE_LIST_SIZE> m_freeList;

    // 数组链表中对应的链表大小
    std::array<std::atomic<size_t>, FREE_LIST_SIZE> m_freeListSize;

    // 与上面的数组链表大小一致的自旋锁数组 分别对应每个链表
    std::array<std::atomic_flag, FREE_LIST_SIZE> m_freeListLock;


};

}



#endif // CENTRAL_CACHE_H