// /**
//  * 比较有互斥锁和无锁链表之间的时间关系
//  *
//  */

// #include "MemoryPool_CAS.h"
// #include "MemoryPool_mutex.h"
// #include <iostream>
// #include <chrono>
// #include <thread>
// #include <assert.h>
// #include <mutex>
// #include <numeric>
// #include <algorithm>
// #include <vector>

// void work_mutex(int loopNums)
// {
//     // std::vector<void *> ptrVec;
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(loopNums);
//     for (int i = 0; i < loopNums; ++i)
//     {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512})
//         {
//             void *tmp = memory_pool::HashBucket::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.push_back({tmp, size});
//         }
//     }

//     for (auto &block : ptrVec)
//     {
//         assert(block.first != nullptr);
//         memory_pool::HashBucket::deallocate(block.first, block.second);
//     }
//     ptrVec.clear();
// }

// void work_cas(int loopNums)
// {
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(loopNums);
//     for (int i = 0; i < loopNums; ++i)
//     {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512})
//         {
//             void* tmp = memory_pool_CAS::HashBucket::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.push_back({tmp, size});
//         }
//     }

//     for (auto &block : ptrVec)
//         memory_pool_CAS::HashBucket::deallocate(block.first, block.second);
//     ptrVec.clear();
// }

// int main()
// {
//     memory_pool::HashBucket::initMemoryPool();
//     memory_pool_CAS::HashBucket::initMemoryPool();

//     // 内存池预热
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(1000 * 7);
//     for(int i = 0; i<1000; ++i)
//     {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512})
//         {
//             void* tmp = memory_pool::HashBucket::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.push_back({tmp, size});
//         }
//     } 
//     for(auto& block : ptrVec)
//         memory_pool::HashBucket::deallocate(block.first, block.second);

//     ptrVec.clear();
//     for(int i = 0; i<1000; ++i)
//     {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512})
//         {
//             void* tmp = memory_pool_CAS::HashBucket::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.push_back({tmp, size});
//         }
//     } 
//     for(auto& block : ptrVec)
//         memory_pool_CAS::HashBucket::deallocate(block.first, block.second);



//     // 测试单线程
//     int loopNums = 100000;
//     std::chrono::system_clock::time_point start, end;
//     int64_t duration_1, duration_2;
//     std::vector<double> nums;
//     double sum, avg;

//     for (int i = 0; i < 10; ++i)
//     {
//         start = std::chrono::system_clock::now();
//         work_mutex(loopNums);
//         end = std::chrono::system_clock::now();
//         duration_1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         std::cout << "use memory_pool_mutex times: " << duration_1 << "ms" << std::endl;

//         start = std::chrono::system_clock::now();
//         work_cas(loopNums);
//         end = std::chrono::system_clock::now();
//         duration_2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         std::cout << "use memory_pool_cas times: " << duration_2 << "ms" << std::endl;

//         double reduce = duration_1 - duration_2;
//         double ratio = reduce / duration_1 * 100.0; // 在mutex的内存池时间基准上往下降低的比例 时间损耗降低多少
//         nums.push_back(ratio);
//         std::cout << std::endl;
//     }
//     sum = std::accumulate(nums.begin(), nums.end(), 0.0);
//     avg = sum / 10;
//     std::cout << "avg: " << avg << "% time reduce" << std::endl;

//     nums.clear();


//     // 测试多线程
//     std::vector<std::thread> threads;
//     int threadNums = 4;
//     for (int i = 0; i < 10; ++i)
//     {
//         start = std::chrono::system_clock::now();
//         for (int i = 0; i < threadNums; ++i)
//         {
//             threads.push_back(std::thread(work_mutex, loopNums));
//         }
//         for (auto &t : threads)
//         {
//             if (t.joinable())
//                 t.join();
//         }
//         end = std::chrono::system_clock::now();
//         duration_1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         std::cout << "use memory_pool_mutex times: " << duration_1 << "ms" << std::endl;
//         threads.clear();

//         start = std::chrono::system_clock::now();
//         for (int i = 0; i < threadNums; ++i)
//         {
//             threads.push_back(std::thread(work_cas, loopNums));
//         }
//         for (auto &t : threads)
//         {
//             if (t.joinable())
//                 t.join();
//         }
//         end = std::chrono::system_clock::now();
//         duration_2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         std::cout << "use memory_pool_cas times: " << duration_2 << "ms" << std::endl;

