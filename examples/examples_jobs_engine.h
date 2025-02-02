#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

#include "../include/jobs_engine.h"

namespace examples::jobs_engine {
    namespace impl {
        //
        // create a complex example for each type (like web requests)
        // - priorities
        // - dependencies calls (parent-child relationship)
        //      (with children for calls that take long time like database access or less time like cache server)
        // - coalesce calls (multiple (later) calls resolved when first one is called)
        // - timeout
        // - throttle (call with some wait-sleep interval in between)
        //
        // POST     will create 2 children (DB+CACHE) and will be finished when ALL will be finished
        //          also CACHE will depend on DB and will start only after DB is finished
        //              (this will demonstrate the default case AND and default children function)
        // GET      will create 2 children (CACHE+DB) and will be finished when at least 1 is finished
        //              (this will demonstrate OR and custom children function)
        // DELETE   will create 2 children (DB+CACHE) and will be finished when DB is finished
        //              (this will demonstrate the default case AND and custom children function)
        // SETTINGS will have no children and second call will timeout due to custom setting for timeout
        // DB       will coalesce calls because calls will simulate to take long
        // CACHE    will simulate small time and also demonstrate that it is running outside of the jobs engine
        //
        enum class JobsType
        {
            kJobsNone = 0,
            kJobsSettings,
            kJobsApiPost,
            kJobsApiGet,
            kJobsApiDelete,
            kJobsDatabase,
            kJobsCache,
        };

        enum class JobsGroupType
        {
            kJobsGroupDefault,
            kJobsGroupApi,
            kJobsGroupDatabase,
            kJobsGroupCache,
        };

        using WebID       = int;
        using WebData     = std::string;
        using WebRequest  = std::pair<WebID, WebData>;
        using WebResponse = WebData;
        using JobsEng     = small::jobs_engine<JobsType, WebRequest, WebResponse, JobsGroupType>;

        inline std::string to_string(JobsType type)
        {
            switch (type) {
            case JobsType::kJobsNone:
                return "JobsNone";
            case JobsType::kJobsSettings:
                return "JobsSettings";
            case JobsType::kJobsApiPost:
                return "JobsApiPost";
            case JobsType::kJobsApiGet:
                return "JobsApiGet";
            case JobsType::kJobsApiDelete:
                return "JobsApiDelete";
            case JobsType::kJobsDatabase:
                return "JobsDatabase";
            case JobsType::kJobsCache:
                return "JobsCache";
            default:
                return "JobsUnknown";
            }
        }

        inline std::string to_string(JobsGroupType type)
        {
            switch (type) {
            case JobsGroupType::kJobsGroupDefault:
                return "GroupDefault";
            case JobsGroupType::kJobsGroupApi:
                return "GroupApi";
            case JobsGroupType::kJobsGroupDatabase:
                return "GroupDatabase";
            case JobsGroupType::kJobsGroupCache:
                return "GroupCache";
            default:
                return "GroupUnknown";
            }
        }

        inline std::string to_string(const small::jobsimpl::EnumJobsState &state)
        {
            switch (state) {
            case small::jobsimpl::EnumJobsState::kNone:
                return "None";
            case small::jobsimpl::EnumJobsState::kInProgress:
                return "InProgress";
            case small::jobsimpl::EnumJobsState::kWaitChildren:
                return "WaitChildren";
            case small::jobsimpl::EnumJobsState::kFinished:
                return "Finished";
            case small::jobsimpl::EnumJobsState::kTimeout:
                return "Timeout";
            case small::jobsimpl::EnumJobsState::kFailed:
                return "Failed";
            case small::jobsimpl::EnumJobsState::kCancelled:
                return "Cancelled";
            default:
                return "Unknown";
            }
        }

        inline std::string to_string(const WebRequest &request)
        {
            return "{ id=" + std::to_string(request.first) + ", data=\"" + request.second + "\" }";
        }

