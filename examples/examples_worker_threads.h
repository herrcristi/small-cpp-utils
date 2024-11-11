#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

#include "../include/worker_thread.h"

namespace examples::worker_thread {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Worker Thread example 1\n";

        using qc = std::pair<int, std::string>;

        small::worker_thread<qc> workers(0 /*dont start any threads*/, [](auto &w /*this*/, auto &item, auto b /*extra param b*/) {
            // process item using the workers lock (not recommended)
            {
                std::unique_lock mlock( w );

                std::cout << "thread " << std::this_thread::get_id()  << " processing {" << item.first << ", \"" << item.second << "\"} and b=" << b << "\n";
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
        std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << "\n";
        workers.wait(); // wait here for workers to finish due to exit flag

        std::cout << "Worker Thread example 1 finish\n\n";

        return 0;
    }

    //
    //  example 2
    //
    int Example2()
    {
        std::cout << "Worker Thread example 2\n";

        using qc = std::pair<int, std::string>;

        // another way for processing function for worker_thread
        struct WorkerThreadFunction
        {
            using qc = std::pair<int, std::string>;
            void operator()(small::worker_thread<qc> &w, qc &a) const
            {
                {
                    std::unique_lock<small::worker_thread<qc>> mlock(w);

                    std::cout << "thread " << std::this_thread::get_id() << " processing {" << a.first << ", \"" << a.second << "\"}\n";
                }
                small::sleep(100);
            }
        };

        small::worker_thread<qc> workers(2 /*threads*/, WorkerThreadFunction());
        workers.push_back({4, "d"});
        small::sleep(300);

        workers.signal_exit_force(); // after signal exit no push will be accepted
        workers.push_back({5, "e"});

        std::cout << "Finished Worker Thread example 2\n\n";
        // workers will be joined on destructor

        return 0;
    }

    //
    //  perf example 3
    //
    int Example3_Perf()
    {
        std::cout << "Worker Thread example 3\n";

        for (int threads = 1; threads <= 4; ++threads) {
            auto timeStart = small::timeNow();

            // create worker
            small::worker_thread<int> workers(threads /*threads*/, [](auto &w /*this*/, int &elem) {
                // nothing to do
                elem++;
            });

            // add 1 million entries for worker
            const int elements = 1'000'000;
            for (int i = 0; i < elements; ++i) {
                workers.push_back(i);
            }

            // wait for processing
            workers.wait();

            // time elapsed
            auto elapsed = small::timeDiffMs(timeStart);
            std::cout << "Processing with " << threads << " threads " << elements << " elements took " << elapsed << " ms"
                      << ", at a rate of " << double(elements) / double(std::max(elapsed, 1LL)) << " elements/ms\n";
        }
        std::cout << "Locking has an important part!\n";
        std::cout << "Finished Worker Thread example 3\n\n";
        // workers will be joined on destructor

        return 0;
    }
} // namespace examples::worker_thread