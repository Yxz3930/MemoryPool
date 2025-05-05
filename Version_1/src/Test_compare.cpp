/**
 * 比较有互斥锁和无锁链表之间的时间关系
 *
 */

#include "MemoryPool_CAS.h"
#include "MemoryPool_mutex.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <assert.h>
#include <mutex>
#include <numeric>
#include <algorithm>
#include <vector>

void work_mutex(int loopNums)
{
    // std::vector<void *> ptrVec;
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums);
    for (int i = 0; i < loopNums; ++i)
    {
        for (size_t size : {8, 16, 32, 64, 128, 256, 512})
        {
            void *tmp = memory_pool::HashBucket::allocate(size);
            assert(tmp != nullptr);
            ptrVec.push_back({tmp, size});
        }
    }

    for (auto &block : ptrVec)
    {
        assert(block.first != nullptr);
        memory_pool::HashBucket::deallocate(block.first, block.second);
    }
    ptrVec.clear();
}

void work_cas(int loopNums)
{
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums);
    for (int i = 0; i < loopNums; ++i)
    {
        for (size_t size : {8, 16, 32, 64, 128, 256, 512})
        {
            void* tmp = memory_pool_CAS::HashBucket::allocate(size);
            assert(tmp != nullptr);
            ptrVec.push_back({tmp, size});
        }
    }

    for (auto &block : ptrVec)
        memory_pool_CAS::HashBucket::deallocate(block.first, block.second);
    ptrVec.clear();
}

int main()
{
    memory_pool::HashBucket::initMemoryPool();
    memory_pool_CAS::HashBucket::initMemoryPool();

    // 内存池预热
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(1000 * 7);
    for(int i = 0; i<1000; ++i)
    {
        for (size_t size : {8, 16, 32, 64, 128, 256, 512})
        {
            void* tmp = memory_pool::HashBucket::allocate(size);
            assert(tmp != nullptr);
            ptrVec.push_back({tmp, size});
        }
    } 
    for(auto& block : ptrVec)
        memory_pool::HashBucket::deallocate(block.first, block.second);

    ptrVec.clear();
    for(int i = 0; i<1000; ++i)
    {
        for (size_t size : {8, 16, 32, 64, 128, 256, 512})
        {
            void* tmp = memory_pool_CAS::HashBucket::allocate(size);
            assert(tmp != nullptr);
            ptrVec.push_back({tmp, size});
        }
    } 
    for(auto& block : ptrVec)
        memory_pool_CAS::HashBucket::deallocate(block.first, block.second);



    // 测试单线程
    int loopNums = 100000;
    std::chrono::system_clock::time_point start, end;
    int64_t duration_1, duration_2;
    std::vector<double> nums;
    double sum, avg;

    for (int i = 0; i < 10; ++i)
    {
        start = std::chrono::system_clock::now();
        work_mutex(loopNums);
        end = std::chrono::system_clock::now();
        duration_1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "use memory_pool_mutex times: " << duration_1 << "ms" << std::endl;

        start = std::chrono::system_clock::now();
        work_cas(loopNums);
        end = std::chrono::system_clock::now();
        duration_2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "use memory_pool_cas times: " << duration_2 << "ms" << std::endl;

        double reduce = duration_1 - duration_2;
        double ratio = reduce / duration_1 * 100.0; // 在mutex的内存池时间基准上往下降低的比例 时间损耗降低多少
        nums.push_back(ratio);
        std::cout << std::endl;
    }
    sum = std::accumulate(nums.begin(), nums.end(), 0.0);
    avg = sum / 10;
    std::cout << "avg: " << avg << "% time reduce" << std::endl;

    nums.clear();


    // 测试多线程
    std::vector<std::thread> threads;
    int threadNums = 4;
    for (int i = 0; i < 10; ++i)
    {
        start = std::chrono::system_clock::now();
        for (int i = 0; i < threadNums; ++i)
        {
            threads.push_back(std::thread(work_mutex, loopNums));
        }
        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }
        end = std::chrono::system_clock::now();
        duration_1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "use memory_pool_mutex times: " << duration_1 << "ms" << std::endl;
        threads.clear();

        start = std::chrono::system_clock::now();
        for (int i = 0; i < threadNums; ++i)
        {
            threads.push_back(std::thread(work_cas, loopNums));
        }
        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }
        end = std::chrono::system_clock::now();
        duration_2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "use memory_pool_cas times: " << duration_2 << "ms" << std::endl;

        double reduce = duration_1 - duration_2;
        double ratio = reduce / duration_1 * 100.0; // 在mutex的内存池时间基准上往下降低的比例 时间损耗降低多少
        nums.push_back(ratio);
        std::cout << std::endl;
    }
    sum = std::accumulate(nums.begin(), nums.end(), 0.0);
    avg = sum / 10;
    std::cout << "avg: " << avg << "% time reduce" << std::endl;

    nums.clear();



    return 0;
}
