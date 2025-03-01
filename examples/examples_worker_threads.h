#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

#include "../include/worker_thread.h"

namespace examples::worker_thread {
    //
    //  example 1
    //
    inline int Example1()
    {
        std::cout << "Worker Thread example 1\n";

        using qc = std::pair<int, std::string>;

        small::worker_thread<qc> workers({.threads_count = 0 /*dont start any threads*/}, [](auto& w /*this*/, const auto& items, auto b /*extra param b*/) {
            // process item using the workers lock (not recommended)
            {
                std::unique_lock mlock( w );
                for(auto &[i, s]:items){
                    std::cout << "thread " << std::this_thread::get_id()  
                              << " processing {" << i << ", \"" << s << "\"} and b=" << b << "\n";
                }
            } 
            small::sleep(300); }, 5 /*param b*/);

        workers.start_threads(2); // manual start threads

        // push
        workers.push_back({1, "a"});
        small::sleep(300);

        workers.push_back(std::make_pair(2, "b"));
        workers.emplace_back(3, "e");
        workers.emplace_back(4, "f");
        workers.emplace_back(5, "g");

        auto ret = workers.wait_for(std::chrono::milliseconds(0)); // wait to finished
        std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << " as timeout\n";
        workers.wait(); // wait here for workers to finish due to exit flag

        std::cout << "Worker Thread example 1 finish\n\n";

        return 0;
    }

    //
    //  example 2
    //
    inline int Example2()
    {
        std::cout << "Worker Thread example 2\n";

        using qc = std::pair<int, std::string>;

        // another way for processing function for worker_thread
        struct WorkerThreadFunction
        {
            using qc = std::pair<int, std::string>;
            void operator()(small::worker_thread<qc>& w, const std::vector<qc>& items) const
            {
                {
                    std::unique_lock mlock(w);

                    for (auto& [i, s] : items) {
                        std::cout << "thread " << std::this_thread::get_id()
                                  << " processing {" << i << ", \"" << s << "\"}";

                        std::cout << " time " << small::to_iso_string(small::time_now()) << "\n";
                    }
                }
                small::sleep(100);
            }
        };

        small::worker_thread<qc> workers({.threads_count = 2}, WorkerThreadFunction());
        workers.push_back({4, "d"});
        workers.push_back_delay_for(std::chrono::milliseconds(300), {5, "e"});
        // will wait for the delayed items to be consumed and then the active ones
        workers.wait();

        workers.push_back({6, "f"});

        std::cout << "Finished Worker Thread example 2\n\n";
        // workers will be joined on destructor

        return 0;
    }

    //
    //  perf example 3
    //
    inline int Example3_Perf()
    {
        std::cout << "Worker Thread example 3\n";

        const auto bulks = {1, 2, 5, 10};

        for (auto bulk_count : bulks) {

            for (int threads = 1; threads <= 4; ++threads) {
                auto timeStart = small::time_now();

                // create worker
                small::worker_thread<int> workers({.threads_count = threads, .bulk_count = bulk_count}, [](auto& w /*this*/, const std::vector<int>& elems) {
                    // simulate some work
                    int sum = 0;
                    for (auto& elem : elems) {
                        sum += elem;
                    }
                    std::ignore = sum;
                });

                // add many entries for worker
                const int elements = 100'000;
                for (int i = 0; i < elements; ++i) {
                    workers.push_back(i);
                }

                // wait for processing
                workers.wait();

                // time elapsed
                auto elapsed = small::time_diff_ms(timeStart);
                std::cout << "Processing with " << threads << " threads " << elements << " elements and bulk " << bulk_count
                          << " took " << elapsed << " ms"
                          << ", at a rate of " << double(elements) / double(std::max(elapsed, 1LL)) << " elements/ms\n";
            }
            std::cout << "\n";
        }

        // Processing with 1 threads 100000 elements and bulk 1 took 8101 ms, at a rate of 12.3442 elements/ms
        // Processing with 2 threads 100000 elements and bulk 1 took 4162 ms, at a rate of 24.0269 elements/ms
        // Processing with 3 threads 100000 elements and bulk 1 took 2866 ms, at a rate of 34.8918 elements/ms
        // Processing with 4 threads 100000 elements and bulk 1 took 2259 ms, at a rate of 44.2674 elements/ms

        // Processing with 1 threads 100000 elements and bulk 2 took 4072 ms, at a rate of 24.558 elements/ms
        // Processing with 2 threads 100000 elements and bulk 2 took 2166 ms, at a rate of 46.1681 elements/ms
        // Processing with 3 threads 100000 elements and bulk 2 took 1417 ms, at a rate of 70.5716 elements/ms
        // Processing with 4 threads 100000 elements and bulk 2 took 1065 ms, at a rate of 93.8967 elements/ms

        // Processing with 1 threads 100000 elements and bulk 5 took 1718 ms, at a rate of 58.2072 elements/ms
        // Processing with 2 threads 100000 elements and bulk 5 took 849 ms, at a rate of 117.786 elements/ms
        // Processing with 3 threads 100000 elements and bulk 5 took 548 ms, at a rate of 182.482 elements/ms
        // Processing with 4 threads 100000 elements and bulk 5 took 470 ms, at a rate of 212.766 elements/ms

        // Processing with 1 threads 100000 elements and bulk 10 took 815 ms, at a rate of 122.699 elements/ms
        // Processing with 2 threads 100000 elements and bulk 10 took 418 ms, at a rate of 239.234 elements/ms
        // Processing with 3 threads 100000 elements and bulk 10 took 296 ms, at a rate of 337.838 elements/ms
        // Processing with 4 threads 100000 elements and bulk 10 took 214 ms, at a rate of 467.29 elements/ms

        std::cout << "Finished Worker Thread example 3\n\n";
        // workers will be joined on destructor

        return 0;
    }
} // namespace examples::worker_thread