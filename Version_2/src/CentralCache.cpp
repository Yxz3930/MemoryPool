#include "CentralCache.h"
#include "PageCache.h"

namespace memory_pool
{

    void *CentralCache::fetchRange(size_t index)
    {
        /**
         * 从中心缓存中获取内存块
         * 整体思路:
         * 参数有效性判断;
         * 自旋锁获取;
         * 中心缓存对应的链表不为空 需要进行链表弹出操作;
         * 中心缓存对应的链表为空 则需要从页面缓存中获取;
         * 最后别忘了自旋锁释放锁
         * 整体的操作是在自旋锁的情况下进行原子操作 数据的存取需要加上对应的内存序
         */
        assert(index >= 0 && index < FREE_LIST_SIZE);

        // 自旋锁保护 同时使用线程yield避免持续自旋消耗cpu  这里加锁了后续要记得释放锁
        while (this->m_centralFreeListLock[index].test_and_set(std::memory_order_acquire))
        {
            // 如果没有获取到锁 通过yield避免持续自选 让该线程放弃cpu竞争
            std::this_thread::yield();
        }

        void *result = nullptr;
        try
        {
            result = this->m_centralFreeList[index].load(std::memory_order_acquire);

            // 如果获取到的指针为空 说明中心缓存为空 从页面缓存中申请内存
            if (!result)
            {
                // 计算索引对应的内存块的大小
                ssize_t size = (index + 1) * ALIGNMENT;
                std::pair<void *, size_t> ret_pair = this->fetchFromPageCache(size);

                result = ret_pair.first;
                size_t numPages = ret_pair.second;
                // 如果从页面缓存那里也没有获取到内存 则释放锁并直接返回
                if (!result)
                {
                    this->m_centralFreeListLock[index].clear();
                    return nullptr;
                }

                // 从页面缓存中获取到了内存(一整块内存) 需要分割成小块并以链表的方式链接
                size_t totalBytes = numPages * PAGE_SIZE;
                int blockNums = totalBytes / size;
                void *curNode = result;
                if (blockNums >= 2)
                {
                    for (int i = 0; i <= blockNums - 2; ++i) // 这里需要注意链表遍历过程中变量i和索引、链表节点数量之间的关系
                    {
                        void *nextNode = reinterpret_cast<void *>(reinterpret_cast<size_t>(curNode) + size);
                        *reinterpret_cast<void **>(curNode) = nextNode;
                        curNode = nextNode;
                    }
                }
                // curNode出来之后 对应的索引位置是blockNums - 1, 也就是最后一个节点
                // 最后一个内存块next指向nullptr 这个很重要 因为有时候就是通过最后一个节点为空来终止while循环
                *reinterpret_cast<void **>(curNode) = nullptr;

                // 将第一个内存块进行返回 其余链接到链表上
                void *nextNode = *reinterpret_cast<void **>(result);
                this->m_centralFreeList[index].store(nextNode, std::memory_order_release);
            }
            // 如果中心缓存中存在内存块
            else
            {
                // 中心缓存向线程缓存分配内存 一次只分需要的一个 不是批量处理 会降低性能
                void *nextNode = *reinterpret_cast<void **>(result);
                *reinterpret_cast<void **>(result) = nullptr;

                this->m_centralFreeList[index].store(nextNode, std::memory_order_release);
            }
        }
        catch (...)
        {
            this->m_centralFreeListLock[index].clear();
            throw;
        }

        this->m_centralFreeListLock[index].clear();
        return result;
    }

    void CentralCache::returnRange(void *startAddr, size_t blockNums, size_t index)
    {
        /**
         * 线程缓存归还内存块到中心缓存
         * 整体思路:
         * 参数有效性检查;
         * 自旋锁加锁;
         * 将归还的链表前插到中心缓存对应的链表上(链表操作);
         * 最后别忘了自旋锁释放锁
         *
         */
        if (!startAddr || index > FREE_LIST_SIZE)
            return;

        // 获取自旋锁 后面要加上释放自旋锁
        while (this->m_centralFreeListLock[index].test_and_set(std::memory_order_acquire))
        {
            // 如果没有获取到 就先放弃cpu
            std::this_thread::yield();
        }

        try
        {
            void *curNode = startAddr;
            for (size_t i = 0; i <= blockNums - 2; ++i) // 这里注意遍历的范围和链表节点数量的关系
            {
                curNode = *reinterpret_cast<void **>(curNode);
            }

            // curNode出来之后就是对应着blockNums-1的节点 也就是链表最后一个节点
            void *endNode = curNode;
            *reinterpret_cast<void **>(endNode) = this->m_centralFreeList[index].load(std::memory_order_relaxed);
            this->m_centralFreeList[index].store(startAddr, std::memory_order_release);
        }
        catch (...)
        {
            this->m_centralFreeListLock[index].clear();
            throw;
        }

        this->m_centralFreeListLock[index].clear();
    }
    std::pair<void *, size_t> CentralCache::fetchFromPageCache(size_t size)
    {
        /**
         * 根据申请的内存大小 从页面缓存中申请内存页
         * 整理流程：
         * 参数有效性判断;
         * 根据申请内存大小，计算需要申请的内存页数量;
         * 申请内存页;
         */

        // 由需要申请的内存块大小计算出俩对应的页数
        size_t numPages = (size - 1 + PAGE_SIZE) / PAGE_SIZE;

        // 根据大小决定分配策略
        if (numPages <= SPAN_PAGES)
            return std::make_pair(PageCache::Instance()->allocateSpanPage(SPAN_PAGES), SPAN_PAGES);
        else
            return std::make_pair(PageCache::Instance()->allocateSpanPage(numPages), numPages);

        return std::make_pair(nullptr, -1);
    }

}