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

        ~PageCache()
        {
            this->systemDealloc();
        }

        /// @brief 向页面缓存申请内存页
        /// @param pageNums 内存页数量
        /// @return void*
        void *allocateSpanPage(size_t pageNums);

        /// @brief 将待释放的内存页归还给页面缓存
        /// @param ptr 待归还的内存页首地址
        void deallocateSpanPage(void *ptr);

    private:
        /// @brief 通过mmap进行内存页申请
        /// @param pageNums 申请的内存页数量 用于计算总大小
        /// @return
        void *systemAlloc(size_t pageNums);

        /// @brief 系统通过munmap进行内存的释放
        void systemDealloc();

    private:
        struct SpanPage
        {
            void *startAddr;
            size_t pageNums;
            SpanPage *next;

            SpanPage() : startAddr(nullptr), pageNums(0), next(nullptr) {}
            SpanPage(void *_startAddr, size_t _pageNums = 0, SpanPage *_next = nullptr) : 
                                                        startAddr(_startAddr), pageNums(_pageNums), next(_next) {}
            ~SpanPage()
            {
                startAddr = nullptr;
                pageNums = 0;
                next = nullptr;
            }
        };

        // 页面缓存的互斥锁
        std::mutex m_pageMutex;

        // 保存内存页大小和SpanPage*的链表Map 第一个位置是内存页的大小 第二个位置是内存页链表
        std::map<size_t, SpanPage *> m_freePageMap;

        // 记录分配出去的以及保存在m_freePageMap中的内存页 第一个位置是内存页地址 第二个位置是内存页的地址(不是链表)
        std::unordered_map<void *, SpanPage *> m_recordPageMap;
    };

}
#endif // PAGE_CACHE_H