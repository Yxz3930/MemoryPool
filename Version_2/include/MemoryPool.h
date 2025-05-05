#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "Common.h"
#include "ThreadCache.h"


namespace memory_pool{



class MemoryPool{


public:
    
    static void* allocate(size_t size)
    {
        return ThreadCache::Instance()->allocate(size);
    }

    static void deallocate(void* ptr, size_t size)
    {
        return ThreadCache::Instance()->deallocate(ptr, size);
    }



    template<class T, class... Args>
    static T* newElement(Args... args)
    {
        // 申请内存
        size_t type_size = sizeof(T);
        void* addr = ThreadCache::Instance()->allocate(type_size);

        // 构造对象
        T* ptr = new (addr) T(std::forward<Args>(args)...);
        return ptr;
    }

    template<class T>
    static void deleteElement(T* ptr)
    {
        // 申请内存
        size_t type_size = sizeof(T);
        ThreadCache::Instance()->deallocate(ptr, type_size);
    }


};







}










#endif // MEMORY_POOL_H