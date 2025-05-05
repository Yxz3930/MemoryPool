#include "MemoryPool_CAS.h"
#include <assert.h>
#include <thread>

namespace memory_pool_CAS
{
    MemoryPool::MemoryPool(size_t blockSize) : m_blockSize(blockSize)
    {
    }

    MemoryPool::~MemoryPool()
    {
        Slot *curNode = this->m_firstBlockPtr;
        while (curNode != nullptr)
        {
            Slot *delNode = curNode;
            curNode = curNode->next;
            operator delete(reinterpret_cast<void *>(delNode));
        }
        this->m_firstBlockPtr = nullptr;
    }

    void MemoryPool::init(size_t slotSize)
    {
        assert(slotSize > 0);
        this->m_slotSize = slotSize;
        this->m_freeSlotPtr = nullptr;
        this->m_curSlotPtr = nullptr;
        this->m_lastSlotPtr = nullptr;
        this->m_firstBlockPtr = nullptr;
    }

    void *MemoryPool::allocate()
    {
        Slot *ptr = this->popFreeSlotList();
        if (ptr != nullptr)
            return ptr;

        Slot *addr = nullptr;
        {
            std::lock_guard<std::mutex> lock(this->m_blockMutex);

            if (this->m_curSlotPtr >= this->m_lastSlotPtr)
                this->allocateNewBlock();

            addr = this->m_curSlotPtr;
            this->m_curSlotPtr = reinterpret_cast<Slot *>(reinterpret_cast<char *>(this->m_curSlotPtr) + this->m_slotSize);
        }

        return addr;
    }

    void MemoryPool::deallocate(void *ptr)
    {
        assert(ptr != nullptr);

        Slot *addr = reinterpret_cast<Slot *>(ptr);
        this->pushFreeSlotList(addr);
    }

    void MemoryPool::allocateNewBlock()
    {
        // 申请一块内存池块
        void *newBlockAddr = operator new(this->m_blockSize);

        // 更新 this->m_firstBlockPtr;
        reinterpret_cast<Slot *>(newBlockAddr)->next = this->m_firstBlockPtr;
        this->m_firstBlockPtr = reinterpret_cast<Slot *>(newBlockAddr);

        // 内存对齐
        char *alignStartAddr = reinterpret_cast<char *>(newBlockAddr) + sizeof(Slot *);
        size_t padSize = this->padPointer(alignStartAddr, this->m_slotSize);
        char *alignEndAddr = alignStartAddr + padSize;

        // 内存槽开始位置 结束位置标志
        this->m_curSlotPtr = reinterpret_cast<Slot *>(alignEndAddr);

        this->m_lastSlotPtr = reinterpret_cast<Slot *>(
            reinterpret_cast<char *>(newBlockAddr) + this->m_blockSize - this->m_slotSize + 1);

        // 空闲内存槽链表指针
        this->m_freeSlotPtr = nullptr;
    }

    size_t MemoryPool::padPointer(char *ptr, size_t align)
    {
        assert(ptr != nullptr && align > 0);

        return (align - reinterpret_cast<size_t>(ptr)) % align;

        // // char*转为size_t
        // size_t addr = reinterpret_cast<size_t>(ptr);

        // // addr相对于align内存槽大小的偏移
        // size_t offset = addr % align;

        // // 计算需要填补的内存大小
        // // size_t padSize = (align - offset) % align;

        // size_t padSize = (offset == 0) ? 0 : (align - offset) % align;

        // return padSize;
    }

    MemoryPool::Slot *MemoryPool::popFreeSlotList()
    {

        while (true)
        {
            Slot *oldHead = this->m_freeSlotPtr.load(std::memory_order_acquire);
            // 空闲内存槽为空 此时直接返回即可
            if (!oldHead)
                return nullptr;

            // 获取下一个节点
            Slot *next;
            try
            {
                next = oldHead->next.load(std::memory_order_relaxed);
            }
            catch (...)
            {
                continue;
            }

            // 更新头节点
            if (this->m_freeSlotPtr.compare_exchange_weak(
                    oldHead, next,
                    std::memory_order_acquire,
                    std::memory_order_relaxed))
            {
                return oldHead;
            }
        }
        return nullptr;
    }

    bool MemoryPool::pushFreeSlotList(Slot *ptr)
    {
        while (true)
        {
            // 获取当前头节点 作为expect使用
            Slot *oldHead = this->m_freeSlotPtr.load(std::memory_order_relaxed);

            // 当前内存槽的next指针指向oldHead
            ptr->next.store(oldHead, std::memory_order_relaxed);

            // 通过compare_exchange_weak判断
            // 如果头节点仍然是原来的那一个(确保其他线程没有改动 仍然是在自己这个线程当中) 就把当前链表头指针指向新添加的内存槽地址
            // 如果失败则下一个while循环重新尝试
            if (this->m_freeSlotPtr.compare_exchange_weak(oldHead, ptr, std::memory_order_release, std::memory_order_relaxed))
            {
                return true;
            }
        }
        return false;
    }

    void HashBucket::initMemoryPool()
    {
        for (int i = 0; i < MEMORYPOOL_MAX_NUM; ++i)
        {
            HashBucket::getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
        }
    }

    MemoryPool &HashBucket::getMemoryPool(int index)
    {
        static MemoryPool memoryPool[MEMORYPOOL_MAX_NUM];
        return memoryPool[index];
    }

    void *HashBucket::useMemory(size_t size)
    {
        assert(size > 0);
        if (size > SLOT_MAX_SIZE)
            return operator new(size);
        else
        {
            int hashIdx = (size - 1) / SLOT_BASE_SIZE;
            return HashBucket::getMemoryPool(hashIdx).allocate();
        }
        return nullptr;
    }

    void HashBucket::freeMemory(void *ptr, size_t size)
    {
        if (!ptr)
            return;
        if (size > SLOT_MAX_SIZE)
            operator delete(ptr);
        else
        {
            int hashIdx = (size - 1) / 8;
            HashBucket::getMemoryPool(hashIdx).deallocate(ptr);
        }
    }
};
