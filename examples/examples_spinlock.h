#pragma once

#include <condition_variable>
#include <cstdio>
#include <iostream>

#include "../include/spinlock.h"

namespace examples::spinlock {
    //
    // example 1
    //
    inline int Example1()
    {
        std::cout << "Spin lock\n";

        // thread function
        auto fn_t = [](auto i, small::spinlock& lock) {
            for (int t = 0; t < 5; ++t) {
                {
                    std::unique_lock<small::spinlock> mlock(lock);
                    std::cout << "spin lock thread=" << i << ", iteration=" << t << "\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        };

        // spinlock
        small::spinlock lock;
        // create threads
        std::array<std::thread, 3> threads;
        for (size_t i = 0; i < threads.size(); ++i) {
            threads[i] = std::thread(fn_t, i, std::ref(lock));
        }

        // wait
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        std::cout << "Spin lock finished\n\n";

        return 0;
    }
} // namespace examples::spinlock