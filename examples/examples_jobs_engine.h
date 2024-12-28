#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

#include "../include/jobs_engine.h"

namespace examples::jobs_engine {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Jobs Engine example 1\n";

        enum class JobsType
        {
            kJobsType1,
            kJobsType2
        };
        enum class JobsGroupType
        {
            kJobsGroup1
        };

        using Request = std::pair<int, std::string>;
        using JobsEng = small::jobs_engine<JobsType, Request, int, JobsGroupType>;

        JobsEng jobs(
            {.threads_count = 0 /*dont start any thread yet*/}, // overall config with default priorities
            {.threads_count = 1, .bulk_count = 1},              // default jobs group config
            {.group = JobsGroupType::kJobsGroup1},              // default jobs type config
            [](auto &j /*this*/, const auto &items) {
                for (auto &item : items) {
                    std::cout << "thread " << std::this_thread::get_id()
                              << " DEFAULT processing "
                              << "{"
                              << " type=" << (int)item->type
                              << " req.int=" << item->request.first << ","
                              << " req.str=\"" << item->request.second << "\""
                              << "}"
                              << " time " << small::toISOString(small::timeNow())
                              << "\n";
                }
                small::sleep(30);
            });

        jobs.add_jobs_group(JobsGroupType::kJobsGroup1, {.threads_count = 1});

        // add specific function for job1
        jobs.add_jobs_type(JobsType::kJobsType1, {.group = JobsGroupType::kJobsGroup1}, [](auto &j /*this*/, const auto &items, auto b /*extra param b*/) {
            // process item using the jobs lock (not recommended)
            {
                std::unique_lock mlock( j );
                for (auto &item : items) {
                    std::cout << "thread " << std::this_thread::get_id()
                              << " DEFAULT processing "
                              << "{"
                              << " type=" << (int)item->type
                              << " req.int=" << item->request.first << ","
                              << " req.str=\"" << item->request.second << "\""
                              << "}"
                              << " time " << small::toISOString(small::timeNow())
                              << "\n";
                }
            } 
            small::sleep(30); }, 5 /*param b*/);

        // use default config and default function for job2
        jobs.add_jobs_type(JobsType::kJobsType2);

        jobs.start_threads(3); // manual start threads

        JobsEng::JobsID              jobs_id{};
        std::vector<JobsEng::JobsID> jobs_ids;

        // push
        jobs.push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, {1, "a"}, &jobs_id);
        jobs.push_back(small::EnumPriorities::kHigh, JobsType::kJobsType2, {2, "b"}, &jobs_id);

        jobs.push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, std::make_pair(3, "c"), &jobs_id);
        jobs.push_back(small::EnumPriorities::kHigh, {.type = JobsType::kJobsType1, .request = {4, "d"}}, &jobs_id);
        jobs.push_back(small::EnumPriorities::kLow, JobsType::kJobsType1, {5, "e"}, &jobs_id);

        Request req = {6, "f"};
        jobs.push_back(small::EnumPriorities::kNormal, {.type = JobsType::kJobsType1, .request = req}, nullptr);

        std::vector<JobsEng::JobsItem> jobs_items = {{.type = JobsType::kJobsType1, .request = {7, "g"}}};
        jobs.push_back(small::EnumPriorities::kHighest, jobs_items, &jobs_ids);

        jobs.push_back_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {100, "x"}, &jobs_id);
        jobs.push_back_delay_until(small::timeNow() + std::chrono::milliseconds(350), small::EnumPriorities::kNormal, JobsType::kJobsType1, {101, "y"}, &jobs_id);
        jobs.push_back_delay_for(std::chrono::milliseconds(400), small::EnumPriorities::kNormal, JobsType::kJobsType1, {102, "z"}, &jobs_id);

        small::sleep(50);
        // jobs.signal_exit_force();
        auto ret = jobs.wait_for(std::chrono::milliseconds(100)); // wait to finished
        std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << " as timeout\n";
        jobs.wait(); // wait here for jobs to finish due to exit flag

        std::cout << "Jobs Engine example 1 finish\n\n";

        return 0;
    }

} // namespace examples::jobs_engine