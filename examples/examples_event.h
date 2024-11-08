#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "../include/event.h"
#include "../include/util.h"

namespace examples::event {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Event\n";

        // small::event e( small::EventType::kEvent_Manual );
        small::event e;
        {
            std::unique_lock<small::event> mlock(e);
            // do something
            std::cout << "Event is used as a mutex\n";
        }

        auto fn_t = [](auto i, auto iterations, small::event &_e) {
            for (int t = 0; t < iterations; ++t) {
                {
                    _e.wait();
                    std::cout << "thread=" << i << ", iteration=" << t << std::endl;
                }
                small::sleep(1 /*ms*/);
            }
        };

        // create thread
        const int iterations = 3;
        std::thread t[3];
        for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
            t[i] = std::thread(fn_t, i, iterations, std::ref(e));
        }

        for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
            for (int j = 0; j < iterations; ++j) {
                e.set_event();
                small::sleep(100 /*ms*/);
            }
        }

        // wait
        for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
            if (t[i].joinable()) {
                t[i].join();
            }
        }

        std::cout << "Event finished\n\n";

        return 0;
    }

} // namespace examples::event