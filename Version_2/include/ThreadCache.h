#ifndef THREAD_CACHE_H
#define THREAD_CACHE_H

#include "Common.h"


namespace memory_pool{


class ThreadCache
{
public:

    /// @brief 单例模式 懒汉式 使用thread_local确保每个线程之间的实例相互独立
    /// @return 
    static ThreadCache* Instance()
    {
        static thread_local ThreadCache instance;
        return &instance;
    }

    /* 开辟与销毁内存 */
    
    /// @brief 分配内存 
    /// @param size 对象大小 用于寻找对应内存块大小的索引位置
    /// @return void*类型的地址
    void* allocate(size_t size);

    /// @brief 将内存归还给线程缓存
    /// @param ptr 待归还地址
    /// @param size 对象大小 用于寻找对应内存块大小的索引位置
    void deallocate(void* ptr, size_t size);

private:
    
    /// @brief 单例模式隐藏构造函数 同时创建对类对象时会调用构造函数
    ThreadCache()
    {
        this->m_freeList.fill(nullptr);
        this->m_freeListSize.fill(0);
    }

    /// @brief 从中心缓存上获取内存
    /// @param index 自由链表的索引位置 即array上链表索引位置
    /// @return void* 地址
    void* fetchFromCentralCache(size_t index);

    /// @brief 判断当前索引的链表中内存块数量是否超过指定数量
    /// @param index 指定的链表索引
    /// @return bool
    bool shouldReturntoCentralCache(size_t index);

    /// @brief 将线程缓存归中的链表内存块还给中心缓存
    /// @param index 自由链表的索引位置 即array上链表索引位置
    void returnToCentralCache(size_t index);

private:

    // 自由链表
    std::array<void*, FREE_LIST_SIZE> m_freeList;
    
    // 自由链表上每个链表的大小
    std::array<size_t, FREE_LIST_SIZE> m_freeListSize;

};

}

















#endif // THREAD_CACHE_H