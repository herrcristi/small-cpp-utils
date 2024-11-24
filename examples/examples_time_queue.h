#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "../include/time_queue.h"

namespace examples::time_queue {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "TimeQueue\n";

        using qc = std::pair<int, std::string>;
        small::time_queue<qc> q;

        auto timeStart = small::timeNow();

        std::thread t([](small::time_queue<qc> &_q) {
            std::cout << "push {1, \"A\"}" << std::endl;
            _q.push_delay_for(std::chrono::milliseconds(600), {1, "A"});

            std::cout << "push {2, \"b\"}" << std::endl;
            _q.emplace_delay_for(std::chrono::milliseconds(300), 2, "b");

            auto time_now = std::chrono::system_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::cout << "push {3, \"c\"}" << std::endl;
            _q.push_delay_until(time_now /*already expired*/, {3, "c"});

            std::this_thread::sleep_for(std::chrono::milliseconds(900));
            std::cout << "signal exit force" << std::endl;
            _q.signal_exit_force();
        },
                      std::ref(q));

        qc e;
        // timeout
        auto ret = q.wait_pop_for(std::chrono::milliseconds(1), &e);
        std::cout << "ret=" << static_cast<int>(ret) << " as timeout\n";

        ret = q.wait_pop(&e);
        auto elapsed = small::timeDiffMs(timeStart);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << ", elapsed " << elapsed << " ms\n";

        e = {};
        ret = q.wait_pop(&e);
        elapsed = small::timeDiffMs(timeStart);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << ", elapsed " << elapsed << " ms\n";

        e = {};
        ret = q.wait_pop(&e);
        elapsed = small::timeDiffMs(timeStart);
        std::cout << "ret=" << static_cast<int>(ret) << ", pop " << e.first << "," << e.second << ", elapsed " << elapsed << " ms\n";

        // force exit signaled
        e = {};
        ret = q.wait_pop(&e);
        elapsed = small::timeDiffMs(timeStart);
        std::cout << "ret=" << static_cast<int>(ret) << " as signal exit" << ", elapsed " << elapsed << " ms\n";

        t.join();

        std::cout << "TimeQueue finished\n\n";

        return 0;
    }
} // namespace examples::time_queue