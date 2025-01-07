#pragma once

#include <unordered_map>

#include "impl/jobs_engine_thread_pool_impl.h"
#include "impl/jobs_item_impl.h"
#include "impl/jobs_queue_impl.h"
#include "jobs_config.h"

// enum class JobsType
// {
//     kJobsType1,
//     kJobsType2
// };
// enum class JobsGroupType
// {
//     kJobsGroup1
// };
//
// using Request = std::pair<int, std::string>;
// using JobsEng = small::jobs_engine<JobsType, Request, int /*response*/, JobsGroupType>;
//
//
// JobsEng::JobsConfig config{
//     .m_engine                      = {.m_threads_count = 0 /*dont start any thread yet*/},   // overall config with default priorities
//     .m_groups                      = {
//          {JobsGroupType::kJobsGroup1, {.m_threads_count = 1}}},                              // config by jobs group
//     .m_types                       = {
//         {JobsType::kJobsType1, {.m_group = JobsGroupType::kJobsGroup1}},
//         {JobsType::kJobsType2, {.m_group = JobsGroupType::kJobsGroup1}},
//     }};
//
// // create jobs engine
// JobsEng jobs(config);
//
// jobs.add_default_function_processing([](auto &j /*this jobs engine*/, const auto &jobs_items) {
//     for (auto &item : jobs_items) {
//         ...
//     }
// });
//
// // add specific function for job1
// jobs.add_job_function_processing(JobsType::kJobsType1, [](auto &j /*this jobs engine*/, const auto &jobs_items, auto b /*extra param b*/) {
//     for (auto &item : jobs_items) {
//         ...
//     }
// }, 5 /*param b*/);
//
// JobsEng::JobsID              jobs_id{};
// std::vector<JobsEng::JobsID> jobs_ids;
//
// // push
// jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, {1, "normal"}, &jobs_id);
// jobs.queue().push_back_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {100, "delay normal"}, &jobs_id);
//
// jobs.start_threads(3); // manual start threads
//
// // jobs.signal_exit_force();
// jobs.wait(); // wait here for jobs to finish due to exit flag
//
// for a more complex example see examples/examples_jobs_engine.h

namespace small {

    //
    // small class for jobs where job items have a type (which are further grouped by group), priority, request and response
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename JobsGroupT = JobsTypeT, typename JobsPrioT = EnumPriorities>
    class jobs_engine
    {
    public:
        using ThisJobsEngine             = typename small::jobs_engine<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        using JobsConfig                 = typename small::jobs_config<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        using JobsItem                   = typename small::jobsimpl::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;
        using JobsQueue                  = typename small::jobsimpl::jobs_queue<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT, ThisJobsEngine>;
        using JobsID                     = typename JobsItem::JobsID;
        using TimeClock                  = typename JobsQueue::TimeClock;
        using TimeDuration               = typename JobsQueue::TimeDuration;
        using FunctionProcessing         = typename JobsConfig::FunctionProcessing;
        using FunctionOnChildrenFinished = typename JobsConfig::FunctionOnChildrenFinished;
        using FunctionFinished           = typename JobsConfig::FunctionFinished;

    public:
        //
        // jobs_engine
        //
        explicit jobs_engine(const JobsConfig &config = {})
            : m_config{config}
        {
            apply_config();
        }

        explicit jobs_engine(JobsConfig &&config)
            : m_config{config}
        {
            apply_config();
        }

        ~jobs_engine()
        {
            wait();
        }

        // size of active items
        inline size_t size() { return m_queue.size(); }
        // empty
        inline bool empty() { return size() == 0; }
        // clear
        inline void clear()
        {
            std::unique_lock l(m_queue);
            m_queue.clear();
            m_thread_pool.clear();
        }

        // clang-format off
        // size of working items
        inline size_t   size_processing () { return m_thread_pool.size();  }
        // empty
        inline bool     empty_processing() { return size_processing() == 0; }
        // clear
        inline void     clear_processing() { m_thread_pool.clear(); }

