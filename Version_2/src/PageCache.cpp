#include "PageCache.h"
#include <sys/mman.h>
#include <cstring>

namespace memory_pool
{

    void *PageCache::allocateSpanPage(size_t numPages)
    {
        /**
         * 页面缓存分配内存
         * 整体思路：
         * 参数有效性判断;
         * 添加互斥锁;
         * 如果map中存在对应大小的内存页 直接分配 如果内存页大于分配的内存页则需要进行分割;
         * 如果map中不存在对应大小的内存页 那么需要通过mmap向系统申请内存;
         * 分配之后需要在另外一个map上记录一下 后续方面通过该map进行查找分配出去的内存
         * 
         */
        assert(numPages > 0);
        std::lock_guard<std::mutex> lock(this->m_pageCacheMutex);

        // lower_bound 找到最小的大于等于numPages 的内存页大小
        std::map<size_t, SpanPage *>::iterator it = this->m_pageFreeMap.lower_bound(numPages);

        // 如果找到了对应大小的内存页
        if (it != this->m_pageFreeMap.end())
        {
            // 获取对应大小的内存页
            SpanPage *spanPage = it->second;

            // 链表操作 判断是否只存在一个节点
            if (spanPage->next == nullptr)
                this->m_pageFreeMap.erase(it);
            else
                it->second = spanPage->next;

            // 如果内存页的大小和申请的内存页大小一致 那么直接分配即可
            if (spanPage->numPages > numPages)
            {
                // 如果内存页大小比申请的内存页要大 那么就需要进行分割 开辟新的SpanPage来指向分割后的内存
                SpanPage *newSpanPage = new SpanPage();

                newSpanPage->next = nullptr;
                newSpanPage->numPages = spanPage->numPages - numPages;
                newSpanPage->startAddr = reinterpret_cast<void *>(reinterpret_cast<size_t>(spanPage->startAddr) + numPages * PAGE_SIZE);

                // 将剩余的Page存放在对应的m_pageFreeMap当中 链表前插
                SpanPage *oldHeadNode = this->m_pageFreeMap[newSpanPage->numPages];
                newSpanPage->next = oldHeadNode;
                this->m_pageFreeMap[newSpanPage->numPages] = newSpanPage;

                // newSpanPage记录到m_recordMemoryMap中
                this->m_recordMemoryMap[newSpanPage->startAddr] = newSpanPage; 

                // 分割之后内存页数量等于给定的参数值
                spanPage->numPages = numPages;
            }
            // 这里也要记录一下 因为这是从大的内存页中分出去的一部分
            this->m_recordMemoryMap[spanPage->startAddr] = spanPage;
            return spanPage->startAddr;
        }
        // 如果没有找到对应大小的内存页 那么就需要从系统中申请
        else
        {
            void *addr = this->MmapAllocate(numPages);
            SpanPage *spanPage = new SpanPage();
            spanPage->next = nullptr;
            spanPage->numPages = numPages;
            spanPage->startAddr = addr;

            // 将申请的内存在map中记录一下 用于最后的释放内存
            this->m_recordMemoryMap[addr] = spanPage;

            return addr;
        }
        return nullptr;
    }

