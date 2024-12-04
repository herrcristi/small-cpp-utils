#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

#include "../include/jobs_engine.h"

namespace examples::jobs_engine {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Jobs Engine example 1\n";

        using qc = std::pair<int, std::string>;

        small::jobs_engine<qc> jobs({.threads_count = 0 /*dont start any threads*/});

        jobs.add_type({}, [](auto &w /*this*/, const auto &items, auto b /*extra param b*/) {
            // process item using the jobs lock (not recommended)
            {
                std::unique_lock mlock( w );
                for(auto &[i, s]:items){
                    std::cout << "thread " << std::this_thread::get_id()  
                              << " processing {" << i << ", \"" << s << "\"} and b=" << b << "\n";
                }
            } 
            small::sleep(300); }, 5 /*param b*/);

        jobs.start_threads(2); // manual start threads

        // push
        jobs.push_back({1, "a"});
        small::sleep(300);

        jobs.push_back(std::make_pair(2, "b"));
        jobs.emplace_back(3, "e");
        jobs.emplace_back(4, "f");
        jobs.emplace_back(5, "g");

        auto ret = jobs.wait_for(std::chrono::milliseconds(0)); // wait to finished
        std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << " as timeout\n";
        jobs.wait(); // wait here for jobs to finish due to exit flag

        std::cout << "Jobs example 1 finish\n\n";

        return 0;
    }

} // namespace examples::jobs_engine