        // size of delayed items
        inline size_t   size_delayed() { return queue().size_delayed();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { queue().clear_delayed(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_engine<T>> m...)
        inline void     lock        () { queue().lock(); }
        inline void     unlock      () { queue().unlock(); }
        inline bool     try_lock    () { return queue().try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            m_config.m_engine.m_threads_count = threads_count;
            m_queue.start_threads(threads_count);
            m_thread_pool.start_threads(threads_count);
        }

        //
        // config
        // THIS SHOULD BE DONE IN THE INITIAL SETUP PHASE ONCE
        //
        inline void set_config(const JobsConfig &config)
        {
            m_config = config;
            apply_config();
        }

        inline void set_config(JobsConfig &&config)
        {
            m_config = std::move(config);
            apply_config();
        }

        // override default jobs function
        template <typename _Callable, typename... Args>
        inline void add_default_function_processing(_Callable function_processing, Args... extra_parameters)
        {
            m_config.add_default_function_processing(std::bind(std::forward<_Callable>(function_processing), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::placeholders::_2 /*config*/, std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void add_default_function_on_children_finished(_Callable function_on_children_finished, Args... extra_parameters)
        {
            m_config.add_default_function_on_children_finished(std::bind(std::forward<_Callable>(function_on_children_finished), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::placeholders::_2 /*config*/, std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void add_default_function_finished(_Callable function_finished, Args... extra_parameters)
        {
            m_config.add_default_function_finished(std::bind(std::forward<_Callable>(function_finished), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::placeholders::_2 /*config*/, std::forward<Args>(extra_parameters)...));
        }

        // specific jobs functions
        template <typename _Callable, typename... Args>
        inline void add_job_function_processing(const JobsTypeT &jobs_type, _Callable function_processing, Args... extra_parameters)
        {
            m_config.add_job_function_processing(jobs_type, std::bind(std::forward<_Callable>(function_processing), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::placeholders::_2 /*config*/, std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void add_job_function_on_children_finished(const JobsTypeT &jobs_type, _Callable function_on_children_finished, Args... extra_parameters)
        {
            m_config.add_job_function_on_children_finished(jobs_type, std::bind(std::forward<_Callable>(function_on_children_finished), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void add_job_function_finished(const JobsTypeT &jobs_type, _Callable function_finished, Args... extra_parameters)
        {
            m_config.add_job_function_finished(jobs_type, std::bind(std::forward<_Callable>(function_finished), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::forward<Args>(extra_parameters)...));
        }

        //
        // queue access
        //
        inline JobsQueue &queue() { return m_queue; }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { m_thread_pool.signal_exit_force(); m_queue.signal_exit_force(); }
        inline void signal_exit_when_done   ()  { m_queue.signal_exit_when_done(); /*when the delayed will be finished will signal the active queue items to exit when done, then the processing pool */ }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_queue.is_exit_force(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();

            // first wait for queue items to finish
            m_queue.wait();

            // only now can signal exit when done for workers (when no more items exists)
            return m_thread_pool.wait();
        }

        // wait some time then signal exit
        template <typename _Rep, typename _Period>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period> &__rtime)
        {
            using __dur    = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_until(std::chrono::system_clock::now() + __reltime);
        }

        // wait until then signal exit
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime)
        {
            signal_exit_when_done();

            // first wait for delayed items to finish
            auto delayed_status = m_queue.wait_until(__atime);
            if (delayed_status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            // only now can signal exit when done for workers  (when no more items exists)
            return m_thread_pool.wait_until(__atime);
        }

    private:
        // some prevention
        jobs_engine(const jobs_engine &)            = delete;
        jobs_engine(jobs_engine &&)                 = delete;
        jobs_engine &operator=(const jobs_engine &) = delete;
        jobs_engine &operator=(jobs_engine &&__t)   = delete;

    private:
        //
        // apply config
        //
        inline void apply_config()
        {
            // setup jobs groups
            for (auto &[jobs_group, jobs_group_config] : m_config.m_groups) {
                m_queue.add_jobs_group(jobs_group, m_config.m_engine.m_config_prio);
                m_thread_pool.add_job_group(jobs_group, jobs_group_config.m_threads_count);
            }

            // setup jobs types
            if (!m_config.m_default_function_on_children_finished) {
                m_config.m_default_function_on_children_finished = std::bind(&jobs_engine::jobs_on_children_finished, this, std::placeholders::_1 /*jobs_items*/);
            }
            if (!m_config.m_default_function_finished) {
                m_config.m_default_function_finished = std::bind(&jobs_engine::jobs_finished, this, std::placeholders::_1 /*jobs_items*/);
            }

            m_config.apply_default_function_processing();
            m_config.apply_default_function_on_children_finished();
            m_config.apply_default_function_finished();

            for (auto &[jobs_type, jobs_type_config] : m_config.m_types) {
                m_queue.add_jobs_type(jobs_type, jobs_type_config.m_group, jobs_type_config.m_timeout);
            }

            // auto start threads if count > 0 otherwise threads should be manually started
            if (m_config.m_engine.m_threads_count) {
                start_threads(m_config.m_engine.m_threads_count);
            }
        }

        //
        // inner thread function for executing items (should return if there are more items)
        //
        friend small::jobsimpl::jobs_engine_thread_pool<JobsGroupT, ThisJobsEngine>;

        inline EnumLock do_action(const JobsGroupT &jobs_group, bool *has_items)
        {
            *has_items = false;

            // get bulk_count
            auto it_cfg_grp = m_config.m_groups.find(jobs_group);
            if (it_cfg_grp == m_config.m_groups.end()) {
                return small::EnumLock::kExit;
            }

            int bulk_count = std::max(it_cfg_grp->second.m_bulk_count, 1);

            // delay request
            typename JobsConfig::ConfigProcessing group_config{};
            group_config.m_delay_next_request = it_cfg_grp->second.m_delay_next_request;

            // get items to process
            auto *q = m_queue.get_group_queue(jobs_group);
            if (!q) {
                return small::EnumLock::kExit;
            }

            std::vector<JobsID> vec_ids;
            auto                ret = q->wait_pop_front_for(std::chrono::nanoseconds(0), vec_ids, bulk_count);
            if (ret != small::EnumLock::kElement) {
                return ret;
            }

            *has_items = true;

            // split by type
            std::unordered_map<JobsTypeT, std::vector<std::shared_ptr<JobsItem>>> elems_by_type;
            {
                // get jobs
                std::vector<std::shared_ptr<JobsItem>> jobs_items = m_queue.jobs_get(vec_ids);
                for (auto &jobs_item : jobs_items) {
                    elems_by_type[jobs_item->m_type].reserve(jobs_items.size());

                    // mark the item as in progress
                    jobs_item->set_state_inprogress();
                    // execute if it is still in progress (may be moved to higher states due to external factors like cancel, timeout, finish early due to other job, etc)
                    if (jobs_item->is_state_inprogress()) {
                        elems_by_type[jobs_item->m_type].push_back(jobs_item);
                    }
                }
            }

            // process specific job by type
            for (auto &[jobs_type, jobs] : elems_by_type) {
                auto it_cfg_type = m_config.m_types.find(jobs_type);
                if (it_cfg_type == m_config.m_types.end()) {
                    continue;
                }

                // TODO if an item has timeout add it to a time queue with callback that marks that item as timeout
                // TODO timeout should be set only if it is not finished/failed/cancelled

                // process specific jobs by type
                typename JobsConfig::ConfigProcessing type_config;
                it_cfg_type->second.m_function_processing(jobs, type_config);

                // get the max for config
                if (!group_config.m_delay_next_request) {
                    group_config.m_delay_next_request = type_config.m_delay_next_request;
                } else {
                    if (type_config.m_delay_next_request) {
                        group_config.m_delay_next_request = std::max(group_config.m_delay_next_request, type_config.m_delay_next_request);
                    }
                }

                // TODO marks the items as either wait for children (if it has children) or finished
                // mark the item as in wait for children of finished
                // if in callback the state is set to failed, cancelled or timeout setting to finish wont succeed because if less value than those
                // jobs_item->set_state(small::EnumJobsState::kInProgress);
                // TODO put in proper thread for processing children and finished work (1/2 thread(s) for each - better to have a config for it?)
                // TODO the worker thread is configured for jobgroup, children and finished are not part of those - a solution is to add a pair or internal_group
            }

            // TODO move this delete on the finished thread
            for (auto &jobs_id : vec_ids) {
                m_queue.jobs_del(jobs_id);
            }

            // TODO  group_config.m_delay_next_request
            // TODO for delay after requests use worker_thread delay item -> check if has_items should be set properly
            // TODO if delay is set set has_items to true to force the sleep, but also a last time sleep so if there too much time and are no items dont continue

            return ret;
        }

        // TODO external set state for a job moves it to proper wait for children or finished

        //
        // inner function for activate the jobs from queue
        //
        friend JobsQueue;

        inline void jobs_activate(const JobsTypeT &jobs_type, const JobsID & /* jobs_id */)
        {
            m_thread_pool.job_start(m_config.m_types[jobs_type].m_group);
        }

        inline void jobs_finished(const std::vector<std::shared_ptr<JobsItem>> &jobs_items)
        {
            // TODO call the custom function from config if exists

            for (auto &jobs_item : jobs_items) {
                m_queue.jobs_del(jobs_item->id);
            }
        }

        inline void jobs_on_children_finished(const std::vector<std::shared_ptr<JobsItem>> &jobs_children)
        {
            // TODO update parent state and progress
            // for (auto &jobs_child : jobs_children) {
            // }
        }

    private:
        //
        // members
        //
        JobsConfig                                                           m_config;
        JobsQueue                                                            m_queue{*this};
        small::jobsimpl::jobs_engine_thread_pool<JobsGroupT, ThisJobsEngine> m_thread_pool{*this}; // for processing items (by group) using a pool of threads
    };
} // namespace small
