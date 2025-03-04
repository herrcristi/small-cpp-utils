#pragma once

#include "examples_common.h"

#include "../include/lock_queue.h"

namespace examples::lock_queue {
    //
    //  example 1
    //
    inline int Example1()
    {
        std::cout << "LockQueue\n";

        using qc = std::pair<int, std::string>;
        small::lock_queue<qc> q;

        std::thread t([](small::lock_queue<qc>& _q) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "push {1, \"A\"}" << std::endl;
            _q.push_back({1, "A"});

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "push {2, \"b\"}" << std::endl;
            _q.emplace_back(2, "b");

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "signal exit force" << std::endl;
            _q.signal_exit_force();
        },
                      std::ref(q));

        qc e;
        // timeout
        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(1), &e);
        std::cout << "ret=" << static_cast<int>(ret) << "\n";

        ret = q.wait_pop_front(&e);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << std::endl;

        e   = {};
        ret = q.wait_pop_front(&e);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << std::endl;

        // force exit signaled
        e   = {};
        ret = q.wait_pop_front(&e);
        std::cout << "ret=" << static_cast<int>(ret) << "\n";

        t.join();

        std::cout << "LockQueue finished\n\n";

        return 0;
    }
} // namespace examples::lock_queue