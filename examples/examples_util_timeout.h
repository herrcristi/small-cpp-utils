#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "../include/util_timeout.h"

namespace examples::utiltimeout {
    //
    //  example 1 for timeout
    //
    inline int Example1()
    {
        std::cout << "Utils Timeout\n";

        auto time_start = small::time_now();

        auto timeoutID1 = small::set_timeout(std::chrono::milliseconds(1000), [&time_start]() {
            std::cout << "Timeout1 executed after " << small::time_diff_ms(time_start) << " ms"
                      << " at " << small::to_iso_string()(small::time_now()) << " ms\n";
        });
        auto timeoutID2 = small::set_timeout(std::chrono::milliseconds(1000), [&time_start]() {
            std::cout << "Timeout2 should not be executed even after " << small::time_diff_ms(time_start) << " ms\n";
        });

        std::cout << "Timeout1 created with timeoutID=" << timeoutID1
                  << " at " << small::to_iso_string()(small::time_now()) << "\n";
        std::cout << "Timeout2 created with timeoutID=" << timeoutID2
                  << " at " << small::to_iso_string()(small::time_now()) << "\n";

        // this will cancel the timeout2
        auto ret = small::clear_timeout(timeoutID2);
        std::cout << "Clear Timeout timeoutID=" << timeoutID2 << " returned " << ret
                  << " at " << small::to_iso_string()(small::time_now()) << "\n";

        std::cout << "Waiting 2 seconds for execution\n\n";
        small::sleep(2000);

        // timeout1 already executed
        ret = small::clear_timeout(timeoutID1);
        std::cout << "Clear Timeout timeoutID=" << timeoutID1 << " returned " << ret
                  << " at " << small::to_iso_string()(small::time_now()) << "\n";

        std::cout << "Utils Timeout finished\n\n";

        return 0;
    }

    //
    // example 2 for interval
    //
    inline int Example2()
    {
        std::cout << "Utils Interval\n";

        auto time_start = small::time_now();

        auto intervalID1 = small::set_interval(std::chrono::milliseconds(1000), [&time_start]() {
            std::cout << "Interval1 executed after " << small::time_diff_ms(time_start) << " ms"
                      << " at " << small::to_iso_string()(small::time_now()) << " ms\n";
        });

        std::cout << "Interval1 created with intervalID=" << intervalID1
                  << " at " << small::to_iso_string()(small::time_now()) << "\n";

        std::cout << "Waiting 2.6 seconds for multiple executions\n\n";
        small::sleep(2600);

        // interval1 already executed
        auto ret = small::clear_interval(intervalID1);
        std::cout << "Clear Interval intervalID=" << intervalID1 << " returned " << ret
                  << " at " << small::to_iso_string()(small::time_now()) << "\n";

        std::cout << "Waiting 1.6 seconds for no executions\n\n";
        small::sleep(1600);

        // make sure the engine is stopped
        small::timeout::wait_for(std::chrono::milliseconds(100));
        small::timeout::signal_exit_force();
        small::timeout::wait();

        std::cout << "Utils Interval finished\n\n";

        return 0;
    }
} // namespace examples::utiltimeout