#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "../include/util_timeout.h"

namespace examples::utiltimeout {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Utils Timeout/Interval\n";

        auto time_start = small::timeNow();

        auto timeoutID1 = small::set_timeout(std::chrono::milliseconds(1000), [&time_start]() {
            std::cout << "Timeout1 executed after " << small::timeDiffMs(time_start) << " ms\n";
        });
        auto timeoutID2 = small::set_timeout(std::chrono::milliseconds(1000), [&time_start]() {
            std::cout << "Timeout2 should not be executed even after " << small::timeDiffMs(time_start) << " ms\n";
        });

        std::cout << "Timeout1 created with timeoutID=" << timeoutID1 << "\n";
        std::cout << "Timeout2 created with timeoutID=" << timeoutID2 << "\n";

        // this will cancel the timeout2
        auto ret = small::clear_timeout(timeoutID2);
        std::cout << "Clear Timeout timeoutID=" << timeoutID2 << " returned " << ret << "\n";

        small::sleep(2000);

        // timeout1 already executed
        ret = small::clear_timeout(timeoutID1);
        std::cout << "Clear Timeout timeoutID=" << timeoutID1 << " returned " << ret << "\n";

        std::cout << "Utils Timeout/Interval finished\n\n";

        return 0;
    }
} // namespace examples::utiltimeout