        //
        // processing helper for cache and database
        //
        inline bool data_processing(JobsEng &jobs, const std::string &data_type, std::shared_ptr<impl::JobsEng::JobsItem> jobs_item)
        {
            using DataMap = std::unordered_map<WebID, WebData>;
            using TypeMap = std::unordered_map<std::string, DataMap>;
            static TypeMap data;

            // use the lock from jobs engine
            std::unique_lock l(jobs);

            const auto &[webID, webData] = jobs_item->m_request;

            bool success = false;
            // simulate cache server work
            if (jobs_item->m_type == JobsType::kJobsApiPost) {
                data[data_type][webID] = webData;
                success                = true;

                std::cout << "ADD TO " << data_type
                          << " {" << webID << ", " << webData << "} jobid=" << jobs_item->m_id
                          << " time " << small::toISOString(small::timeNow())
                          << " thread " << std::this_thread::get_id()
                          << "\n";
            } else if (jobs_item->m_type == JobsType::kJobsApiDelete) {

                auto it_data = data[data_type].find(webID);
                success      = it_data != data[data_type].end();

                if (success) {
                    // save the response
                    jobs_item->m_response = it_data->second;
                    data[data_type].erase(it_data);

                    std::cout << "DELETE FROM " << data_type
                              << " {" << webID << ", " << it_data->second << "} jobid=" << jobs_item->m_id
                              << " time " << small::toISOString(small::timeNow())
                              << " thread " << std::this_thread::get_id()
                              << "\n";
                } else {
                    std::cout << "DELETE NOT FOUND IN " << data_type
                              << " {" << webID << "} jobid=" << jobs_item->m_id
                              << " time " << small::toISOString(small::timeNow())
                              << " thread " << std::this_thread::get_id()
                              << "\n";
                }

            } else if (jobs_item->m_type == JobsType::kJobsApiGet) {
                auto it_data = data[data_type].find(webID);
                success      = it_data != data[data_type].end();

                if (success) {
                    // save the response
                    jobs_item->m_response = it_data->second;

                    std::cout << "GET FROM " << data_type
                              << " {" << webID << ", " << it_data->second << "} jobid=" << jobs_item->m_id
                              << " time " << small::toISOString(small::timeNow())
                              << " thread " << std::this_thread::get_id()
                              << "\n";
                } else {
                    std::cout << "GET NOT FOUND IN " << data_type
                              << " {" << webID << "} jobid=" << jobs_item->m_id
                              << " time " << small::toISOString(small::timeNow())
                              << " thread " << std::this_thread::get_id()
                              << "\n";
                }
            }

            return success;
        }

        //
        // cache processing
        //
        inline bool cache_processing(JobsEng &jobs, std::shared_ptr<impl::JobsEng::JobsItem> jobs_item)
        {
            return data_processing(jobs, "CACHE", jobs_item);
        }

        //
        // database processing
        //
        using DbRequests = std::vector<impl::JobsEng::JobsID>;

        inline void db_add_request(JobsEng &jobs, DbRequests &db_requests, const impl::JobsEng::JobsID &jobs_id)
        {
            std::unique_lock l(jobs); // better use another lock
            db_requests.push_back(jobs_id);
        }

        inline bool db_processing(JobsEng &jobs, std::shared_ptr<impl::JobsEng::JobsItem> jobs_item)
        {
            return data_processing(jobs, "DATABASE", jobs_item);
        }

        inline DbRequests db_call(JobsEng &jobs, DbRequests &db_requests, const std::vector<std::shared_ptr<impl::JobsEng::JobsItem>> &jobs_items)
        {
            // coalesce calls
            DbRequests requests;
            {
                std::unique_lock l(jobs);
                std::swap(requests, db_requests);
            }

            std::stringstream ssr;
            for (auto &jobs_id : requests) {
                ssr << jobs_id << " ";
            }

            std::stringstream ssj;
            for (auto &jobs_item : jobs_items) {
                ssj << jobs_item->m_id << " ";
            }

            // simulate long db call
            small::sleep(200);

            std::cout << "DATABASE processing (coalesced) calls " << ssr.str()
                      << " current calls " << ssj.str()
                      << " time " << small::toISOString(small::timeNow())
                      << " thread " << std::this_thread::get_id()
                      << "\n";

            return requests;
        }

    } // namespace impl

    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Jobs Engine example 1\n";

        // this functions is defined without the engine params (it is here just for the example)
        auto jobs_function_processing = [](const std::vector<std::shared_ptr<impl::JobsEng::JobsItem>> &items, impl::JobsEng::JobsConfig::ConfigProcessing & /* config */) {
            std::cout << "this function is defined without the engine params, called for " << (int)items[0]->m_type << "\n";
        };

