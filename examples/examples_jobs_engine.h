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
            kJobsNone = 0,
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

        using Request  = std::pair<int, std::string>;
        using Response = int;
        using JobsEng  = small::jobs_engine<JobsType, Request, Response, JobsGroupType>;

        auto jobs_function_processing = [](const std::vector<std::shared_ptr<JobsEng::JobsItem>> &items, JobsEng::JobsConfig::ConfigProcessing & /* config */) {
            // this functions is defined without the engine params (it is here just for the example)
            std::cout << "this function is defined without the engine params, called for " << (int)items[0]->m_type << "\n";
        };

        //
        // config object for priorities, groups, types, threads, timeouts, sleeps, etc
        //
        JobsEng::JobsConfig config{
            .m_engine = {.m_threads_count = 0, // dont start any thread yet
                         .m_config_prio   = {.priorities = {{small::EnumPriorities::kHighest, 2},
                                                            {small::EnumPriorities::kHigh, 2},
                                                            {small::EnumPriorities::kNormal, 2},
                                                            {small::EnumPriorities::kLow, 1}}}}, // overall config with default priorities

            .m_default_function_processing = jobs_function_processing, // default processing function, better use jobs.config_default_function_processing to set it

            .m_groups = {{JobsGroupType::kJobsGroup12, {.m_threads_count = 1}}, // config by jobs group
                         {JobsGroupType::kJobsGroup3, {.m_threads_count = 1, .m_delay_next_request = std::chrono::milliseconds(30)}},
                         {JobsGroupType::kJobsGroupDatabase, {.m_threads_count = 1}}, // these requests will coalesce results for demo purposes
                         {JobsGroupType::kJobsGroupCache, {.m_threads_count = 0}}},   // no threads !!, these requests are executed outside of jobs engine for demo purposes

            .m_types = {{JobsType::kJobsType1, {.m_group = JobsGroupType::kJobsGroup12}},
                        {JobsType::kJobsType2, {.m_group = JobsGroupType::kJobsGroup12}},
                        {JobsType::kJobsType3, {.m_group = JobsGroupType::kJobsGroup3, .m_timeout = std::chrono::milliseconds(500)}},
                        {JobsType::kJobsDatabase, {.m_group = JobsGroupType::kJobsGroupDatabase}},
                        {JobsType::kJobsCache, {.m_group = JobsGroupType::kJobsGroupCache}}}};

        //
        // create jobs engine with the above config
        //
        JobsEng jobs(config);

        // create a cache server (with workers to simulate access to it)
        // (as an external engine outside the jobs engine for demo purposes)
        small::worker_thread<JobsEng::JobsID> cache_server({.threads_count = 1}, [&](auto &w /*this*/, const auto &items) {
            for (auto &i : items) {
                std::cout << "thread " << std::this_thread::get_id()
                          << " CACHE   processing"
                          << " {" << i << "}" << "\n";

                // mark the jobs id associated as succeeded (for demo purposes to avoid creating other structures)
                jobs.jobs_finished(i, (Response)i);
            }
            // sleep long enough
            // no coalesce for demo purposes (sleep 500) so 3rd parent items is finished due to database and not cache server
            small::sleep(500);
        });

        // default processing used for job type 3 with custom delay in between requests
        // one request will succeed and one request will timeout for demo purposes
        jobs.config_default_function_processing([](auto &j /*this jobs engine*/, const auto &jobs_items, auto &jobs_config) {
            for (auto &item : jobs_items) {
                std::cout << "thread " << std::this_thread::get_id()
                          << " DEFAULT processing "
                          << "{"
                          << " type=" << (int)item->m_type
                          << " req.int=" << item->m_request.first << ","
                          << " req.str=\"" << item->m_request.second << "\""
                          << "}"
                          << " ref count " << item.use_count()
                          << " time " << small::toISOString(small::timeNow())
                          << "\n";
            }

            // set a custom delay (timeout for job3 is 500 ms)
            jobs_config.m_delay_next_request = std::chrono::milliseconds(1000);
        });

        // add specific function for job1 (calling the function from jobs intead of config allows to pass the engine and extra param)
        jobs.config_jobs_function_processing(JobsType::kJobsType1, [&](auto &j /*this jobs engine*/, const auto &jobs_items, auto & /* config */, auto b /*extra param b*/) {
            for (auto &item : jobs_items) {
                std::cout << "thread " << std::this_thread::get_id()
                          << " JOB1    processing "
                          << "{"
                          << " type=" << (int)item->m_type
                          << " req.int=" << item->m_request.first << ","
                          << " req.str=\"" << item->m_request.second << "\""
                          << "}"
                          << " ref count " << item.use_count()
                          << " time " << small::toISOString(small::timeNow())
                          << "\n";

                // add 2 more children jobs for current one for database and server cache
                JobsEng::JobsID jobs_child_db_id{};
                JobsEng::JobsID jobs_child_cache_id{};

                auto ret = j.queue().push_back_child(item->m_id /*parent*/, JobsType::kJobsDatabase, item->m_request, &jobs_child_db_id);
                if (!ret) {
                    j.jobs_failed(item->m_id);
                }
                ret = j.queue().push_back_child(item->m_id /*parent*/, JobsType::kJobsCache, item->m_request, &jobs_child_cache_id);
                if (!ret) {
                    j.jobs_failed(jobs_child_db_id);
                    j.jobs_failed(item->m_id);
                }

                j.jobs_start(small::EnumPriorities::kNormal, jobs_child_db_id);
                // jobs_child_cache_id has no threads to execute, it has external executors
                cache_server.push_back(jobs_child_cache_id);
            }
            small::sleep(30); }, 5 /*param b*/);

        // TODO save type1 requests into a promises unordered_map and complete on finishing the job
        // TODO add custom finish function for jobtype1 to complete the promises

        // TODO save somewhere in an unordered_map the database requests (passes as request params for job type1)
        // TODO daca as vrea sa folosesc un alt job_server cum modelez asa incat jobul dintr-o parte sa ramana intr-o stare ca si cand ar avea copii si
        // TODO sa se faca un request in alta parte si ala cand se termina pe finish (sau daca e worker thread in functia de procesare) sa faca set state
        // TODO set state merge daca e doar o dependinta, daca sunt mai multe atunci ar tb o functie custom - childProcessing (desi are sau nu are children - sau cum fac un dummy children - poate cu thread_count 0?)

        // add specific function for job2
        jobs.config_jobs_function_processing(JobsType::kJobsType2, [](auto &j /*this jobs engine*/, const auto &jobs_items, auto & /* config */) {
            bool first_job = true;
            for (auto &item : jobs_items) {
                std::cout << "thread " << std::this_thread::get_id()
                          << " JOB2    processing "
                          << "{"
                          << " type=" << (int)item->m_type
                          << " req.int=" << item->m_request.first << ","
                          << " req.str=\"" << item->m_request.second << "\""
                          << "}"
                          << " ref count " << item.use_count()
                          << " time " << small::toISOString(small::timeNow())
                          << "\n";

                if (first_job) {
                    // for type 2 only database children (for demo purposes no result will be used from database)
                    auto ret = j.queue().push_back_and_start_child(item->m_id /*parent*/,
                                                                   small::EnumPriorities::kNormal, 
                                                                   JobsType::kJobsDatabase, 
                                                                   item->m_request);
                    if (!ret) {
                        j.jobs_failed(item->m_id);
                    }
                } else {
                    j.jobs_failed(item->m_id);
                }

                first_job = false;
            }
            // TODO config to wait after request (even if it is not specified in the global config - so custom throttle)
            small::sleep(30); });

        // TODO add function for database where demonstrate coalesce of 3 items (sleep 1000)
        // TODO add function for cache server - no coalesce for demo purposes (sleep 500) so 3rd parent items is finished due to database and not cache server

        // TODO add function for custom wait children and demonstrate set progress to another item
        // TODO add function for custom finished (for type1 to set the promises completed)

        JobsEng::JobsID              jobs_id{};
        std::vector<JobsEng::JobsID> jobs_ids;

        // type3 one request will succeed and one request will timeout for demo purposes
        jobs.queue().push_back_and_start(small::EnumPriorities::kNormal, JobsType::kJobsType3, {3, "normal3"}, &jobs_id);
        jobs.queue().push_back_and_start(small::EnumPriorities::kHigh, JobsType::kJobsType3, {3, "high3"}, &jobs_id);

        // type2 only the first request succeeds and waits for child the other fails from the start
        jobs.queue().push_back_and_start(small::EnumPriorities::kNormal, JobsType::kJobsType2, {2, "normal2"}, &jobs_id);
        jobs.queue().push_back_and_start(small::EnumPriorities::kHigh, JobsType::kJobsType2, {2, "high2"}, &jobs_id);

        // show coalesce for children database requests
        std::unordered_map<unsigned int, JobsEng::JobsID> web_requests;

        // TODO create a promises/futures unordered_map for type1 requests and wait later
        // push with multiple variants
        jobs.queue().push_back_and_start(small::EnumPriorities::kNormal, JobsType::kJobsType1, {11, "normal11"}, &jobs_id);

        std::vector<std::shared_ptr<JobsEng::JobsItem>> jobs_items = {
            std::make_shared<JobsEng::JobsItem>(JobsType::kJobsType1, Request{12, "highest12"}),
        };
        jobs.queue().push_back_and_start(small::EnumPriorities::kHighest, jobs_items, &jobs_ids);

        jobs.queue().push_back_and_start(small::EnumPriorities::kLow, JobsType::kJobsType1, {13, "low13"}, nullptr);

        Request req = {14, "normal14"};
        jobs.queue().push_back(JobsType::kJobsType1, req, &jobs_id);
        jobs.jobs_start(small::EnumPriorities::kNormal, jobs_id);

        jobs.queue().push_back_and_start_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {115, "delay normal115"}, &jobs_id);

        // manual start threads
        jobs.start_threads(3);

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