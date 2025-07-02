// ring_buffer_vyukov_test.cpp
// --------------------------------------------------
// Correctness + throughput benchmark for LockFreeRingBufferVyukov **with
// reliable shutdown**.  因为 Vyukov 的 dequeue 是阻塞式，消费者如果提前
// 发现生产完毕会卡在空队列上。解决办法：每个生产者结束后投递一个
// “终止哨兵” (SENTINEL)。消费者遇到哨兵就退出，因此不会死锁。
//
// Build (GCC / Clang):
//   g++ -std=c++20 -O2 -pthread ring_buffer_vyukov_test.cpp -o rb_test
//   ./rb_test
//
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
#include <vector>

#include "lock_free.h"

int main() {
    std::cout << "Start test\n";

    using Buffer = LockFreeRingBufferVyukov<int, 1024>; // 1 Ki slot queue
    Buffer q;

    constexpr std::size_t PRODUCERS          = 4;
    constexpr std::size_t CONSUMERS          = 4;
    constexpr std::size_t ITEMS_PER_PRODUCER = 100'00000; // 调参改变工作量
    constexpr std::size_t TOTAL_ITEMS        = PRODUCERS * ITEMS_PER_PRODUCER;
    constexpr int         SENTINEL           = std::numeric_limits<int>::min();

    std::atomic<std::size_t> produced{0};      // real items only
    std::atomic<std::size_t> consumed{0};
    std::atomic<long long>   sumProduced{0};
    std::atomic<long long>   sumConsumed{0};
    std::atomic<std::size_t> sentinelsSeen{0};

    // ------------------ producer ------------------
    auto producer = [&](int id) {
        for (std::size_t i = 0; i < ITEMS_PER_PRODUCER; ++i) {
            int value = static_cast<int>(id * ITEMS_PER_PRODUCER + i);
            sumProduced.fetch_add(value, std::memory_order_relaxed);
            q.enqueue(value);
            produced.fetch_add(1, std::memory_order_release);
        }
        // push termination token
        q.enqueue(SENTINEL);
    };

    // ------------------ consumer ------------------
    auto consumer = [&]() {
        int value;
        while (true) {
            q.dequeue(value); // blocking dequeue
            if (value == SENTINEL) {
                break;
            }
            sumConsumed.fetch_add(value, std::memory_order_relaxed);
            consumed.fetch_add(1, std::memory_order_release);
        }
    };

    // ------------------ progress monitor ------------------
    auto monitor = [&]() {
        using namespace std::chrono_literals;
        std::size_t lastLen = 0;
        while (consumed.load(std::memory_order_acquire) < TOTAL_ITEMS) {
            std::size_t doneP = produced.load(std::memory_order_relaxed);
            std::size_t doneC = consumed.load(std::memory_order_relaxed);
            auto pct = [](std::size_t x, std::size_t total) {
                return static_cast<int>(x * 100.0 / total);
            };

            std::ostringstream oss;
            oss << "Produced: " << std::setw(8) << doneP << " (" << std::setw(3) << pct(doneP, TOTAL_ITEMS)
                << "%)  |  Consumed: " << std::setw(8) << doneC << " (" << std::setw(3) << pct(doneC, TOTAL_ITEMS) << "%)";
            std::string line = oss.str();
            if (line.size() < lastLen) line.append(lastLen - line.size(), ' ');
            lastLen = line.size();
            std::cout << '\r' << line << std::flush;
            std::this_thread::sleep_for(200ms);
        }
        std::cout << '\r' << std::string(lastLen, ' ') << '\r';
        std::cout << "Produced: " << TOTAL_ITEMS << " (100%)  |  Consumed: " << TOTAL_ITEMS << " (100%)\n";
    };

    // ------------------ run test ------------------
    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(PRODUCERS + CONSUMERS);
    for (std::size_t i = 0; i < PRODUCERS; ++i) workers.emplace_back(producer, static_cast<int>(i));
    for (std::size_t i = 0; i < CONSUMERS; ++i) workers.emplace_back(consumer);

    // std::thread monitorThread(monitor);

    for (auto &th : workers) th.join();
    // monitorThread.join();

    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = t1 - t0;

    // ------------------ report ------------------
    bool pass = (consumed.load() == TOTAL_ITEMS) && (sumProduced.load() == sumConsumed.load());

    std::cout << "\nCorrectness test: " << (pass ? "PASS" : "FAIL") << '\n';
    std::cout << "  produced items : " << TOTAL_ITEMS      << '\n';
    std::cout << "  consumed items : " << consumed.load()   << '\n';
    std::cout << "  sum produced   : " << sumProduced.load() << '\n';
    std::cout << "  sum consumed   : " << sumConsumed.load() << "\n\n";

    std::cout << "Performance:\n";
    std::cout << "  elapsed time   : " << elapsed.count() * 1000 << " ms\n";
    std::cout << "  throughput     : " << (TOTAL_ITEMS / elapsed.count()) / 1.0e6 << " M ops/s\n";

    return pass ? 0 : 1;
}