//         double reduce = duration_1 - duration_2;
//         double ratio = reduce / duration_1 * 100.0; // 在mutex的内存池时间基准上往下降低的比例 时间损耗降低多少
//         nums.push_back(ratio);
//         std::cout << std::endl;
//     }
//     sum = std::accumulate(nums.begin(), nums.end(), 0.0);
//     avg = sum / 10;
//     std::cout << "avg: " << avg << "% time reduce" << std::endl;

//     nums.clear();



//     return 0;
// }


#include "MemoryPool_CAS.h"
#include "MemoryPool_mutex.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <numeric>
#include <cassert>

constexpr size_t blockSizes[] = {8, 16, 32, 64, 128, 256, 512};

// ==================== 单线程任务函数 ====================
void work_mutex(int loopNums)
{
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums * std::size(blockSizes));
    for (int i = 0; i < loopNums; ++i)
        for (size_t size : blockSizes)
            ptrVec.emplace_back(memory_pool::HashBucket::allocate(size), size);

    for (auto& block : ptrVec)
        memory_pool::HashBucket::deallocate(block.first, block.second);
}

void work_cas(int loopNums)
{
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums * std::size(blockSizes));
    for (int i = 0; i < loopNums; ++i)
        for (size_t size : blockSizes)
            ptrVec.emplace_back(memory_pool_CAS::HashBucket::allocate(size), size);

    for (auto& block : ptrVec)
        memory_pool_CAS::HashBucket::deallocate(block.first, block.second);
}

// ==================== 预热函数 ====================
void warmup()
{
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(1000 * std::size(blockSizes));
    for (int i = 0; i < 1000; ++i)
        for (size_t size : blockSizes)
            ptrVec.emplace_back(memory_pool::HashBucket::allocate(size), size);

    for (auto& block : ptrVec)
        memory_pool::HashBucket::deallocate(block.first, block.second);
    ptrVec.clear();

    for (int i = 0; i < 1000; ++i)
        for (size_t size : blockSizes)
            ptrVec.emplace_back(memory_pool_CAS::HashBucket::allocate(size), size);

    for (auto& block : ptrVec)
        memory_pool_CAS::HashBucket::deallocate(block.first, block.second);
}

// ==================== 性能测试函数 ====================
template<typename Func>
double test_function(Func f, int loopNums, int threadNums = 1)
{
    auto start = std::chrono::high_resolution_clock::now();

    if (threadNums == 1)
    {
        f(loopNums);
    }
    else
    {
        std::vector<std::thread> threads;
        threads.reserve(threadNums);
        for (int i = 0; i < threadNums; ++i)
            threads.emplace_back(f, loopNums);

        for (auto& t : threads)
            if (t.joinable()) t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// ==================== 主函数 ====================
int main()
{
    memory_pool::HashBucket::initMemoryPool();
    memory_pool_CAS::HashBucket::initMemoryPool();

    warmup();

    const int loopNums = 100000;
    const int repeatTimes = 10;
    const int threadNums = 4;

    std::vector<double> singleThreadRatios;
    std::vector<double> multiThreadRatios;

    std::cout << "========= 单线程测试 =========\n";
    for (int i = 0; i < repeatTimes; ++i)
    {
        double t1 = test_function(work_mutex, loopNums);
        double t2 = test_function(work_cas, loopNums);

        double ratio = (t1 - t2) / t1 * 100.0;
        singleThreadRatios.push_back(ratio);

        std::cout << "Round " << i + 1 << ": Mutex = " << t1 << " ms, CAS = " << t2 << " ms, speedup = " << ratio << " %\n";
    }

    double avgSingle = std::accumulate(singleThreadRatios.begin(), singleThreadRatios.end(), 0.0) / repeatTimes;
    std::cout << "Average Time speedup (Single Thread): " << avgSingle << " %\n\n";

    std::cout << "========= 多线程测试 (" << threadNums << " 线程) =========\n";
    for (int i = 0; i < repeatTimes; ++i)
    {
        double t1 = test_function(work_mutex, loopNums, threadNums);
        double t2 = test_function(work_cas, loopNums, threadNums);

        double ratio = (t1 - t2) / t1 * 100.0;
        multiThreadRatios.push_back(ratio);

        std::cout << "Round " << i + 1 << ": Mutex = " << t1 << " ms, CAS = " << t2 << " ms, speedup = " << ratio << " %\n";
    }

    double avgMulti = std::accumulate(multiThreadRatios.begin(), multiThreadRatios.end(), 0.0) / repeatTimes;
    std::cout << "Average Time speedup (Multi Thread): " << avgMulti << " %\n";

    return 0;
}
