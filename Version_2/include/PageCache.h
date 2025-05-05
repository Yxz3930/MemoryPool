#ifndef PAGE_CACHE_H
#define PAGE_CACHE_H

#include "Common.h"

namespace memory_pool
{

    class PageCache
    {
    public:
        static PageCache *Instance()
        {
            static PageCache instance;
            return &instance;
        }

        ~PageCache(){
            this->clear();
        }

        /// @brief 页面缓存从系统中分配内存
        /// @param numPages
        /// @return
        void *allocateSpanPage(size_t numPages);


        /// @brief 释放对应的内存空间 归还给系统
        /// @param ptr 需要释放的地址
        /// @param numPages 需要释放内存大小(页)
        void deallocateSpanPage(void* ptr, size_t numPages);

        /// @brief 释放PageCache中的所有内存
        void clear();
    
    private:
        struct SpanPage
        {
            void* startAddr;
            size_t numPages;
            SpanPage* next;

            SpanPage(): startAddr(nullptr), numPages(0), next(nullptr) {}
            SpanPage(void* _startAddr, size_t _numPages, SpanPage* _next=nullptr): startAddr(_startAddr), numPages(_numPages), next(_next){}
            ~SpanPage(){
                startAddr = nullptr;
                numPages = 0;
                next = nullptr;
            }
        };


        /// @brief 像系统申请多个内存页 使用mmap
        /// @param numPages 内存页数量
        /// @return 
        void* MmapAllocate(ssize_t numPages);
        
        
        // 记录内存页大小和对应的保存内存页的自由链表 保存着空闲的内存页
        // <size_t pageSize, SpanPage* 内存页管理> 分配内存时从中进行查找
        std::map<size_t, SpanPage*> m_pageFreeMap;

        // 记录从页面缓存中分配出去的，以及在空闲链表中的内存页
        // <void* startAddr, SpanPage* 内存页管理> 
        std::map<void*, SpanPage*> m_recordMemoryMap;

        // 互斥锁
        std::mutex m_pageCacheMutex;
    
    };

}

#endif // PAGE_CACHE_H