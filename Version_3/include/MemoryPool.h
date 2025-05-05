#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "Common.h"
#include "ThreadCache.h"

namespace memory_pool
{


class MemoryPool
{
public:
    static void* allocate(size_t size)
    {
        return ThreadCache::Instance()->allocate(size);
    }

    static void deallocate(void* ptr, size_t size)
    {
        return ThreadCache::Instance()->deallocate(ptr, size);
    }

};




}

#endif // MEMORY_POOL_H