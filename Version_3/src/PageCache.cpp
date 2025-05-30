#include "PageCache.h"


namespace memory_pool
{

    void* PageCache::allocateSpanPage(size_t pageNums)
    {
        /**
         * 从页面缓存中申请内存
         * 整体流程:
         * 参数有效性判断;
         * 如果中心缓存的链表中存在大于等于该内存页数量的内存页 则直接进行分配 并且如果大于pageNums还需要进行分割;
         * 如果中心缓存中没有这样的内存页 则需要从系统中进行申请;
         */
        assert(pageNums > 0);

        std::lock_guard<std::mutex> lock(this->m_pageMutex);

        std::map<size_t, SpanPage*>::iterator it = this->m_freePageMap.lower_bound(pageNums);
        if(it != this->m_freePageMap.end())
        {
            // 如果存在这样的内存页
            SpanPage* spanPage = it->second;
            size_t _pageNums = spanPage->pageNums;
            if(spanPage->pageNums > pageNums)
            {
                SpanPage* newSpanPage = new SpanPage;
                newSpanPage->next = nullptr;
                newSpanPage->pageNums = spanPage->pageNums - pageNums;
                newSpanPage->startAddr = reinterpret_cast<void*>(reinterpret_cast<size_t>(spanPage->startAddr) + pageNums * PAGE_SIZE);

                spanPage->pageNums = pageNums;

                // 将new出来的SpanPage放入到对应的链表当中
                SpanPage* oldHead = this->m_freePageMap[newSpanPage->pageNums];
                newSpanPage->next = oldHead;
                this->m_freePageMap[newSpanPage->pageNums] = newSpanPage;
                
                // 放入到记录的unordered_map当中
                this->m_recordPageMap[newSpanPage->startAddr] = newSpanPage;    
            }
            
            // 将原来的spanPage从链表中移除 只需要将头节点移除即可 因为spanPage本身就是头节点
            SpanPage* nextPage = spanPage->next;
            spanPage->next = nullptr;
            this->m_freePageMap[_pageNums] = nextPage;

            this->m_recordPageMap[spanPage->startAddr] = spanPage;
            return spanPage->startAddr;
        }

        // 如果页面缓存中没有这么大小的内存页 则向系统申请
        void* retAddr = this->systemAlloc(pageNums);
        if(!retAddr)
            return nullptr;
        SpanPage* spanPage = new SpanPage;
        spanPage->next = nullptr;
        spanPage->pageNums = pageNums;
        spanPage->startAddr = retAddr;

        this->m_recordPageMap[spanPage->startAddr] = spanPage;
        return spanPage->startAddr;
    }

    void PageCache::deallocateSpanPage(void *ptr)
    {
        /**
         * 页面缓存将内存进行释放，归还到链表当中
         * 整体思路:
         * 参数有效性判断;
         * 互斥锁加锁;
         * 尝试向右侧合并 可以合并则寻找链表中的内存页进行合并;
         * 不能合并 则直接前插到链表上;
         */
        assert(ptr != nullptr);

        std::lock_guard<std::mutex> lock(this->m_pageMutex);

        auto it = this->m_recordPageMap.find(ptr);
        if(!it->first)
            return;
        
        void* addr = it->first;
        SpanPage* spanPage = it->second;
        void* nextAddr = reinterpret_cast<void*>(reinterpret_cast<size_t>(addr) + spanPage->pageNums * PAGE_SIZE);

        auto nextIt = this->m_recordPageMap.find(nextAddr);
        if(nextIt->first)
        {
            // 如果找到了右侧相邻的内存页 需要查看其是否在链表上
            SpanPage* nextSpanPage = nextIt->second;

            bool isFound = false;
            SpanPage* curNode = this->m_freePageMap[nextSpanPage->pageNums];
            SpanPage* preNode = curNode;
            while(curNode)
            {
                preNode = curNode;
                curNode = curNode->next;
                if(curNode == nextSpanPage)
                {
                    isFound = true;
                    preNode->next = curNode->next;
                    break;
                }
            }

            if(isFound)
            {
                // 消除掉m_recordPageMap中这两个内存页的记录 因为合并之后这俩就没有了 会产生新的内存页需要记录
                this->m_recordPageMap.erase(spanPage->startAddr);
                this->m_recordPageMap.erase(nextSpanPage->startAddr);

                // 记录新的内存页 m_recordPageMap
                spanPage->pageNums += nextSpanPage->pageNums;
                this->m_recordPageMap[spanPage->startAddr] = spanPage;

                // 新的内存页前插到链表中 m_freePageMap
                SpanPage* oldHead = this->m_freePageMap[spanPage->pageNums];
                spanPage->next = oldHead;
                this->m_freePageMap[spanPage->pageNums] = spanPage->next;
                return;
            }
            
        }

        // 没有找到右侧相邻的内存页 或者 找到了右侧相邻内存页但是在空闲链表中没有找到
        SpanPage* oldHead = this->m_freePageMap[spanPage->pageNums];
        spanPage->next = oldHead;
        this->m_freePageMap[spanPage->pageNums] = spanPage->next;

    }

    void *PageCache::systemAlloc(size_t pageNums)
    {
        assert(pageNums > 0);

        void *addr = mmap(nullptr, pageNums * PAGE_SIZE, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if(!addr)
            return nullptr;
        
        memset(addr, 0, pageNums * PAGE_SIZE);

        return addr;
    }

    void PageCache::systemDealloc()
    {
        for(auto& [ptr, spanPage] : this->m_recordPageMap)
        {
            assert(ptr != nullptr);
            munmap(ptr, spanPage->pageNums * PAGE_SIZE);
            delete spanPage;
        }
        this->m_recordPageMap.clear();
        this->m_freePageMap.clear();

    }
}
