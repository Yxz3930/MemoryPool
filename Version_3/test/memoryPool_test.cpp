// #include "MemoryPool.h"
// #include <iostream>
// #include <chrono>
// #include <thread>
// #include <mutex>
// #include <numeric>
// #include <algorithm>
// #include <iomanip>
// #include <vector>
// using namespace memory_pool;






// void warmUp()
// {
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(1000 * 7);
//     for (int i = 0; i < 1000; ++i)
//     {
//         // for (size_t size : {8, 16, 32, 64, 128, 256, 512})
//         for (size_t size : {8, 16, 32, 64})
//         // for (size_t size : {32})
//         {
//             void *tmp = MemoryPool::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.push_back({tmp, size});
//         }
//     }

//     for (auto &block : ptrVec)
//     {
//         assert(block.first != nullptr);
//         MemoryPool::deallocate(block.first, block.second);
//     }
//     ptrVec.clear();
// }

// void workUseMemory(int loopNums)
// {
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(loopNums * 7);
//     for (int i = 0; i < loopNums; ++i)
//     {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512})
//         // for (size_t size : {32})
//         {
//             void *tmp = MemoryPool::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.push_back({tmp, size});
//         }
//     }

//     for (auto &block : ptrVec)
//     {
//         assert(block.first != nullptr);
//         MemoryPool::deallocate(block.first, block.second);
//     }
//     ptrVec.clear();
// }

// void workUnuseMemory(int loopNums)
// {
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(loopNums * 7);
//     for (int i = 0; i < loopNums; ++i)
//     {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512})
//         // for (size_t size : {32})
//         {
//             void *tmp = operator new(size);
//             assert(tmp != nullptr);
//             ptrVec.push_back({tmp, size});
//         }
//     }

//     for (auto &block : ptrVec)
//         operator delete(block.first, block.second);
//     ptrVec.clear();
// }



// int main()
// {

//     warmUp();


//     // 测试单线程
//     int loopNums = 100000;
//     std::chrono::system_clock::time_point start, end;
//     double duration_1, duration_2;

//     std::vector<double> nums;
//     for (int i = 0; i < 10; ++i)
//     {
//         start = std::chrono::system_clock::now();
//         workUseMemory(loopNums);
//         end = std::chrono::system_clock::now();
//         duration_1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
//         std::cout << "use memoryPool times: " << duration_1 << "ms" << std::endl;

//         start = std::chrono::system_clock::now();
//         workUnuseMemory(loopNums);
//         end = std::chrono::system_clock::now();
//         duration_2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
//         std::cout << "use new/delete times: " << duration_2 << "ms" << std::endl;

//         double reduce = (duration_2 - duration_1);
//         double ratio = (reduce / duration_2) * 100.0;
//         nums.push_back(ratio);
//         std::cout << std::endl;
//     }
//     double sum = std::accumulate(nums.begin(), nums.end(), 0.0);
//     double avg = sum / 10;
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
//             threads.push_back(std::thread(workUseMemory, loopNums));
//         }
//         for (auto &t : threads)
//         {
//             if (t.joinable())
//                 t.join();
//         }
//         end = std::chrono::system_clock::now();
//         duration_1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
//         std::cout << "use memoryPool times: " << duration_1 << "ms" << std::endl;
//         threads.clear();

//         start = std::chrono::system_clock::now();
//         for (int i = 0; i < threadNums; ++i)
//         {
//             threads.push_back(std::thread(workUnuseMemory, loopNums));
//         }
//         for (auto &t : threads)
//         {
//             if (t.joinable())
//                 t.join();
//         }
//         end = std::chrono::system_clock::now();
//         duration_2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
//         std::cout << "use new/delete times: " << duration_2 << "ms" << std::endl;
//         threads.clear();

//         double reduce = duration_2 - duration_1;
//         double ratio = (reduce / duration_2) * 100.0;
//         nums.push_back(ratio);
//         std::cout << std::endl;
//     }
//     sum = std::accumulate(nums.begin(), nums.end(), 0.0);
//     avg = sum / 10;
//     std::cout << "avg: " << avg << "% time reduce" << std::endl;
//     nums.clear();

//     return 0;
// }






