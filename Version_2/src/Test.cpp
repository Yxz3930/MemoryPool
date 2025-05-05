#include "MemoryPool.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <vector>
using namespace memory_pool;






void warmUp()
{
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(1000 * 7);
    for (int i = 0; i < 1000; ++i)
    {
        for (size_t size : {8, 16, 32, 64, 128, 256, 512})
        // for (size_t size : {32})
        {
            void *tmp = MemoryPool::allocate(size);
            assert(tmp != nullptr);
            ptrVec.push_back({tmp, size});
        }
    }

    for (auto &block : ptrVec)
    {
        assert(block.first != nullptr);
        MemoryPool::deallocate(block.first, block.second);
    }
    ptrVec.clear();
}

void workUseMemory(int loopNums)
{
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums * 7);
    for (int i = 0; i < loopNums; ++i)
    {
        for (size_t size : {8, 16, 32, 64, 128, 256, 512})
        // for (size_t size : {32})
        {
            void *tmp = MemoryPool::allocate(size);
            assert(tmp != nullptr);
            ptrVec.push_back({tmp, size});
        }
    }

    for (auto &block : ptrVec)
    {
        assert(block.first != nullptr);
        MemoryPool::deallocate(block.first, block.second);
    }
    ptrVec.clear();
}

void workUnuseMemory(int loopNums)
{
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums * 7);
    for (int i = 0; i < loopNums; ++i)
    {
        for (size_t size : {8, 16, 32, 64, 128, 256, 512})
        // for (size_t size : {32})
        {
            void *tmp = operator new(size);
            assert(tmp != nullptr);
            ptrVec.push_back({tmp, size});
        }
    }

    for (auto &block : ptrVec)
        operator delete(block.first, block.second);
    ptrVec.clear();
}



int main()
{

    warmUp();


    // 测试单线程
    int loopNums = 100000;
    std::chrono::system_clock::time_point start, end;
    double duration_1, duration_2;

    std::vector<double> nums;
    for (int i = 0; i < 10; ++i)
    {
        start = std::chrono::system_clock::now();
        workUseMemory(loopNums);
        end = std::chrono::system_clock::now();
        duration_1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        std::cout << "use memoryPool times: " << duration_1 << "ms" << std::endl;

        start = std::chrono::system_clock::now();
        workUnuseMemory(loopNums);
        end = std::chrono::system_clock::now();
        duration_2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        std::cout << "use new/delete times: " << duration_2 << "ms" << std::endl;

        double reduce = (duration_2 - duration_1);
        double ratio = (reduce / duration_2) * 100.0;
        nums.push_back(ratio);
        std::cout << std::endl;
    }
    double sum = std::accumulate(nums.begin(), nums.end(), 0.0);
    double avg = sum / 10;
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
            threads.push_back(std::thread(workUseMemory, loopNums));
        }
        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }
        end = std::chrono::system_clock::now();
        duration_1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        std::cout << "use memoryPool times: " << duration_1 << "ms" << std::endl;
        threads.clear();

        start = std::chrono::system_clock::now();
        for (int i = 0; i < threadNums; ++i)
        {
            threads.push_back(std::thread(workUnuseMemory, loopNums));
        }
        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }
        end = std::chrono::system_clock::now();
        duration_2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        std::cout << "use new/delete times: " << duration_2 << "ms" << std::endl;
        threads.clear();

        double reduce = duration_2 - duration_1;
        double ratio = (reduce / duration_2) * 100.0;
        nums.push_back(ratio);
        std::cout << std::endl;
    }
    sum = std::accumulate(nums.begin(), nums.end(), 0.0);
    avg = sum / 10;
    std::cout << "avg: " << avg << "% time reduce" << std::endl;
    nums.clear();

    return 0;
}
