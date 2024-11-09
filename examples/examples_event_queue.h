#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "../include/event_queue.h"

namespace examples::event_queue {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "EventQueue\n";

        using qc = std::pair<int, std::string>;
        small::event_queue<qc> q;

        std::thread t([](small::event_queue<qc> &_q) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "push {1, \"B\"}" << std::endl;
            _q.push_back({1, "B"});

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "push {2, \"a\"}" << std::endl;
            _q.emplace_back(2, "a");

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "signal exit" << std::endl;
            _q.signal_exit();
        },
                      std::ref(q));

        qc e;
        // timeout
        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(1), &e);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << std::endl;

        ret = q.wait_pop_front(&e);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << std::endl;

        e = {};
        ret = q.wait_pop_front(&e);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << std::endl;

        // exit signaled
        e = {};
        ret = q.wait_pop_front(&e);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << std::endl;

        t.join();

        std::cout << "Event Queue finished\n\n";

        return 0;
    }
} // namespace examples::event_queue