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

        small::worker_thread<qc> workers(2 /*threads*/, [](auto &w /*this*/, auto &item, auto b /*extra param b*/) -> void {
        {
            std::unique_lock< small::worker_thread<qc>> mlock( w );

            std::cout << "thread " << std::this_thread::get_id()  << "processing " << item.first << " " << item.second << " b=" << b << "\n";
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) ); }, 5 /*param b*/);

        workers.push_back({1, "a"});
        // std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
        workers.push_back(std::make_pair(2, "b"));
        workers.emplace_back(3, "e");
        std::this_thread::sleep_for(std::chrono::seconds(3));

        workers.signal_exit();
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::cout << "Finished Worker Thread example 1\n\n";

        return 0;
    }

    //
    //  example 2
    //
    int Example2()
    {
        std::cout << "Worker Thread example 2\n";

        using qc = std::pair<int, std::string>;

        // another way
        // processing function for worker_thread
        struct WorkerThreadFunction
        {
            using qc = std::pair<int, std::string>;
            void operator()(small::worker_thread<qc> &w, qc &a)
            {
                {
                    std::unique_lock<small::worker_thread<qc>> mlock(w);

                    std::cout << "thread " << std::this_thread::get_id() << "processing " << a.first << " " << a.second << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        };

        small::worker_thread<qc> workers2(2, WorkerThreadFunction());
        workers2.push_back({4, "d"});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        std::cout << "Finished Worker Thread example 2\n\n";

        return 0;
    }
} // namespace examples::worker_thread