        //
        // CONFIG object for priorities, groups, types, threads, timeouts, sleeps, etc
        //
        impl::JobsEng::JobsConfig config{
            .m_engine = {.m_threads_count = 0, // dont start any thread yet
                         .m_config_prio   = {.priorities = {{small::EnumPriorities::kHighest, 2},
                                                            {small::EnumPriorities::kHigh, 2},
                                                            {small::EnumPriorities::kNormal, 2},
                                                            {small::EnumPriorities::kLow, 1}}}}, // overall config with default priorities

            .m_default_function_processing = jobs_function_processing, // default processing function, better use jobs.config_default_function_processing to set it

            // config by jobs group
            .m_groups = {{impl::JobsGroupType::kJobsGroupDefault, {.m_threads_count = 1, .m_delay_next_request = std::chrono::milliseconds(30)}},
                         {impl::JobsGroupType::kJobsGroupApi, {.m_threads_count = 1}},
                         {impl::JobsGroupType::kJobsGroupDatabase, {.m_threads_count = 1}}, // these requests will coalesce results for demo purposes
                         {impl::JobsGroupType::kJobsGroupCache, {.m_threads_count = 0}}},   // no threads !!, these requests are executed outside of jobs engine for demo purposes

            // config by jobs type
            .m_types = {{impl::JobsType::kJobsSettings, {.m_group = impl::JobsGroupType::kJobsGroupDefault, .m_timeout = std::chrono::milliseconds(500)}},
                        {impl::JobsType::kJobsApiPost, {.m_group = impl::JobsGroupType::kJobsGroupApi}},
                        {impl::JobsType::kJobsApiGet, {.m_group = impl::JobsGroupType::kJobsGroupApi}},
                        {impl::JobsType::kJobsApiDelete, {.m_group = impl::JobsGroupType::kJobsGroupApi}},
                        {impl::JobsType::kJobsDatabase, {.m_group = impl::JobsGroupType::kJobsGroupDatabase}},
                        {impl::JobsType::kJobsCache, {.m_group = impl::JobsGroupType::kJobsGroupCache}}}};

        //
        // create JOBS ENGINE with the above config
        //
        impl::JobsEng jobs(config);

        auto fn_print_item = [](auto item, std::string fn_type) {
            std::cout << std::setw(10) << fn_type
                      << " processing "
                      << "{"
                      << " jobid=" << std::setw(2) << item->m_id
                      << " type=" << std::setw(10) << impl::to_string(item->m_type)
                      << " state=" << std::setw(12) << impl::to_string(item->m_state.load())
                      << " req=" << impl::to_string(item->m_request)
                      << "}"
                      << " time " << small::toISOString(small::timeNow())
                      << " thread " << std::this_thread::get_id()
                      << "\n";
        };

        //
        // DEFAULTS
        //
        // default processing used for kJobsSettings with custom delay in between requests
        // one request will succeed and one request will timeout for demo purposes
        jobs.config_default_function_processing([&fn_print_item](auto &j /*this jobs engine*/, const auto &jobs_items, auto &jobs_config) {
            for (auto &item : jobs_items) {
                fn_print_item(item, "DEFAULT");
            }

            // set a custom delay (timeout for kJobsSettings is 500 ms) so next one will timeout
            jobs_config.m_delay_next_request = std::chrono::milliseconds(600);
        });

        jobs.config_default_function_finished([&fn_print_item](auto &j /*this jobs engine*/, const auto &jobs_items) {
            for (auto &item : jobs_items) {
                fn_print_item(item, "FINISHED");
            }
        });

        //
        // SETTINGS
        //
        // and setup specific finish function for kJobsSettings
        // to setup the promises/futures for the requests and complete them on finish
        std::unordered_map<impl::JobsEng::JobsID, std::promise<bool>> settings_promises;
        jobs.config_jobs_function_finished(impl::JobsType::kJobsSettings, [&settings_promises](auto &j /*this jobs engine*/, const auto &jobs_items) {
            for (auto &item : jobs_items) {
                auto it_promise = settings_promises.find(item->m_id);
                if (it_promise != settings_promises.end()) {
                    it_promise->second.set_value(item->is_state_finished() /*success finish or not (not is like timeout)*/);
                }
            }
        });

        //
        // DATABASE
        //
        // add specific function for kJobsDatabase
        // will coalesce calls because calls will simulate to take long
        impl::DbRequests db_requests;

