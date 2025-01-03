#pragma once

#include <unordered_map>

#include "jobs_config.h"
#include "jobs_engine_thread_pool.h"
#include "jobs_item.h"
#include "jobs_queue.h"

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
// using JobsRequest = std::pair<int, std::string>;
// using JobsResponse = int;
// using JobsEng = small::jobs_engine<JobsType, JobsRequest, JobsResponse, JobsGroupType>;
//
// JobsEng jobs(
//     {.threads_count = 0 /*dont start any thread yet*/}, // overall config with default priorities
//     {.threads_count = 1, .bulk_count = 1},              // default jobs group config
//     {.group = JobsGroupType::kJobsGroup1},              // default jobs type config
//     [](auto &j /*this*/, const auto &items) {
//         for (auto &item : items) {
//             ...
//         }
//     });
//
// jobs.add_jobs_group(JobsGroupType::kJobsGroup1, {.threads_count = 1});
//
// // add specific function for job1
// jobs.add_jobs_type(JobsType::kJobsType1, {.group = JobsGroupType::kJobsGroup1}, [](auto &j /*this*/, const auto &items, auto b /*extra param b*/) {
//    for (auto &item : items) {
//      ...
//    }
// }, 5 /*param b*/);
//
// // use default config and default function for job2
// jobs.add_jobs_type(JobsType::kJobsType2);
//
// JobsEng::JobsID              jobs_id{};
// std::vector<JobsEng::JobsID> jobs_ids;
//
// // push
// jobs.push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, {1, "normal"}, &jobs_id);
// jobs.push_back(small::EnumPriorities::kHigh, {.type = JobsType::kJobsType1, .request = {4, "high"}}, &jobs_id);
//
// std::vector<JobsEng::JobsItem> jobs_items = {{.type = JobsType::kJobsType1, .request = {7, "highest"}}};
// jobs.push_back(small::EnumPriorities::kHighest, jobs_items, &jobs_ids);
//
// jobs.push_back_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {100, "delay normal"}, &jobs_id);
//
// jobs.start_threads(3); // manual start threads
//
// // jobs.signal_exit_force();
// auto ret = jobs.wait_for(std::chrono::milliseconds(100)); // wait to finished
// ...
// jobs.wait(); // wait here for jobs to finish due to exit flag
//

namespace small {

    //
    // small class for jobs where job items have a type (which are further grouped by group), priority, request and response
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename JobsGroupT = JobsTypeT, typename JobsPrioT = EnumPriorities>
    class jobs_engine
    {
    public:
        using JobsConfig         = small::jobs_config<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        using JobsItem           = small::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;
        using JobsQueue          = small::jobs_queue<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        using JobsID             = typename JobsItem::JobsID;
        using TimeClock          = typename JobsQueue::TimeClock;
        using TimeDuration       = typename JobsQueue::TimeDuration;
        using ProcessingFunction = typename JobsConfig::ProcessingFunction;

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

        // processing function
        template <typename _Callable, typename... Args>
        inline void add_default_processing_function(_Callable processing_function, Args... extra_parameters)
        {
            m_config.add_default_processing_function(std::bind(std::forward<_Callable>(processing_function), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void add_job_processing_function(const JobsTypeT &jobs_type, _Callable processing_function, Args... extra_parameters)
        {
            m_config.add_job_processing_function(jobs_type, std::bind(std::forward<_Callable>(processing_function), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::forward<Args>(extra_parameters)...));
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
            m_config.apply_default_processing_function();
            for (auto &[jobs_type, jobs_type_config] : m_config.m_types) {
                m_queue.add_jobs_type(jobs_type, jobs_type_config.m_group);
            }

            // auto start threads if count > 0 otherwise threads should be manually started
            if (m_config.m_engine.m_threads_count) {
                start_threads(m_config.m_engine.m_threads_count);
            }
        }

        //
        // inner thread function for executing items (should return if there are more items)
        //
        using ThisJobsEngine = small::jobs_engine<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        friend small::jobs_engine_thread_pool<JobsGroupT, ThisJobsEngine>;

        inline EnumLock do_action(const JobsGroupT &jobs_group, bool *has_items)
        {
            *has_items = false;

            // get bulk_count
            auto it_cfg_grp = m_config.m_groups.find(jobs_group);
            if (it_cfg_grp == m_config.m_groups.end()) {
                return small::EnumLock::kExit;
            }

            int bulk_count = std::max(it_cfg_grp->second.m_bulk_count, 1);

            // get items to process
            auto *q = m_queue.get_group_queue(jobs_group);
            if (!q) {
                return small::EnumLock::kExit;
            }

            std::vector<JobsID> vec_ids;
            auto                ret = q->wait_pop_front_for(std::chrono::nanoseconds(0), vec_ids, bulk_count);
            if (ret == small::EnumLock::kElement) {
                *has_items = true;

                // get jobs
                std::vector<JobsItem *> jobs_items = m_queue.jobs_get(vec_ids);

                // split by type
                std::unordered_map<JobsTypeT, std::vector<JobsItem *>> elems_by_type;
                for (auto &jobs_item : jobs_items) {
                    elems_by_type[jobs_item->type].reserve(jobs_items.size());
                    elems_by_type[jobs_item->type].push_back(jobs_item);
                }

                // process specific job by type
                for (auto &[jobs_type, jobs] : elems_by_type) {
                    auto it_cfg_type = m_config.m_types.find(jobs_type);
                    if (it_cfg_type == m_config.m_types.end()) {
                        continue;
                    }

                    // process specific jobs by type
                    if (it_cfg_type->second.m_processing_function) {
                        it_cfg_type->second.m_processing_function(std::move(jobs));
                    }
                }

                for (auto &jobs_id : vec_ids) {
                    m_queue.jobs_del(jobs_id);
                }
            }

            return ret;
        }

        // activate the jobs
        // inline std::size_t jobs_activate(const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsID &jobs_id)
        // {
        //     std::size_t ret = 0;

        //     // optimization to get the queue from the type
        //     // (instead of getting the group from type from m_config.m_types and then getting the queue from the m_groups_queues)
        //     auto it_q = m_types_queues.find(jobs_type);
        //     if (it_q != m_types_queues.end()) {
        //         auto *q = it_q->second;
        //         ret     = q->push_back(priority, jobs_id);
        //     }

        //     if (ret) {
        //         m_thread_pool.job_start(m_config.m_types[jobs_type].m_config.group);
        //     } else {
        //         jobs_del(jobs_id);
        //     }
        //     return ret;
        // }

        //
        // inner thread function for delayed items
        //

    private:
        //
        // members
        //
        small::jobs_config<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT> m_config;
        small::jobs_queue<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>  m_queue;
        small::jobs_engine_thread_pool<JobsGroupT, ThisJobsEngine>                        m_thread_pool{*this}; // for processing items (by group) using a pool of threads
    };
} // namespace small