// #include "MemoryPool.h"
// #include <iostream>
// #include <chrono>
// #include <thread>
// #include <mutex>
// #include <numeric>
// #include <algorithm>
// #include <iomanip>
// #include <vector>
// #include <cassert>

// using namespace memory_pool;

// // 预热阶段
// void warmUp() {
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(1000 * 4);
//     for (int i = 0; i < 1000; ++i) {
//         for (size_t size : {8, 16, 32, 64}) {
//             void* tmp = MemoryPool::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.emplace_back(tmp, size);
//         }
//     }
//     for (auto& block : ptrVec) {
//         MemoryPool::deallocate(block.first, block.second);
//     }
// }

// // 使用 MemoryPool 分配/释放内存
// void workUseMemory(int loopNums) {
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(loopNums * 7);
//     for (int i = 0; i < loopNums; ++i) {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512}) {
//             void* tmp = MemoryPool::allocate(size);
//             assert(tmp != nullptr);
//             ptrVec.emplace_back(tmp, size);
//         }
//     }
//     for (auto& block : ptrVec) {
//         MemoryPool::deallocate(block.first, block.second);
//     }
// }

// // 使用 new/delete 进行分配/释放
// void workUnuseMemory(int loopNums) {
//     std::vector<std::pair<void*, size_t>> ptrVec;
//     ptrVec.reserve(loopNums * 7);
//     for (int i = 0; i < loopNums; ++i) {
//         for (size_t size : {8, 16, 32, 64, 128, 256, 512}) {
//             void* tmp = ::operator new(size);
//             assert(tmp != nullptr);
//             ptrVec.emplace_back(tmp, size);
//         }
//     }
//     for (auto& block : ptrVec) {
//         ::operator delete(block.first, block.second);
//     }
// }

// // 单线程性能测试
// void testSingleThread(int loopNums, int rounds) {
//     std::cout << "\n========== [ 单线程测试 ] ==========\n";

//     std::vector<double> reductionRates;
//     for (int i = 0; i < rounds; ++i) {
//         auto start = std::chrono::steady_clock::now();
//         workUseMemory(loopNums);
//         auto end = std::chrono::steady_clock::now();
//         double t1 = std::chrono::duration<double, std::milli>(end - start).count();

//         start = std::chrono::steady_clock::now();
//         workUnuseMemory(loopNums);
//         end = std::chrono::steady_clock::now();
//         double t2 = std::chrono::duration<double, std::milli>(end - start).count();

//         double reduction = (t2 - t1) / t2 * 100.0;
//         reductionRates.push_back(reduction);

//         std::cout << std::fixed << std::setprecision(2);
//         std::cout << "Round " << i + 1 << ": "
//                   << "MemoryPool = " << t1 << " ms, "
//                   << "new/delete = " << t2 << " ms, "
//                   << "reduction = " << reduction << " %\n";
//     }

//     double avg = std::accumulate(reductionRates.begin(), reductionRates.end(), 0.0) / rounds;
//     std::cout << "\n[单线程平均节省率] => " << std::setprecision(2) << avg << " %\n";
// }

// // 多线程性能测试
// void testMultiThread(int loopNums, int threadNums, int rounds) {
//     std::cout << "\n========== [ 多线程测试 ] ==========\n";

//     std::vector<double> reductionRates;

//     for (int i = 0; i < rounds; ++i) {
//         std::vector<std::thread> threads;

//         auto start = std::chrono::steady_clock::now();
//         for (int j = 0; j < threadNums; ++j) {
//             threads.emplace_back(workUseMemory, loopNums);
//         }
//         for (auto& t : threads) t.join();
//         auto end = std::chrono::steady_clock::now();
//         double t1 = std::chrono::duration<double, std::milli>(end - start).count();

//         threads.clear();

//         start = std::chrono::steady_clock::now();
//         for (int j = 0; j < threadNums; ++j) {
//             threads.emplace_back(workUnuseMemory, loopNums);
//         }
//         for (auto& t : threads) t.join();
//         end = std::chrono::steady_clock::now();
//         double t2 = std::chrono::duration<double, std::milli>(end - start).count();

