#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "../include/prio_queue.h"

namespace examples::prio_queue {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "PrioQueue\n";

        using qc = std::pair<int, std::string>;
        small::prio_queue<qc> q;

        std::jthread t([](small::prio_queue<qc> &_q) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "push {1, \"A\"}" << std::endl;
            _q.push_back(small::EnumPriorities::kNormal, {1, "Normal"});
            _q.push_back(small::EnumPriorities::kLow, {2, "Low"});
            _q.push_back(small::EnumPriorities::kHigh, {3, "High"});

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "push {2, \"b\"}" << std::endl;
            _q.emplace_back(small::EnumPriorities::kNormal, 4, "Normal again");

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "signal exit force" << std::endl;
            _q.signal_exit_force();
        },
                       std::ref(q));

        qc e;
        // timeout
        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(1), &e);
        std::cout << "ret=" << static_cast<int>(ret) << "\n";

        for (; ret != small::EnumLock::kExit;) {
            e   = {};
            ret = q.wait_pop_front(&e);

            if (ret == small::EnumLock::kExit) {
                std::cout << "ret=" << static_cast<int>(ret) << "\n";
            } else {
                std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << std::endl;
            }
        }

        t.join();

        std::cout << "PrioQueue finished\n\n";

        return 0;
    }
} // namespace examples::prio_queue