    void PageCache::deallocateSpanPage(void *ptr, size_t numPages)
    {
        /**
         * 将内存回收到页面缓存的map中
         * 整体流程:
         * 参数有效性判断;
         * 添加互斥锁;
         * 判断ptr是否是由内存池分配的;
         * 判断右侧相邻内存页是否可以合并，如果可以合并则进行合并，并添加到map的链表当中
         * 如果不可以合并，则直接添加到map的链表当中
         */

        if (!ptr || numPages <= 0)
            return;

        std::lock_guard<std::mutex> lock(this->m_pageCacheMutex);

        // 在m_recordMemoryMap中寻找该地址 如果可以找到说明就是从内存池中分配出去的
        std::map<void *, SpanPage *>::iterator it = this->m_recordMemoryMap.find(ptr);
        if (it == this->m_recordMemoryMap.end())
            return;

        if (it != this->m_recordMemoryMap.end())
        {
            SpanPage *spanPage = it->second;
            // 计算下一个地址 这里是将后面地址的内存进行合并 这样可以减少内存碎片
            void *nextAddr = reinterpret_cast<void *>(reinterpret_cast<char *>(spanPage->startAddr) + spanPage->numPages * PAGE_SIZE);

            // 检查在m_recordMemoryMap中是否存在该地址
            std::map<void *, SpanPage *>::iterator nextAddrIt = this->m_recordMemoryMap.find(nextAddr);

            bool isFound = false;
            if (nextAddrIt != this->m_recordMemoryMap.end())
            {
                // 在 m_recordMemoryMap 中可以找到 说明右侧相邻地址也是内存池分配出去的 可以尝试合并
                // 需要先在m_pageFreeMap中对应的链表上查找 看是否存在 存在的话需要先从链表中分离出来 然后再进行合并

                SpanPage *nextSpanPage = nextAddrIt->second;

                // 从m_pageFreeMap的空闲链表中查找是否存在这块地址
                SpanPage *curNode = this->m_pageFreeMap[nextSpanPage->numPages];
                
                // 因为要分离出来对应的节点 所以需要寻找该节点的前一个节点 如果不判断第一个节点的话就会漏掉
                if (curNode == nextSpanPage)
                {
                    this->m_pageFreeMap[nextSpanPage->numPages] = curNode->next;
                    isFound = true;
                }
                // 这里需要确保链表不是空的
                else if (curNode)
                {
                    while (curNode->next)
                    {
                        if (curNode->next == nextSpanPage)
                        {
                            isFound = true;
                            curNode->next = nextSpanPage->next;
                            break;
                        }
                        curNode = curNode->next;
                    }
                }
            }

            if (isFound)
            {
                SpanPage *nextSpanPage = nextAddrIt->second;
                spanPage->numPages += nextSpanPage->numPages;
                spanPage->next = this->m_pageFreeMap[spanPage->numPages];
                this->m_pageFreeMap[spanPage->numPages] = spanPage;
                 
                this->m_recordMemoryMap.erase(nextSpanPage->startAddr);
                delete nextSpanPage;
            }
            else
            {
                // 如果没有找到相邻的地址 那么直接插入到m_pageFreeMap对应链表中即可
                SpanPage *oldHead = this->m_pageFreeMap[spanPage->numPages];
                spanPage->next = oldHead;
                this->m_pageFreeMap[spanPage->numPages] = spanPage;
            }
        }
    }

    void PageCache::clear()
    {
        /**
         * 释放页面缓存中的所有内存
         * 整体流程:
         * 添加互斥锁
         * 释放m_recordMemoryMap中记录所有已经分配的或者未分配保存在m_pageFreeMap链表中的内存页;
         * 释放m_pageFreeMap中new出来的SpanPage*对象
         */

        std::lock_guard<std::mutex> lock(this->m_pageCacheMutex);

        // 释放m_recordMemoryMap中的内存
        for(auto& [ptr, spanPage] : this->m_recordMemoryMap)
        {
            munmap(ptr, spanPage->numPages * PAGE_SIZE);
        }
        this->m_recordMemoryMap.clear();

        // 释放m_pageFreeMap中new出来的SpanPage*对象
        for(auto& [numPages, spanPage] : this->m_pageFreeMap)
        {
            SpanPage* tmp = spanPage;
            spanPage = spanPage->next;
            delete tmp;
        }
        this->m_pageFreeMap.clear();

    }

    void *PageCache::MmapAllocate(ssize_t numPages)
    {
        /**
         * 向系统申请内存
         * 整体流程:
         * 参数有效性判断;
         * 根据内存页数量计算出来对应的内存大小;
         * mmap申请内存;
         * 内存初始化;
         */

        ssize_t blockSize = numPages * PAGE_SIZE;

        void *addr = mmap(nullptr, blockSize, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (!addr)
            return nullptr;

        // 内存初始化为零
        memset(addr, 0, blockSize);

        return addr;
    }
}