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

        enum JobType
        {
            job1,
            job2
        };

        small::jobs_engine<JobType, qc> jobs(
            {.threads_count = 0 /*dont start any thread yet*/},
            {.threads_count = 1, .bulk_count = 1},
            [](auto &j /*this*/, const auto &items) {
                for (auto &[i, s] : items) {
                    std::cout << "thread " << std::this_thread::get_id()
                              << " default processing {" << i << ", \"" << s << "\"}\n";
                }
                small::sleep(300);
            });

        // add specific function for job1
        jobs.add_job_type(JobType::job1, {.threads_count = 2}, [](auto &j /*this*/, const auto &items, auto b /*extra param b*/) {
            // process item using the jobs lock (not recommended)
            {
                std::unique_lock mlock( j );
                for(auto &[i, s]:items){
                    std::cout << "thread " << std::this_thread::get_id()  
                              << " specific processing {" << i << ", \"" << s << "\"} and b=" << b << "\n";
                }
            } 
            small::sleep(300); }, 5 /*param b*/);

        // use default config and deafult function for job2
        jobs.add_job_type(JobType::job2);

        jobs.start_threads(2); // manual start threads

        // // push
        // jobs.push_back({1, "a"});
        // small::sleep(300);

        // jobs.push_back(std::make_pair(2, "b"));
        // jobs.emplace_back(3, "e");
        // jobs.emplace_back(4, "f");
        // jobs.emplace_back(5, "g");

        // auto ret = jobs.wait_for(std::chrono::milliseconds(0)); // wait to finished
        // std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << " as timeout\n";
        // jobs.wait(); // wait here for jobs to finish due to exit flag

        std::cout << "Jobs example 1 finish\n\n";

        return 0;
    }

} // namespace examples::jobs_engine