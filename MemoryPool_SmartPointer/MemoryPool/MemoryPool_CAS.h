#ifndef MEMORY_POOL_CAS_H
#define MEMORY_POOL_CAS_H

// 该版本的内存池使用无所队列结构进行内存槽的插入和弹出
#include <mutex>
#include <atomic>

namespace memory_pool_CAS{

#define MEMORYPOOL_MAX_NUM 64
#define SLOT_BASE_SIZE 8
#define SLOT_MAX_SIZE 512


class MemoryPool
{
    struct Slot
    {
        std::atomic<Slot*> next;
    };
public:

    /// @brief 构造函数
    /// @param blockSize 内存池块大小 
    MemoryPool(size_t blockSize = 4096);

    /// @brief 析构函数 释放内存池空间
    ~MemoryPool();

    /// @brief 初始化内存池参数
    /// @param slotSize 内存槽大小
    void init(size_t lotSize);

    /// @brief 从内存池中分配内存
    /// @param size 
    /// @return 
    void* allocate();

    /// @brief 将内存归还给内存池
    /// @param ptr 待回收的地址
    void deallocate(void* ptr);

private:
    /// @brief 分配新的内存池块 需要内存对齐 4个Slot*指针指向更新
    void allocateNewBlock();

    /// @brief 对起始位置的Slot*剩余内存进行对齐
    /// @param ptr Slot*后面的起始地址
    /// @param align 内存槽大小
    /// @return 
    size_t padPointer(char* ptr, size_t align);

    /// @brief 从无锁链表中弹出一个内存槽 并返回对应的地址
    /// @return 
    Slot* popFreeSlotList();


    /// @brief 将内存槽压入无锁链表当中 前插
    /// @param ptr 待插入的地址指针
    /// @return 
    bool pushFreeSlotList(Slot* ptr);


private:

    int m_slotSize;
    int m_blockSize;
    // Slot* m_freeSlotPtr;
    std::atomic<Slot*> m_freeSlotPtr;
    Slot* m_curSlotPtr;
    Slot* m_lastSlotPtr;
    Slot* m_firstBlockPtr;

    std::mutex m_blockMutex;

};


class HashBucket
{
public:
    /// @brief 对哈希桶中的每个内存池块进行初始化
    static void initMemoryPool();


    /// @brief 获取索引位置上的内存池
    /// @param index 哈希桶索引 
    /// @return 
    static MemoryPool& getMemoryPool(int index);


    /// @brief 开辟内存空间 对allocate的包装
    /// @param size 需要开辟的大小 用于计算哈希桶索引
    /// @return void*指针
    static void* useMemory(size_t size);


    /// @brief 释放内存 归还给内存池
    /// @param ptr 待归还的指针
    /// @param size 对象大小 用于计算哈希桶索引
    static void freeMemory(void* ptr, size_t size);


    template<class T, class... Args>
    static T* newElement(Args&&... args);

    template<class T>
    static void deleteElement(T* ptr);

    static void* allocate(size_t size)
    {
        return HashBucket::useMemory(size);
    }

    static void deallocate(void* ptr, size_t size)
    {
        HashBucket::freeMemory(ptr, size);
    }

};

template <class T, class... Args>
T *HashBucket::newElement(Args &&...args)
{
    void* addr = HashBucket::useMemory(sizeof(T));
    if(!addr)
        return nullptr;
    else
        return new (addr) T(std::forward<Args>(args)...);

}

template <class T>
void HashBucket::deleteElement(T* ptr)
{
    if(!ptr)
        return;

    // 先析构
    ptr->~T();

    // 在归还内存
    void* p = reinterpret_cast<void*>(ptr);
    HashBucket::freeMemory(p, sizeof(T));
}
};

#endif // MEMORY_POOL_CAS_H