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

        //
        // create a complex example for each type 1,2,3 (like web requests)
        // - priorities
        // - dependencies calls (parent-child relationship)
        //      (with children for calls that take long time like database access or less time like cache server)
        // - coalesce calls (multiple (later) calls resolved when first one is called)
        // - timeout
        // - throttle (call with some wait-sleep interval in between)
        //
        // type1 will create 2 children and will be finished when at least 1 is finished
        //      (this will demonstrate OR and custom children function)
        // type2 will create 2 children and will be finished whenn ALL children are finished
        //      (this will demonstrate the default case AND and default children function)
        // type3 will have no children and one call will timeout
        //
        enum class JobsType
        {
            kJobsType1,
            kJobsType2,
            kJobsType3,
            kJobsDatabase,
            kJobsCache,
        };

        enum class JobsGroupType
        {
            kJobsGroup12,
            kJobsGroup3,
            kJobsGroupDatabase,
            kJobsGroupCache,
        };

        using Request = std::pair<int, std::string>;
        using JobsEng = small::jobs_engine<JobsType, Request, int /*response*/, JobsGroupType>;

        auto jobs_processing_function = [](const std::vector<std::shared_ptr<JobsEng::JobsItem>> &items) {
            // this functions is defined without the engine params (it is here just for the example)
            std::cout << "this function is defined without the engine params, called for " << (int)items[0]->type << "\n";
        };

        //
        // config object for priorities, groups, types, threads, timeouts, sleeps, etc
        //
        JobsEng::JobsConfig config{
            .m_engine = {.m_threads_count = 0 /*dont start any thread yet*/, // TODO add thread count for wait_for_children processing and finished, default 1/2 here use 1 due to coalesce
                         .m_config_prio   = {.priorities = {{small::EnumPriorities::kHighest, 2},
                                                            {small::EnumPriorities::kHigh, 2},
                                                            {small::EnumPriorities::kNormal, 2},
                                                            {small::EnumPriorities::kLow, 1}}}}, // overall config with default priorities

            .m_default_processing_function = jobs_processing_function, // default processing function, better use jobs.add_default_processing_function to set it

            .m_groups = {{JobsGroupType::kJobsGroup12, {.m_threads_count = 1}}, // config by jobs group // TODO add sleep_between_requests
                         {JobsGroupType::kJobsGroup3, {.m_threads_count = 1}},
                         {JobsGroupType::kJobsGroupDatabase, {.m_threads_count = 1}},
                         {JobsGroupType::kJobsGroupCache, {.m_threads_count = 1}}},

            .m_types = {{JobsType::kJobsType1, {.m_group = JobsGroupType::kJobsGroup12}},          //
                        {JobsType::kJobsType2, {.m_group = JobsGroupType::kJobsGroup12}},          //
                        {JobsType::kJobsType3, {.m_group = JobsGroupType::kJobsGroup3}},           // TODO add timeout for job
                        {JobsType::kJobsDatabase, {.m_group = JobsGroupType::kJobsGroupDatabase}}, // TODO add timeout for job
                        {JobsType::kJobsCache, {.m_group = JobsGroupType::kJobsGroupCache}}}};     // TODO add timeout for job

        //
        // create jobs engine with the above config
        //
        JobsEng jobs(config);

        // TODO add config as param so the sleep after can be overridden
        jobs.add_default_processing_function([](auto &j /*this jobs engine*/, const auto &jobs_items) {
            for (auto &item : jobs_items) {
                std::cout << "thread " << std::this_thread::get_id()
                          << " DEFAULT processing "
                          << "{"
                          << " type=" << (int)item->type
                          << " req.int=" << item->request.first << ","
                          << " req.str=\"" << item->request.second << "\""
                          << "}"
                          << " ref count " << item.use_count()
                          << " time " << small::toISOString(small::timeNow())
                          << "\n";
            }
            small::sleep(30);
        });

        // TODO add config as param so the sleep after can be overridden
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
                          << " ref count " << item.use_count()
                          << " time " << small::toISOString(small::timeNow())
                          << "\n";
                // TODO add 2 more children jobs for current one for database and server cache
                // TODO save somewhere in an unordered_map the database requests - the problem is that jobid is received after push_jobs
                // TODO save type1 requests into a promises unordered_map
                // TODO for type 2 only database (add another processing function)
            }
            small::sleep(30); }, 5 /*param b*/);

        // TODO daca as vrea sa folosesc un alt job_server cum modelez asa incat jobul dintr-o parte sa ramana intr-o stare ca si cand ar avea copii si
        // TODO sa se face un request in alta parte si ala cand se termina pe finish (sau daca e worker thread in functia de procesare) sa faca set state
        // TODO set state merge daca e doar o dependinta, daca sunt mai multe atunci ar tb o functie custom - childProcessing (desi are sau nu are children - sau cum fac un dummy children - poate cu thread_count 0?)

        // add specific function for job2
        jobs.add_job_processing_function(JobsType::kJobsType2, [](auto &j /*this jobs engine*/, const auto &jobs_items) {
            for (auto &item : jobs_items) {
                std::cout << "thread " << std::this_thread::get_id()
                          << " JOB2 processing "
                          << "{"
                          << " type=" << (int)item->type
                          << " req.int=" << item->request.first << ","
                          << " req.str=\"" << item->request.second << "\""
                          << "}"
                          << " ref count " << item.use_count()
                          << " time " << small::toISOString(small::timeNow())
                          << "\n";
                // TODO for type 2 only database children (add another processing function)
            }
            // TODO config to wait after request (even if it is not specified in the global config - so custom throttle)
            small::sleep(30); });

        // TODO add function for database where demonstrate coalesce of 3 items (sleep 1000)
        // TODO add function for cache server - no coalesce for demo purposes (sleep 500) so 3rd parent items is finished due to database and not cache server

        // TODO add function for custom wait children and demonstrate set progress to another item
        // TODO add function for custom finished (for type1 to set the promises completed)

        JobsEng::JobsID              jobs_id{};
        std::vector<JobsEng::JobsID> jobs_ids;

        // TODO create a promises/futures unordered_map for type1 requests and wait later

        // show coalesce for children database requests
        std::unordered_map<unsigned int, JobsEng::JobsID> web_requests;

        // push
        jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, {1, "normal"}, &jobs_id);
        jobs.queue().push_back(small::EnumPriorities::kHigh, JobsType::kJobsType2, {2, "high"}, &jobs_id);

        jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, std::make_pair(3, "normal"), &jobs_id);
        jobs.queue().push_back(small::EnumPriorities::kHigh, JobsType::kJobsType1, {4, "high"}, &jobs_id);
        jobs.queue().push_back(small::EnumPriorities::kLow, JobsType::kJobsType1, {5, "low"}, &jobs_id);

        Request req = {6, "normal"};
        jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, req, nullptr);

        std::vector<std::shared_ptr<JobsEng::JobsItem>> jobs_items = {
            std::make_shared<JobsEng::JobsItem>(JobsType::kJobsType1, Request{7, "highest"}),
            std::make_shared<JobsEng::JobsItem>(JobsType::kJobsType1, Request{8, "highest"}),
        };
        jobs.queue().push_back(small::EnumPriorities::kHighest, jobs_items, &jobs_ids);
        jobs.queue().push_back(small::EnumPriorities::kHighest, {std::make_shared<JobsEng::JobsItem>(JobsType::kJobsType1, Request{9, "highest"})}, &jobs_ids);

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