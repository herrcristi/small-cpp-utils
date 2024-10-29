#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "../include/event.h"

namespace examples::event {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "hello" << std::endl;

        // small::event e( small::EventType::kEvent_Manual );
        small::event e;
        {
            std::unique_lock<small::event> mlock(e);
            // do somthing
        }

        auto fn_t = [](auto i, small::event &e) {
            for (int t = 0; t < 5; ++t) {
                {
                    e.wait();

                    // std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

                    std::cout << "thread=" << i << ", iteration=" << t << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        };

        // create thread
        std::thread t[3];
        for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
            t[i] = std::move(std::thread(fn_t, i, std::ref(e)));
        }

        for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
            for (int j = 0; j < 5; ++j) {
                e.set_event();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // wait
        for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
            if (t[i].joinable())
                t[i].join();
        }

        std::cout << "finishing" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        return 0;
    }

} // namespace examples::event