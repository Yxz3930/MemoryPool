#include "ThreadCache.h"
#include "CentralCache.h"

namespace memory_pool{

void *ThreadCache::allocate(size_t size)
{
    /**
     * 整体流程：
     * 参数有效性判断;
     * 数组链表中存在内存块如何操作;
     * 数组链表中不存在内存块如何操作;
    */

    assert(size > 0);

    if (size > MAX_BYTES)
        return operator new(size);

    size_t idx = SizeClass::getIndex(size);
    void *headNode = this->m_freeList[idx];

    // 链表中没有空闲内存块 则从中心缓存中拿取内存
    if (!headNode)
        return this->fetchFromCentralCache(idx);
    // 链表中存在空闲内存块 直接进行分配
    else
    {
        void *next = *reinterpret_cast<void **>(headNode);
        *reinterpret_cast<void**>(headNode) = nullptr; // 内存块使用之前将执行的地址转为nullptr
        this->m_freeList[idx] = next;
        this->m_freeListSize[idx]--;
    }

    return headNode;
}

void ThreadCache::deallocate(void *ptr, size_t size)
{
    /**
     * 线程缓存释放内存 将内存归还到数组链表当中
     * 整体思路：
     * 参数有效性判断;
     * 将内存块前插到对应链表当中;
     * 判断一下是否需要向中心缓存归还内存链表 需要的话进行归还
     */

    assert(size > 0 && ptr != nullptr);

    // 对象在销毁之后 最好将指向的内存地址中写入nullptr 确保while循环遍历链表的时候不会出问题

    if (size > MAX_BYTES)
    {
        operator delete(ptr);
        return;
    }

    size_t idx = SizeClass::getIndex(size);
    void *headNode = this->m_freeList[idx];
    *reinterpret_cast<void **>(ptr) = headNode;
    this->m_freeList[idx] = ptr;
    this->m_freeListSize[idx]++;

    // 判断一下当当前链表是否需要回收到中心缓存
    if (this->shouldReturntoCentralCache(idx))
    {
        this->returnToCentralCache(idx);
    }
}

void *ThreadCache::fetchFromCentralCache(size_t index)
{
    /**
     * 整体思路：
     * 线程缓存中没有对应大小的内存块 需要从中心缓存中获取
     * 参数有效性判断;
     * 从中心缓存中获取该大小的内存块(这里只获取到了一个内存块)
     * 后续这里可以进行赶紧 将线程缓存从中心缓存申请内存改为批量
     */

    assert(index >= 0 && index < FREE_LIST_SIZE);

    // 从中心缓存上获取一批内存块(内存链表)
    void *addr = CentralCache::Instance()->fetchRange(index);
    if (!addr)
        return nullptr;

    *reinterpret_cast<void**>(addr) = nullptr;
    // 这里只写获取单个内存块的情况
    return addr;

/*
    下面这种是批量获取内存的情况
    // 从内存链表中拿取第一个进行返回 其余的放在m_freeList链表上面
    void *ret = addr;
    void *start = *reinterpret_cast<void **>(ret);
    this->m_freeList[index] = start;

    // 统计链表中内存块的个数 但是可以每次固定一个批次数量
    size_t batchNum = 0;
    void *curNode = start;
    while (curNode)
    {
        curNode = *reinterpret_cast<void **>(curNode);
        ++batchNum;
    }

    this->m_freeListSize[index] += batchNum;

    return ret;
*/
}


bool ThreadCache::shouldReturntoCentralCache(size_t index)
{
    /**
     * 判断是否愮进行内存链表归还
     * 设定固定的大小阈值
     */
    assert(index >= 0 && index < FREE_LIST_SIZE);

    if (this->m_freeListSize[index] > 64)
        return true;
    return false;
}

void ThreadCache::returnToCentralCache(size_t index)
{
    /**
     * 向中心缓存归还内存链表思路:
     * 参数有效性判断;
     * 计算需要归还的内存块数量, 用于进行链表的遍历与分离;
     * 如果链表长度不够 则不需要进行分离;
     */
    assert(index >= 0 && index < FREE_LIST_SIZE);

    // 计算当前链表需要保留的内存块数量
    size_t keepNum = std::max((size_t)this->m_freeListSize[index] / 4, (size_t)(1));
    size_t returnNum = this->m_freeListSize[index] - keepNum;

    // 找到链表上需要分割的内存块索引位置 第keepNum个内存块 对应索引是keepNum - 1
    void* curNode = this->m_freeList[index];
    for (size_t i = 0; i < keepNum - 1; ++i)
    {
        curNode = *reinterpret_cast<void**>(curNode);
        
        // 如果还没找完就已经变为nullptr了 说明不够keepNum 直接返回即可
        if(!curNode)
            return;
    }

    // 找到下一个开始分割的第一个节点
    void* splitNode = *reinterpret_cast<void**>(curNode);
    *reinterpret_cast<void**>(curNode) = nullptr;

    this->m_freeListSize[index] = keepNum;

    // 如果链表分割的位置后面不为空 就将这部分归还给中心缓存
    CentralCache::Instance()->returnRange(splitNode, returnNum, index);
}

}