        jobs.config_jobs_function_processing(impl::JobsType::kJobsDatabase, [&db_requests](auto &j /*this jobs engine*/, const auto &jobs_items, auto & /* config */) {
            // will coalesce current requests
            auto requests = db_call(j, db_requests, jobs_items);

            auto jobs_requests = j.jobs_get(requests);
            for (auto &jobs_item : jobs_requests) {
                // save the response
                auto success = impl::db_processing(j, jobs_item);

                // mark dbs as finished (this should include current jobs_item too, unless they were coalesced before)
                if (success) {
                    j.jobs_finished(jobs_item->m_id); // result is in the response already
                } else {
                    j.jobs_failed(jobs_item->m_id); // 404 not found
                }
            }
        });

        //
        // CACHE
        //
        // create a cache server (with workers to simulate access to it)
        // as an external engine outside the jobs engine for demo purposes
        small::worker_thread<std::shared_ptr<impl::JobsEng::JobsItem>> cache_server({.threads_count = 1}, [&jobs](auto &w /*this*/, const auto &items) {
            // simulate small time for cache server (let's say one roundtrip to the server)
            small::sleep(10);

            for (auto &item : items) {
                auto &job_id  = item->m_id;
                bool  success = impl::cache_processing(jobs, item);
                if (success) {
                    jobs.jobs_finished(job_id); // the data found will be returned in m_response already
                } else {
                    jobs.jobs_failed(job_id); // 404 not found
                }
            }
        });

        // cache may have as child the db (in POST scenario)
        // add custom children finish function to actually add the cache item into the processing cache worker
        jobs.config_jobs_function_children_finished(impl::JobsType::kJobsCache, [&cache_server](auto &j /*this jobs engine*/, auto jobs_item /*parent*/, auto jobs_child) {
            if (jobs_child->is_state_finished()) {
                // cache has external executors
                cache_server.push_back(jobs_item); // when it will be finished parent POST will be automatically finished
            } else {
                // if db failed, cache will fail too
                // because this function overrides the default children function jobs need to be failed manually
                j.jobs_cancelled(jobs_item->m_id);
            }
        });

        //
        // POST
        //
        // will create 2 children (DB+CACHE) and will be finished when ALL will be finished
        // also CACHE will depend on DB and will start only after DB is finished
        //      (this is the default case for finish function AND and default for children function)
        jobs.config_jobs_function_processing(impl::JobsType::kJobsApiPost, [&fn_print_item, &db_requests](auto &j /*this jobs engine*/, const auto &jobs_items, auto & /* config */, auto b /*extra param b*/) {
            for (auto &jobs_item : jobs_items) {
                fn_print_item(jobs_item, "POST");

                // create first the cache child (it will start when db is finished)
                impl::JobsEng::JobsID jobs_child_cache_id{};
                
                auto ret = j.queue().push_back_child(jobs_item->m_id /*parent*/, impl::JobsType::kJobsCache, jobs_item->m_request, &jobs_child_cache_id);
                if (!ret) {
                    j.jobs_failed(jobs_item->m_id);
                    continue;
                }

                // create the database child with cache as parent (but dont start yet)
                impl::JobsEng::JobsID jobs_child_db_id{};
                ret = j.queue().push_back_child(jobs_child_cache_id /*cache parent*/, impl::JobsType::kJobsDatabase, jobs_item->m_request, &jobs_child_db_id);
                if (!ret) {
                    j.jobs_failed(jobs_item->m_id); // set parent to failed here even thou the failure of child will trigger the parent to fail
                    j.jobs_failed(jobs_child_cache_id);
                    continue;
                }

                // add to db_requests
                impl::db_add_request(j, db_requests, jobs_child_db_id);
                // start only the db request (because cache is the parent of db, when db is finished the cache will be started)
                j.jobs_start(small::EnumPriorities::kNormal, jobs_child_db_id);
            }

            small::sleep(30); }, 5 /*param b*/);

