#include "CentralCache.h"
#include "PageCache.h"

namespace memory_pool
{

    void CentralCache::printListSize()
    {
        std::cout << "m_freeListSize nums: " << std::endl;
        for (int i = 0; i < 8; i++)
        {
            std::cout << std::setw(5) << std::left << m_freeListSize[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "while get list nums: " << std::endl;
        std::array<size_t, 8> arr;
        for (int i = 0; i < 8; ++i)
        {
            void *curNode = this->m_freeList[i];
            size_t count = 0;
            while (curNode)
            {
                curNode = *reinterpret_cast<void **>(curNode);
                ++count;
            }
            arr[i] = count;
        }
        for (int i = 0; i < 8; i++)
        {
            std::cout << std::setw(5) << std::left << arr[i] << " ";
        }
        std::cout << std::endl;
    }

    std::pair<void *, size_t> CentralCache::fetchRange(size_t index)
    {
        /**
         * 从中心缓存申请内存块
         * 整体流程:
         * 参数有效性判断;
         * 获取自旋锁
         * 如果中心缓存数组链表中存在空闲内存块 则批量返回 大于对应的批量大小则按照批量进行返回 小于则直接返回剩余的所有内存块;
         * 如果中心缓存数组链表中没有空闲内存块 则获取之后进行批量返回;
         */
        assert(index >= 0 && index < FREE_LIST_SIZE);

        // 自旋锁加锁
        while (this->m_freeListLock[index].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        void *headNode = this->m_freeList[index].load(std::memory_order_acquire);
        size_t fetchNums = this->getBatchNum(index);

        // this->printListSize();

        // 中心缓存中存在空闲内存块
        if (headNode)
        {
            size_t listSize = this->m_freeListSize[index].load(std::memory_order_acquire);
            if (listSize <= fetchNums)
            {
                // 链表中内存块数量小于等于对应的批次大小 则直接返回所有的内存块
                size_t returnNums = listSize;
                void *returnNode = this->m_freeList[index].load(std::memory_order_release);
                this->m_freeList[index].store(nullptr, std::memory_order_relaxed);
                this->m_freeListSize[index].store(0, std::memory_order_relaxed);

                this->m_freeListLock[index].clear();
                return std::make_pair(returnNode, returnNums);
            }
            else
            {
                // 如果链表中的内存块数量大于对应的批次 则可以进行截取 将前面fetchNums个内存块进行返回
                size_t returnNums = fetchNums;
                void *returnNode = this->m_freeList[index].load(std::memory_order_acquire);
                void *curNode = this->m_freeList[index].load(std::memory_order_acquire);


                for(size_t i = 0; i<=returnNums -2; ++i)
                {
                    curNode = *reinterpret_cast<void**>(curNode);
                }
                void *splitNode = *reinterpret_cast<void **>(curNode);
                *reinterpret_cast<void **>(curNode) = nullptr;
                this->m_freeList[index].store(splitNode, std::memory_order_release);
                this->m_freeListSize[index].fetch_sub(fetchNums, std::memory_order_release);

                this->m_freeListLock[index].clear();
                return std::make_pair(returnNode, returnNums);
            }
        }

        // 如果中心缓存中没有内存块 则需要从页面缓存中申请内存页
        // 这里申请的内存页是一大块内存页 还没有进行分割
        size_t blockSize = SizeClass::getBlockSize(index);
        std::pair<void *, size_t> retPair = this->fetchFromPageCache(blockSize);
        void *addr = retPair.first;       // 申请的内存页首地址
        size_t pageNums = retPair.second; // 申请的内存页数量

        size_t totalBytes = PAGE_SIZE * pageNums;
        size_t blockNums = totalBytes / blockSize;

        if (blockNums <= 1)
        {
            // 只有一个构不成链表 直接返回即可
            this->m_freeListLock[index].clear();
            return std::make_pair(addr, blockNums);
        }
        else if (blockNums <= fetchNums)
        {
            // 如果小于等于批次大小 那么可以直接都分配给线程缓存
            // 注意这里需要进行分块
            void *returnNode = addr;
            void *curNode = addr;
            for (size_t i = 0; i <= blockNums - 2; ++i)
            {
                void *nextNode = reinterpret_cast<void *>(reinterpret_cast<size_t>(curNode) + blockSize);
                *reinterpret_cast<void **>(curNode) = nextNode;
                curNode = nextNode;
            }
            *reinterpret_cast<void **>(curNode) = nullptr;

            this->m_freeListLock[index].clear();
            return std::make_pair(returnNode, blockNums);
        }
        else
        {
            // 剩余这种情况就是返回来的内存块数量大于批次数量 需要进行分离

            // 先分割需要返回的这一段链表
            void *returnNode = addr;
            void *curNode = addr;
            for (size_t i = 0; i <= fetchNums - 2; ++i)
            {
                void *nextNode = reinterpret_cast<void *>(reinterpret_cast<size_t>(curNode) + blockSize);
                *reinterpret_cast<void **>(curNode) = nextNode;
                curNode = nextNode;
            }
            *reinterpret_cast<void **>(curNode) = nullptr;

            // 后面计算需要链接到中心缓存链表上的内存块
            void *splitNode = reinterpret_cast<void *>(reinterpret_cast<size_t>(curNode) + blockSize);
            curNode = splitNode;
            for (size_t i = fetchNums; i <= blockNums - 2; ++i)
            {
                void *nextNode = reinterpret_cast<void *>(reinterpret_cast<size_t>(curNode) + blockSize);
                *reinterpret_cast<void **>(curNode) = nextNode;
                curNode = nextNode;
            }
            *reinterpret_cast<void **>(curNode) = nullptr;
            this->m_freeList[index].store(splitNode, std::memory_order_release);
            this->m_freeListSize[index].fetch_add((blockNums - fetchNums), std::memory_order_release);

            
            // std::cout << std::endl << "fetch from PageCache: " << std::endl;
            // this->printListSize();

            this->m_freeListLock[index].clear();
            return std::make_pair(returnNode, fetchNums);
        }

        this->m_freeListLock[index].clear();
        return std::make_pair(nullptr, 0);
    }

    void CentralCache::returnRange(void *ptr, size_t blockNums, size_t index)
    {
        /**
         * 将线程缓存的内存链表归还给中心缓存
         * 整体流程:
         * 参数有效性判断;
         * 自旋锁加锁;
         * 链表进行前插操作;
         */

        assert(ptr != nullptr && blockNums > 0 && index >= 0 && index < FREE_LIST_SIZE);

        while (this->m_freeListLock[index].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        void *curNode = ptr;
        // 找了很久的bug 最后发现是在for循环中(i <= blockNums - 2)没有加上等号
        for (size_t i = 0; i <= blockNums - 2; ++i)
        {
            curNode = *(reinterpret_cast<void **>(curNode));
        }
        void *oldHead = this->m_freeList[index].load(std::memory_order_relaxed);
        *reinterpret_cast<void **>(curNode) = oldHead;
        this->m_freeList[index].store(ptr, std::memory_order_release);
        this->m_freeListSize[index].fetch_add(blockNums, std::memory_order_release);

        this->m_freeListLock[index].clear();
    }

    std::pair<void *, size_t> CentralCache::fetchFromPageCache(size_t size)
    {
        assert(size > 0);

        size_t pageNums = (size - 1 + PAGE_SIZE) / PAGE_SIZE;
        // 如果申请的大小小于固定的4页内存 则申请固定的内存页
        if (pageNums <= SPAN_PAGE)
            return std::make_pair(PageCache::Instance()->allocateSpanPage(SPAN_PAGE), SPAN_PAGE);

        // 如果申请的内存页数量大于4页 则申请对应的内存页数量
        else
            return std::make_pair(PageCache::Instance()->allocateSpanPage(pageNums), pageNums);
    }

    size_t CentralCache::getBatchNum(size_t index)
    {
        /**
         * 根据索引计算每次获取的批量大小
         */
        assert(index >= 0 && index < FREE_LIST_SIZE);

        size_t batchNums = 0;
        if (index <= 3)
            batchNums = 64;
        else if (index <= 7)
            batchNums = 32;
        else if (index <= 15)
            batchNums = 16;
        else if (index <= 31)
            batchNums = 8;
        else if (index <= 63)
            batchNums = 4;
        else if (index <= 127)
            batchNums = 2;
        else
            batchNums = 1;

        return batchNums;
    }

}
