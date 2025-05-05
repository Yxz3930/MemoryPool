#include "ThreadCache.h"
#include "CentralCache.h"

namespace memory_pool
{

    void *ThreadCache::allocate(size_t size)
    {
        /**
         * 线程缓存申请内存
         * 整理流程:
         * 参数有效性判断;
         * 如果对应链表中有空闲内存 直接分配;
         * 如果对应链表中没有空闲内存 则从中心缓存中批量申请;
         */

        assert(size > 0);

        size_t idx = SizeClass::getIndex(size);

        void *headNode = this->m_freeList[idx];
        if (headNode)
        {
            void *nextNode = *reinterpret_cast<void **>(headNode);
            *reinterpret_cast<void **>(headNode) = nullptr;
            this->m_freeList[idx] = nextNode;
            this->m_freeListSize[idx]--;
            return headNode;
        }

        return this->fetchFromCentralCache(idx);
    }

    void ThreadCache::deallocate(void *ptr, size_t size)
    {
        /**
         * 线程缓存释放内存
         * 整体流程:
         * 参数有效性判断;
         * 将释放的内存块存放到链表当中
         */
        assert(ptr != nullptr && size > 0);

        size_t idx = SizeClass::getIndex(size);

        void *oldHead = this->m_freeList[idx];
        *reinterpret_cast<void **>(ptr) = oldHead;
        this->m_freeList[idx] = ptr;
        this->m_freeListSize[idx]++;

        if (this->shouldReturntoCentralCache(idx))
        {
            this->returnToCentralCache(idx);
        }
    }

    void *ThreadCache::fetchFromCentralCache(size_t index)
    {
        /**
         * 从中心缓存中批量获取内存块
         * 整理流程:
         * 参数有效性判断;
         * 从中心缓存中获取内存块链表以及对应的链表大小;
         * 将内存链表(去除第一个内存块)前插到线程缓存中对应的数组链表上;
         */

        assert(index >= 0 && index < FREE_LIST_SIZE);

        // 获取得到的是一个链表
        std::pair<void *, size_t> fetchRet = CentralCache::Instance()->fetchRange(index);
        assert(fetchRet.first != nullptr);
        void *ptr = fetchRet.first;
        size_t blockNums = fetchRet.second;

        // 抽取第一个内存块进行返回 这里进行链表的节点操作
        void *nextNode = *reinterpret_cast<void **>(ptr);
        *reinterpret_cast<void **>(ptr) = nullptr;
        this->m_freeList[index] = nextNode;

        this->m_freeListSize[index] += blockNums - 1; // -1是因为第一个用于返回了

        return ptr;
    }

    bool ThreadCache::shouldReturntoCentralCache(size_t index)
    {
        assert(index >= 0 && index < FREE_LIST_SIZE);
        if (this->m_freeListSize[index] > 64)
            return true;
        return false;
    }

    void ThreadCache::returnToCentralCache(size_t index)
    {
        /**
         * 将线程缓存中的内存链表返回给中心缓存
         * 整体流程:
         * 参数有效性判断;
         * 计算归还的内存块数量;
         * 分割内存块链表;
         * 调用中心缓存方法进行归还;
         */
        assert(index >= 0 && index < FREE_LIST_SIZE);

        size_t keepNums = std::max(this->m_freeListSize[index] / 4, size_t(1));
        if(keepNums < 4)
            return;
        size_t returnNums = this->m_freeListSize[index] - keepNums;

        // 寻找第keepNums个内存块的位置 也就是索引keepNums-1的节点
        void *curNode = this->m_freeList[index];
        for (size_t i = 0; i <= keepNums - 2; ++i)
        {
            curNode = *reinterpret_cast<void **>(curNode);
            // 如果遍历过程中遇到nullptr 说明数量不够 直接返回
            if (!curNode)
                return;
        }

        void *splitNode = *reinterpret_cast<void **>(curNode);
        *reinterpret_cast<void **>(curNode) = nullptr;
        this->m_freeListSize[index] = keepNums;

        CentralCache::Instance()->returnRange(splitNode, returnNums, index);
    }

}