        //
        // GET
        //
        // will create 2 children (DB+CACHE) and will be finished when at least 1 is finished
        //      unlike POST where children are chained, here there is a star topology even thou the DB will wait for cache
        //      (normally this will need 3 children if CACHE (not found) -> DB -> CACHE update)
        //
        jobs.config_jobs_function_processing(impl::JobsType::kJobsApiGet, [&fn_print_item, &db_requests, &cache_server](auto &j /*this jobs engine*/, const auto &jobs_items, auto &jobs_config) {
            for (auto &jobs_item : jobs_items) {
                fn_print_item(jobs_item, "GET");

                // create children
                impl::JobsEng::JobsID jobs_child_cache_id{};
                impl::JobsEng::JobsID jobs_child_db_id{};

                auto ret_cache = j.queue().push_back_child(jobs_item->m_id /*parent*/, impl::JobsType::kJobsCache, jobs_item->m_request, &jobs_child_cache_id);
                auto ret_db    = j.queue().push_back_child(jobs_item->m_id /*parent*/, impl::JobsType::kJobsDatabase, jobs_item->m_request, &jobs_child_db_id);

                // if both failed, then the parent will fail too
                if (!ret_cache && !ret_db) {
                    j.jobs_failed(jobs_item->m_id);
                    if (ret_cache) {
                        j.jobs_failed(jobs_child_cache_id);
                    }
                    if (ret_db) {
                        j.jobs_failed(jobs_child_db_id);
                    }
                    continue;
                }

                // the db request will be started (if not found in cache) in children function
                // add to db_requests
                impl::db_add_request(j, db_requests, jobs_child_db_id); // this may execute earlier due to coalesce db calls

                // start the external cache executor
                auto jobs_cache = j.jobs_get(jobs_child_cache_id);
                cache_server.push_back(jobs_cache);
            }

            // config to wait after request (even if it is not specified in the global config - so custom throttle)
            jobs_config.m_delay_next_request = std::chrono::milliseconds(30);
        });

        // custom children function for GET to finish the parent when at least 1 child is finished
        jobs.config_jobs_function_children_finished(impl::JobsType::kJobsApiGet, [](auto &j /*this jobs engine*/, auto jobs_item /*parent*/, auto jobs_child) {
            if (jobs_child->m_type == impl::JobsType::kJobsCache) {
                std::string           response;
                impl::JobsEng::JobsID jobs_child_db_id{};
                {
                    std::unique_lock l(j);
                    response         = jobs_child->m_response;
                    jobs_child_db_id = jobs_item->m_childrenIDs.size() >= 2 ? jobs_item->m_childrenIDs[1] : 0;
                }
                if (jobs_child->is_state_finished()) {
                    // found in cache, finish the db, and the parent should be finished too
                    // parent could be finished earlier due to db coalesce calls
                    j.jobs_finished(jobs_child_db_id, response); // this will call children function again for the db
                } else {
                    // cache failed, start the db request
                    j.jobs_start(small::EnumPriorities::kNormal, jobs_child_db_id);
                }
            } else /* if (jobs_child->m_type == impl::JobsType::kJobsDatabase) */ {
                // db is finished, so the parent should be finished with same state as the db
                if (jobs_child->is_state_finished()) {
                    // finish the parent
                    std::string response;
                    {
                        std::unique_lock l(j);
                        response = jobs_child->m_response;
                    }
                    j.jobs_finished(jobs_item->m_id, response);
                } else {
                    // fail the parent with the same state as the db
                    j.jobs_set_state(jobs_item->m_id, jobs_child->m_state.load());
                }
            }
        });

        //
        // DELETE
        //
        // will create 2 children (DB+CACHE) and will be finished when DB is finished
        //  (this will demonstrate the default case AND and custom children function)
        jobs.config_jobs_function_processing(impl::JobsType::kJobsApiDelete, [&fn_print_item, &db_requests, &cache_server](auto &j /*this jobs engine*/, const auto &jobs_items, auto &jobs_config) {
            for (auto &jobs_item : jobs_items) {
                fn_print_item(jobs_item, "DELETE");

                // create children
                impl::JobsEng::JobsID jobs_child_cache_id{};
                impl::JobsEng::JobsID jobs_child_db_id{};

                auto ret_cache = j.queue().push_back_child(jobs_item->m_id /*parent*/, impl::JobsType::kJobsCache, jobs_item->m_request, &jobs_child_cache_id);
                auto ret_db    = j.queue().push_back_child(jobs_item->m_id /*parent*/, impl::JobsType::kJobsDatabase, jobs_item->m_request, &jobs_child_db_id);

                // if both failed, then the parent will fail too
                if (!ret_cache && !ret_db) {
                    j.jobs_failed(jobs_item->m_id);
                    if (ret_cache) {
                        j.jobs_failed(jobs_child_cache_id);
                    }
                    if (ret_db) {
                        j.jobs_failed(jobs_child_db_id);
                    }
                    continue;
                }

                // the db request will be started (if not found in cache) in children function
                // add to db_requests
                impl::db_add_request(j, db_requests, jobs_child_db_id); // this may execute earlier due to coalesce db calls

                // start the db request
                j.jobs_start(small::EnumPriorities::kNormal, jobs_child_db_id);

                // start the external cache executor
                auto jobs_cache = j.jobs_get(jobs_child_cache_id);
                cache_server.push_back(jobs_cache);
            }
        });

