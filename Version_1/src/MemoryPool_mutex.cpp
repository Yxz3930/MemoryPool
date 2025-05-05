#include "MemoryPool_mutex.h"
#include <assert.h>
#include <iostream>

namespace memory_pool
{

    MemoryPool::MemoryPool(size_t blockSize) : m_blockSize(blockSize)
    {
    }

    MemoryPool::~MemoryPool()
    {
        // 释放内存池 不需要操作里面的每个内存槽 而是直接将整个内存池进行删除
        // 需要先转为void*类型 因为void* 类型删除不需要调用析构函数
        Slot *curNode = this->m_firstBlockPtr;
        while (curNode != nullptr)
        {
            Slot *delNode = curNode;
            curNode = curNode->next;

            // operator delete(函数参数)
            operator delete(reinterpret_cast<void *>(delNode));
        }
    }

    void MemoryPool::init(size_t slotSize)
    {
        assert(slotSize > 0 && slotSize % 8 == 0);
        this->m_slotSize = slotSize;
        this->m_freeSlotPtr = nullptr;
        this->m_curSlotPtr = nullptr;
        this->m_lastSlotPtr = nullptr;
        this->m_firstBlockPtr = nullptr;
    }

    void *MemoryPool::allocate()
    {
        // 首先拿取m_freeSlotPtr链表中的内存槽
        if (this->m_freeSlotPtr != nullptr)
        {
            // 加锁 然后再次判断 两次判断需要注意
            {
                std::lock_guard<std::mutex> lock(this->m_freeSlotMutex);
                if (this->m_freeSlotPtr != nullptr)
                {
                    Slot *addr = this->m_freeSlotPtr;
                    this->m_freeSlotPtr = addr->next;
                    return reinterpret_cast<void *>(addr);
                }
            }
        }

        // 问题就出现在这个else上
        // m_freeSlotPtr 没有空闲内存槽 则从curSlot上拿取
        {
            std::lock_guard<std::mutex> lock(this->m_blockMutex);
            if (this->m_curSlotPtr >= this->m_lastSlotPtr)
            {
                this->allocateNewBlock();
            }

            // 链表中没有空闲内存块 则从分割的整个内存池中取内存块
            Slot *addr = this->m_curSlotPtr;
            this->m_curSlotPtr = reinterpret_cast<Slot *>(reinterpret_cast<size_t>(this->m_curSlotPtr) + this->m_slotSize);
            if (addr == nullptr)
                std::cout << "m_curSlotPtr: " << this->m_curSlotPtr << ", addr: " << addr << std::endl;
            return reinterpret_cast<void *>(addr);
        }
        std::cout << "end" << std::endl;
        return nullptr;
    }

    void MemoryPool::deallocate(void *ptr)
    {
        assert(ptr != nullptr);
        if (!ptr)
            return;
        std::lock_guard<std::mutex> lock(this->m_freeSlotMutex);

        if (ptr != nullptr)
        {
            Slot *p = reinterpret_cast<Slot *>(ptr);
            p->next = this->m_freeSlotPtr;
            this->m_freeSlotPtr = p;
        }
    }

    void MemoryPool::allocateNewBlock()
    {
        // 申请新的内存池之后 里面的四个指针都需要进行更新
        // 因为上面调用该函数时加锁了 所以函数里面就不用加锁 否则会出现死锁
        void *newBlockAddr = operator new(this->m_blockSize);

        // 内存池维护链表结构
        reinterpret_cast<Slot *>(newBlockAddr)->next = this->m_firstBlockPtr;
        this->m_firstBlockPtr = reinterpret_cast<Slot *>(newBlockAddr);

        // 内存对齐
        // 需要对齐的开始地址
        char *alignStartAddr = reinterpret_cast<char *>(newBlockAddr) + sizeof(Slot *);
        size_t padSize = padPointer(alignStartAddr, this->m_slotSize);
        char *alignEndAddr = alignStartAddr + padSize;

        // 内存池上的空间内存槽和最后的内存槽标志位
        this->m_curSlotPtr = reinterpret_cast<Slot *>(alignEndAddr);
        this->m_lastSlotPtr = reinterpret_cast<Slot *>(
            reinterpret_cast<size_t>(newBlockAddr) + this->m_blockSize - this->m_slotSize + 1);
        this->m_freeSlotPtr = nullptr;
    }

    size_t MemoryPool::padPointer(char *ptr, size_t align)
    {
        // std::cout << "MemoryPool::padPointer align: " << align << std::endl;
        size_t addr = reinterpret_cast<size_t>(ptr);
        size_t offset = addr % align;
        size_t padSize = (offset == 0) ? 0 : (align - offset) % align;
        return padSize;
    }

    void HashBucket::initMemoryPool()
    {
        for (int i = 0; i < MEMORYPOOL_MAX_NUM; ++i)
        {
            getMemory(i).init((i + 1) * SLOT_BASE_SIZE);
        }
    }

    MemoryPool &HashBucket::getMemory(int index)
    {
        static MemoryPool memoryPoll[MEMORYPOOL_MAX_NUM];
        return memoryPoll[index];
    }

    void *HashBucket::useMemory(size_t size)
    {
        assert(size > 0);
        if (size > 512)
            return operator new(size);

        int hashIdx = (size - 1) / SLOT_BASE_SIZE;
        void *addr = getMemory(hashIdx).allocate();
        return addr;
    }

    void HashBucket::freeMemory(void *ptr, size_t size)
    {
        assert(ptr != nullptr);
        assert(size > 0);
        if (!ptr)
            return;
        assert(size > 0);
        if (ptr == nullptr)
            return;
        if (size > 512)
            operator delete(ptr);

        int hashIdx = (size - 1) / SLOT_BASE_SIZE;
        getMemory(hashIdx).deallocate(ptr);
    }
};