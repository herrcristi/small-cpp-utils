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
        using JobsEng = small::jobs_engine<JobsType, Request, int /*response*/, JobsGroupType>;

        auto jobs_processing_function = [](const std::vector<JobsEng::JobsItem *> &items) {
            // this functions is defined without the engine params (it is here just for the example)
            std::cout << "this function is defined without the engine params\n";
        };

        JobsEng::JobsConfig config{
            .m_engine                      = {.m_threads_count = 0 /*dont start any thread yet*/,
                                              .m_config_prio   = {.priorities = {{small::EnumPriorities::kHighest, 2},
                                                                                 {small::EnumPriorities::kHigh, 2},
                                                                                 {small::EnumPriorities::kNormal, 2},
                                                                                 {small::EnumPriorities::kLow, 1}}}}, // overall config with default priorities
            .m_default_processing_function = jobs_processing_function,                                              // default processing function, better use jobs.add_default_processing_function to set it
            .m_groups                      = {{JobsGroupType::kJobsGroup1, {.m_threads_count = 1}}},                // config by jobs group
            .m_types                       = {
                {JobsType::kJobsType1, {.m_group = JobsGroupType::kJobsGroup1}},
                {JobsType::kJobsType2, {.m_group = JobsGroupType::kJobsGroup1}},
            }};

        // create jobs engine
        JobsEng jobs(config);

        jobs.add_default_processing_function([](auto &j /*this jobs engine*/, const auto &jobs_items) {
            for (auto &item : jobs_items) {
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

        // add specific function for job1 (calling the function from jobs intead of config allows to pass the engine and extra param)
        jobs.add_job_processing_function(JobsType::kJobsType1, [](auto &j /*this jobs engine*/, const auto &jobs_items, auto b /*extra param b*/) {
            for (auto &item : jobs_items) {
                std::cout << "thread " << std::this_thread::get_id()
                          << " JOB1 processing "
                          << "{"
                          << " type=" << (int)item->type
                          << " req.int=" << item->request.first << ","
                          << " req.str=\"" << item->request.second << "\""
                          << "}"
                          << " time " << small::toISOString(small::timeNow())
                          << "\n";
            }
            small::sleep(30); }, 5 /*param b*/);

        JobsEng::JobsID              jobs_id{};
        std::vector<JobsEng::JobsID> jobs_ids;

        // push
        jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, {1, "normal"}, &jobs_id);
        jobs.queue().push_back(small::EnumPriorities::kHigh, JobsType::kJobsType2, {2, "high"}, &jobs_id);

        jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, std::make_pair(3, "normal"), &jobs_id);
        jobs.queue().push_back(small::EnumPriorities::kHigh, {.type = JobsType::kJobsType1, .request = {4, "high"}}, &jobs_id);
        jobs.queue().push_back(small::EnumPriorities::kLow, JobsType::kJobsType1, {5, "low"}, &jobs_id);

        Request req = {6, "normal"};
        jobs.queue().push_back(small::EnumPriorities::kNormal, {.type = JobsType::kJobsType1, .request = req}, nullptr);

        std::vector<JobsEng::JobsItem> jobs_items = {{.type = JobsType::kJobsType1, .request = {7, "highest"}}};
        jobs.queue().push_back(small::EnumPriorities::kHighest, jobs_items, &jobs_ids);
        jobs.queue().push_back(small::EnumPriorities::kHighest, {{.type = JobsType::kJobsType1, .request = {8, "highest"}}}, &jobs_ids);

        jobs.queue().push_back_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {100, "delay normal"}, &jobs_id);
        jobs.queue().push_back_delay_until(small::timeNow() + std::chrono::milliseconds(350), small::EnumPriorities::kNormal, JobsType::kJobsType1, {101, "delay normal"}, &jobs_id);
        jobs.queue().push_back_delay_for(std::chrono::milliseconds(400), small::EnumPriorities::kNormal, JobsType::kJobsType1, {102, "delay normal"}, &jobs_id);

        jobs.start_threads(3); // manual start threads

        small::sleep(50);
        // jobs.signal_exit_force();
        auto ret = jobs.wait_for(std::chrono::milliseconds(100)); // wait to finished
        std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << " as timeout\n";
        jobs.wait(); // wait here for jobs to finish due to exit flag

        std::cout << "size = " << jobs.size() << "\n";

        std::cout << "Jobs Engine example 1 finish\n\n";

        return 0;
    }

} // namespace examples::jobs_engine