        // custom children function for DELETE to finish the parent only if the db is finished
        jobs.config_jobs_function_children_finished(impl::JobsType::kJobsApiGet, [](auto &j /*this jobs engine*/, auto jobs_item /*parent*/, auto jobs_child) {
            if (jobs_child->m_type == impl::JobsType::kJobsDatabase) {
                // db is finished, so the parent should be finished with same state as the db
                std::string response;
                {
                    std::unique_lock l(j);
                    response = jobs_child->m_response;
                }
                j.jobs_set_state(jobs_item->m_id, jobs_child->m_state.load(), response);
            }
        });

        //
        // ADD JOBS
        //
        impl::JobsEng::JobsID              jobs_id{};
        std::vector<impl::JobsEng::JobsID> jobs_ids;

        // kJobsSettings one request will succeed and one request will timeout for demo purposes
        jobs.queue().push_back(impl::JobsType::kJobsSettings, {1, "normal1"}, &jobs_id);
        settings_promises[jobs_id] = std::promise<bool>();
        // TODO jobs.queue().start_delay_for(std::chrono::milliseconds(100), small::EnumPriorities::kNormal, jobs_id);
        jobs.queue().push_back(impl::JobsType::kJobsSettings, {2, "high2"}, &jobs_id);
        settings_promises[jobs_id] = std::promise<bool>();
        // TODO jobs.queue().start_delay_for(std::chrono::milliseconds(100), small::EnumPriorities::kHigh, jobs_id);

        // type2 only the first request succeeds and waits for child the other fails from the start
        // jobs.queue().push_back_and_start(small::EnumPriorities::kNormal, impl::JobsType::kJobsType2, {2, "normal2"}, &jobs_id);
        // jobs.queue().push_back_and_start(small::EnumPriorities::kHigh, JobsType::kJobsType2, {2, "high2"}, &jobs_id);

        // // show coalesce for children database requests
        // std::unordered_map<unsigned int, JobsEng::JobsID> web_requests;

        // // push with multiple variants
        // jobs.queue().push_back_and_start(small::EnumPriorities::kNormal, JobsType::kJobsType1, {11, "normal11"}, &jobs_id);

        // std::vector<std::shared_ptr<JobsEng::JobsItem>> jobs_items = {
        //     std::make_shared<JobsEng::JobsItem>(JobsType::kJobsType1, Request{12, "highest12"}),
        // };
        // jobs.queue().push_back_and_start(small::EnumPriorities::kHighest, jobs_items, &jobs_ids);

        // jobs.queue().push_back_and_start(small::EnumPriorities::kLow, JobsType::kJobsType1, {13, "low13"}, nullptr);

        // Request req = {14, "normal14"};
        // jobs.queue().push_back(JobsType::kJobsType1, req, &jobs_id);
        // jobs.jobs_start(small::EnumPriorities::kNormal, jobs_id);

        // jobs.queue().push_back_and_start_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {115, "delay normal115"}, &jobs_id);

        // manual start threads
        jobs.start_threads(3);

        // show wait for with timeout
        auto ret = jobs.wait_for(std::chrono::milliseconds(100)); // wait to finished
        std::cout << "wait for with timeout, ret = " << static_cast<int>(ret) << " as timeout\n";

        // show wait for custom promises
        for (auto &[id, promise] : settings_promises) {
            auto f       = promise.get_future();
            auto success = f.get();
            std::cout << "promise for jobid=" << id << " success=" << success << "\n";
        }

        // wait here for jobs to finish due to exit flag
        jobs.wait();

        // cache should exit immediately after all jobs are finished
        cache_server.signal_exit_force();
        cache_server.wait();

        // remaining work should be zero
        std::cout << "size = " << jobs.size() << "\n";

        std::cout << "Jobs Engine example 1 finish\n\n";

        return 0;
    }

} // namespace examples::jobs_engine