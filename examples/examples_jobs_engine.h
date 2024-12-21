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
        std::cout << "Jobs Queue example 1\n";

        enum JobType
        {
            kJob1
        };
        enum JobGroupType
        {
            kJobGroup1
        };

        small::jobs_queue<JobType, int, JobGroupType> q;
        q.set_job_type_group(JobType::kJob1, JobGroupType::kJobGroup1);

        q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, 1);

        // on some thread
        std::pair<JobType, int> e{};
        auto                    ret = q.wait_pop_front(JobGroupType ::kJobGroup1, &e);
        // or wait_pop_front_for( std::chrono::minutes( 1 ), JobGroupType:kJobGroup1, &e );
        // ret can be small::EnumLock::kExit, small::EnumLock::kTimeout or ret == small::EnumLock::kElement
        if (ret == small::EnumLock::kElement) {
            // do something with e
            std::cout << "elem from jobs q " << e.first << ", " << e.second << "\n";
        }

        // on main thread no more processing (aborting work)
        q.signal_exit_force(); // q.signal_exit_when_done()

        std::cout << "Jobs Queue example 1 finish\n\n";

        return 0;
    }

    //
    //  example 2
    //
    int Example2()
    {
        std::cout << "Jobs Engine example 2\n";

        using qc = std::pair<int, std::string>;

        enum JobType
        {
            job1,
            job2
        };

        small::jobs_engine<JobType, qc> jobs(
            {.threads_count = 0 /*dont start any thread yet*/},
            {.threads_count = 1, .bulk_count = 1},
            {.group = JobType::job1},
            [](auto &j /*this*/, const auto job_type, const auto &items) {
                for (auto &[i, s] : items) {
                    std::cout << "thread " << std::this_thread::get_id()
                              << " default processing type " << job_type << " {" << i << ", \"" << s << "\"}"
                              << " time " << small::toISOString(small::timeNow())
                              << "\n";
                }
                small::sleep(30);
            });

        // add specific function for job1
        jobs.add_job_type(JobType::job1, {.threads_count = 2}, [](auto &j /*this*/, const auto job_type, const auto &items, auto b /*extra param b*/) {
            // process item using the jobs lock (not recommended)
            {
                std::unique_lock mlock( j );
                for(auto &[i, s]:items){
                    std::cout << "thread " << std::this_thread::get_id()  
                              << " specific processing type " << job_type << " {" << i << ", \"" << s << "\"} and b=" << b 
                              << " time " << small::toISOString(small::timeNow())
                              << "\n";
                }
            } 
            small::sleep(30); }, 5 /*param b*/);

        // use default config and default function for job2
        jobs.add_job_type(JobType::job2);

        jobs.start_threads(3); // manual start threads

        // push
        jobs.push_back(small::EnumPriorities::kNormal, JobType::job1, {1, "a"});
        jobs.push_back(small::EnumPriorities::kHigh, JobType::job2, {2, "b"});

        jobs.push_back(small::EnumPriorities::kNormal, JobType::job1, std::make_pair(3, "c"));
        jobs.emplace_back(small::EnumPriorities::kNormal, JobType::job1, 4, "d");
        jobs.emplace_back(small::EnumPriorities::kNormal, JobType::job1, 5, "e");
        jobs.emplace_back(small::EnumPriorities::kNormal, JobType::job1, 6, "f");
        jobs.emplace_back_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobType::job1, 7, "g");
        jobs.emplace_back_delay_until(small::timeNow() + std::chrono::milliseconds(350), small::EnumPriorities::kNormal, JobType::job1, 8, "h");
        jobs.push_back_delay_for(std::chrono::milliseconds(400), small::EnumPriorities::kNormal, JobType::job1, {9, "i"});

        small::sleep(50);
        // jobs.signal_exit_force();
        auto ret = jobs.wait_for(std::chrono::milliseconds(100)); // wait to finished
        std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << " as timeout\n";
        jobs.wait(); // wait here for jobs to finish due to exit flag

        std::cout << "Jobs Engine example 2 finish\n\n";

        return 0;
    }

} // namespace examples::jobs_engine