#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <iostream>
#include <mutex>

// 该版本的线程池使用互斥锁来保证操作原子性
namespace memory_pool{
/*
该版本内存池本身是通过链表加锁的模式进行操作 没有采用无锁队列
*/

#define MEMORYPOOL_MAX_NUM 64
#define SLOT_BASE_SIZE 8
#define SLOT_MAX_SIZE 512


class MemoryPool
{

public:
    MemoryPool(size_t blockSize = 4096);
    ~MemoryPool();

    /// @brief 初始化内存池中的变量
    /// @param slotSize 内存槽大小
    void init(size_t slotSize);

    /* 申请内存和释放内存 */

    /// @brief 从内存池中申请内存 因为在对应的内存池中内存槽大小固定 所以这里就不需要所需要的内存大小作为参数了
    /// @return void* 指针
    void* allocate(); 

    /// @brief 归还内存给内存池
    /// @param ptr 待归还的地址
    void deallocate(void* ptr);


private:
    /// @brief 申请新的内存池
    void allocateNewBlock();

    /// @brief 内存对齐 补齐首地址指针后面的内存空缺
    /// @param ptr 首地址 char* 类型 实际上size_t 也是一样的
    /// @param align 槽大小 即slotSize
    /// @return 需要补齐所需要的字节大小
    size_t padPointer(char* ptr, size_t align);
    

private:
    struct Slot
    {
        Slot* next;
    };

    int m_slotSize;
    int m_blockSize;
    Slot* m_freeSlotPtr;
    Slot* m_curSlotPtr;
    Slot* m_lastSlotPtr;
    Slot* m_firstBlockPtr;

    std::mutex m_freeSlotMutex; // 往freeSlotPtr链表中添加内存槽对应的互斥锁, 保证链表添加和使用的原子性
    std::mutex m_blockMutex; // 内存不够时申请新的内存对应的互斥锁, 避免多线程下重复开辟内存池

};


class HashBucket
{
public:
    /// @brief 初始化每个哈希桶中的内存池
    static void initMemoryPool();


    /// @brief 获取哈希桶中对应索引位置的内存池 单例模式
    /// @param index 索引位置
    /// @return MemoryPool& 对应内存池引用
    static MemoryPool& getMemory(int index);


    /// @brief 使用哈希桶中的某个内存池的内存槽 是对内存池中allocate函数的封装
    /// @param size 需要分配的内存大小 用来计算对应的哈希桶内存池索引
    /// @return 返回对应哈希桶中内存池中的某一个内存槽的地址
    static void* useMemory(size_t size);


    /// @brief 释放空间 将内存归还给内存池 是对内存池中deallocate函数的封装
    /// @param ptr 待释放的地址
    /// @param size 对象大小 用于计算是哪一个哈希桶对应的内存池
    static void freeMemory(void* ptr, size_t size);


    /* newElement && deleteElement */
    
    /// @brief 创建对象 先分配内存 然后在得到的内存地址上构造对象
    /// @tparam T 
    /// @tparam ...Args 
    /// @param ...args 
    /// @return T* 类型的指针
    template<class T, class... Args>
    friend T* newElement(Args... args);

    /// @brief 销毁对象 先调用析构 再归还内存
    /// @tparam T 
    /// @param ptr 
    template<class T>
    friend void deleteElement(T* ptr);

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
T *newElement(Args&&... args)
{
    // 开辟内存地址
    void* ptr = HashBucket::useMemory(sizeof(T));
    // std::cout << "newElement ptr: " << ptr << std::endl;
    if(ptr == nullptr)
        return nullptr;
    
    // 在该地址上构造对象
    T* p = new (ptr) T(std::forward<Args>(args)...);

    // std::cout << "newElement p: "  << p << std::endl;
    // std::cout << "newElement *p: " << *p << std::endl;

    return p;
}

template <class T>
void deleteElement(T *ptr)
{
    if(ptr == nullptr)
        return;
    
    // 先析构
    ptr->~T();

    // 再归还内存
    HashBucket::freeMemory(reinterpret_cast<void*>(ptr), sizeof(T));
}
};

#endif // MEMORY_POOL_H