//         double reduction = (t2 - t1) / t2 * 100.0;
//         reductionRates.push_back(reduction);

//         std::cout << std::fixed << std::setprecision(2);
//         std::cout << "Round " << i + 1 << ": "
//                   << "MemoryPool = " << t1 << " ms, "
//                   << "new/delete = " << t2 << " ms, "
//                   << "reduction = " << reduction << " %\n";
//     }

//     double avg = std::accumulate(reductionRates.begin(), reductionRates.end(), 0.0) / rounds;
//     std::cout << "\n[多线程平均节省率] => " << std::setprecision(2) << avg << " %\n";
// }

// int main() {
//     warmUp();  // 预热内存池

//     const int loopNums = 100000;
//     const int threadNums = 4;
//     const int testRounds = 10;

//     testSingleThread(loopNums, testRounds);
//     testMultiThread(loopNums, threadNums, testRounds);

//     return 0;
// }




#include "MemoryPool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <numeric>
#include <cassert>
#include <utility>

using namespace memory_pool;
using Clock = std::chrono::steady_clock;

constexpr int loopNums = 100000;
constexpr int repeatTimes = 10;
constexpr int threadCount = 4;
constexpr size_t allocSizes[] = {8, 16, 32, 64, 128, 256, 512};

// 使用内存池分配
void useMemoryPoolTask() {
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums * std::size(allocSizes));

    for (int i = 0; i < loopNums; ++i) {
        for (size_t size : allocSizes) {
            void* p = MemoryPool::allocate(size);
            assert(p != nullptr);
            ptrVec.emplace_back(p, size);
        }
    }

    for (auto& [p, s] : ptrVec)
        MemoryPool::deallocate(p, s);
}

// 使用 new/delete 分配
void useNewDeleteTask() {
    std::vector<std::pair<void*, size_t>> ptrVec;
    ptrVec.reserve(loopNums * std::size(allocSizes));

    for (int i = 0; i < loopNums; ++i) {
        for (size_t size : allocSizes) {
            void* p = ::operator new(size);
            assert(p != nullptr);
            ptrVec.emplace_back(p, size);
        }
    }

    for (auto& [p, s] : ptrVec)
        ::operator delete(p, s);
}

// 计时函数模板
template<typename Func>
int64_t measure(Func&& f) {
    auto start = Clock::now();
    f();
    auto end = Clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

// 平均计算函数
double computeAverage(const std::vector<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// 主程序
int main() {
    std::vector<double> speedupSingle;

    std::cout << "\n===== 单线程测试 =====\n";
    // 预热内存池
    useMemoryPoolTask();

    for (int i = 0; i < repeatTimes; ++i) {
        auto t_pool = measure(useMemoryPoolTask);
        auto t_new = measure(useNewDeleteTask);
        double ratio = (t_new - t_pool) * 100.0 / t_new;
        speedupSingle.push_back(ratio);
        std::cout << "Round " << i + 1 << ": pool = " << t_pool << "us, new/delete = " << t_new
                  << "us, speedup = " << ratio << "%\n";
    }

    std::cout << "Average speedup (single-thread): " << computeAverage(speedupSingle) << "%\n";

    std::vector<double> speedupMulti;
    std::cout << "\n===== 多线程测试 (" << threadCount << " threads) =====\n";

    for (int i = 0; i < repeatTimes; ++i) {
        auto t_pool = measure([&]() {
            std::vector<std::thread> threads;
            for (int j = 0; j < threadCount; ++j)
                threads.emplace_back(useMemoryPoolTask);
            for (auto& t : threads) t.join();
        });

        auto t_new = measure([&]() {
            std::vector<std::thread> threads;
            for (int j = 0; j < threadCount; ++j)
                threads.emplace_back(useNewDeleteTask);
            for (auto& t : threads) t.join();
        });

        double ratio = (t_new - t_pool) * 100.0 / t_new;
        speedupMulti.push_back(ratio);
        std::cout << "Round " << i + 1 << ": pool = " << t_pool << "us, new/delete = " << t_new
                  << "us, speedup = " << ratio << "%\n";
    }

    std::cout << "Average speedup (multi-thread): " << computeAverage(speedupMulti) << "%\n";
    return 0;
}

