#pragma once

#include <unordered_map>

#include "jobs_engine_scheduler.h"
#include "jobs_queue.h"

// // example
// using qc = std::pair<int, std::string>;
//
// enum JobType
// {
//     job1,
//     job2
// };
//
// small::jobs_engine<JobType, qc> jobs(
//     {.threads_count = 0 /*dont start any thread yet*/},  // job engine config
//     {.threads_count = 1, .bulk_count = 1},               // default group config
//     {.group = JobType::job1},                            // default job type config
//     [](auto &j /*this*/, const auto job_type, const auto &items) {
//         for (auto &[i, s] : items) {
//             ...
//         }
//     });
//
// // add specific function for job1
// jobs.add_job_group(JobType::job1, {.threads_count = 2} );
//
// jobs.add_job_type(JobType::job1, {.group = JobType::job1}, [](auto &j /*this*/, const auto job_type, const auto &items, auto b /*extra param b*/) {
//     for(auto &[i, s]:items){
//         ...
//     }
// }, 5 /*param b*/);
//
// // use default config and default function for job2
// jobs.add_job_type(JobType::job2);
// // manual start threads
// jobs.start_threads(3);
//
// // push
// jobs.push_back(JobType::job1, {1, "a"});
// jobs.push_back(JobType::job2, {2, "b"});
//
//
// auto ret = jobs.wait_for(std::chrono::milliseconds(0)); // wait to finished
// ... check ret
// jobs.wait(); // wait here for jobs to finish due to exit flag
//

namespace small {

    // config the entire jobs engine
    template <typename PrioT = EnumPriorities>
    struct config_jobs_engine
    {
        int                             threads_count{8}; // how many total threads for processing
        small::config_prio_queue<PrioT> config_prio{};
    };

    // config an individual job type
    template <typename JobGroupT>
    struct config_job_type
    {
        JobGroupT group{}; // job type group (multiple job types can be configured to same group)
    };

    // config the job group (where job types can be grouped)
    struct config_job_group
    {
        int threads_count{1}; // how many threads for processing (out of the global threads)
        int bulk_count{1};    // how many objects are processed at once
    };

    //
    // small class for jobs
    //
    template <typename JobTypeT, typename JobElemT, typename JobGroupT = JobTypeT, typename PrioT = EnumPriorities>
    class jobs_engine
    {
    public:
        //
        // jobs_engine
        //
        explicit jobs_engine(const config_jobs_engine<PrioT> &config_engine = {})
            : m_config{
                  .m_engine(config_engine)}
        {
            // auto start threads if count > 0 otherwise threads should be manually started
            if (m_config.threads_count) {
                start_threads(m_config.threads_count);
            }
        }

        template <typename _Callable, typename... Args>
        jobs_engine(const config_jobs_engine<PrioT> config_engine, const config_job_group config_default_group, config_job_type<JobTypeT> &config_default_job_type, _Callable function, Args... extra_parameters)
            : m_config{
                  .m_engine{config_engine},
                  .m_default_group{config_default_group},
                  .m_default_job_type{config_default_job_type},
                  .m_default_processing_function{std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*job_type*/, std::placeholders::_2 /*items*/, std::forward<Args>(extra_parameters)...)}}

        {
            if (m_config.threads_count) {
                start_threads(m_config.threads_count);
            }
        }

        ~jobs_engine()
        {
            wait();
        }

        // clang-format off
        // size of active items
        inline size_t   size        () { return m_jobs_queues.size();  }
        // empty
        inline bool     empty       () { return size() == 0; }
        // clear
        inline void     clear       () { m_jobs_queues.clear(); }
        
        
        // size of working items
        inline size_t   size_processing () { return m_scheduler.size();  }
        // empty
        inline bool     empty_processing() { return size_processing() == 0; }
        // clear
        inline void     clear_processing() { m_scheduler.clear(); }

        // size of delayed items
        inline size_t   size_delayed() { return m_delayed_items.queue().size();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { m_delayed_items.queue().clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_engine<T>> m...)
        inline void     lock        () { m_scheduler.lock(); }
        inline void     unlock      () { m_scheduler.unlock(); }
        inline bool     try_lock    () { return m_scheduler.try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            m_config.threads_count = threads_count;
            m_delayed_items.start_threads();
            m_scheduler.start_threads(threads_count);
        }

        //
        // config processing by job type
        // THIS SHOULD BE DONE IN THE INITIAL SETUP PHASE ONCE
        //
        //
        // set default job group, job type config and processing function
        //
        template <typename _Callable, typename... Args>
        inline void add_default_config(const config_job_group &config_default_group, const config_job_type<JobGroupT> &config_default_job_type, _Callable function, Args... extra_parameters)
        {
            m_config.m_default_group               = config_default_group;
            m_config.m_default_job_type            = config_default_job_type;
            m_config.m_default_processing_function = std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*job_type*/, std::placeholders::_2 /*items*/, std::forward<Args>(extra_parameters)...);
        }

        //
        // config groups
        //
        inline void add_job_group(const JobGroupT job_group)
        {
            m_config.m_jobs_groups[job_group] = m_config.m_default_group;
            m_scheduler.add_job_group(job_group, m_config.m_default_group.threads_count);
        }

        inline void add_job_group(const JobGroupT job_group, const config_job_group &config_group)
        {
            m_config.m_jobs_groups[job_group] = config_group;
            m_scheduler.add_job_group(job_group, config_group.threads_count);
        }

        inline void add_job_groups(std::vector<std::pair<JobGroupT, config_job_group>> &job_group_configs)
        {
            for (auto &[job_group, job_config] : job_group_configs) {
                add_job_group(job_group, job_config);
            }
        }

        //
        // config job typpes
        //
        void add_job_type(const JobTypeT job_type)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_config.m_jobs_types[job_type] = {
                .m_config              = m_config.m_default_job_type,
                .m_processing_function = m_config.m_default_processing_function};
        }

        inline void add_job_type(const JobTypeT job_type, const config_job_type<JobGroupT> &config)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_jobs.m_queues[job_type] = {
                .m_config              = config,
                .m_processing_function = m_config.m_processing_function};
        }

        template <typename _Callable, typename... Args>
        inline void add_job_type(const JobTypeT job_type, const config_job_type<JobGroupT> &config, _Callable function, Args... extra_parameters)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_jobs.m_queues[job_type] = {
                .m_config              = config,
                .m_processing_function = std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*job_type*/, std::placeholders::_2 /*items*/, std::forward<Args>(extra_parameters)...)};
        }

        //
        // add items to be processed
        // push_back
        //
        inline std::size_t push_back(const PrioT priority, const JobTypeT job_type, const JobElemT &elem)
        {
            auto ret = m_jobs_queues.push_back(priority, job_type, elem);
            if (ret) {
                m_scheduler.job_start(job_type);
            }
            return ret;
        }

        // push back with move semantics
        inline std::size_t push_back(const PrioT priority, const JobTypeT job_type, T &&t)
        {
            auto ret = m_jobs_queues.push_back(priority, job_type, elem);
            if (ret) {
                m_scheduler.job_start(job_type);
            }
            return ret;
        }

        // emplace_back
        template <typename... _Args>
        inline std::size_t emplace_back(const PrioT priority, const JobTypeT job_type, _Args &&...__args)
        {
            auto ret = m_jobs_queues.emplace_back(priority, job_type, std::forward<_Args>(__args)...);
            if (ret) {
                m_scheduler.job_start(job_type);
            }
            return ret;
        }

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobTypeT job_type, const JobElemT &elem)
        {
            return m_delayed_items.queue().push_delay_for(__rtime, {job_type, elem});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobTypeT job_type, const JobElemT &elem)
        {
            return m_delayed_items.queue().push_delay_until(__atime, {job_type, elem});
        }

        // push_back move semantics
        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobTypeT job_type, JobElemT &&elem)
        {
            return m_delayed_items.queue().push_delay_for(__rtime, {job_type, std::forward<T>(elem)});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobTypeT job_type, JobElemT &&elem)
        {
            return m_delayed_items.queue().push_delay_until(__atime, {job_type, std::forward<T>(elem)});
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline std::size_t emplace_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobTypeT job_type, _Args &&...__args)
        {
            return m_delayed_items.queue().push_delay_for(__rtime, {job_type, T{std::forward<_Args>(__args)...}});
        }

        template </* typename _Clock, typename _Duration, */ typename... _Args> // avoid time_casting from one clock to another
        inline std::size_t emplace_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobTypeT job_type, _Args &&...__args)
        {
            return m_delayed_items.queue().push_delay_until(__atime, {job_type, T{std::forward<_Args>(__args)...}});
        }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { 
            m_scheduler.signal_exit_force(); 
            m_delayed_items.queue().signal_exit_force(); 
            m_jobs_queues.signal_exit_force();
        }
        inline void signal_exit_when_done   ()  { m_delayed_items.queue().signal_exit_when_done(); /*when the delayed will be finished will signal the queue items to exit when done*/ }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_delayed_items.queue().is_exit_force() || m_scheduler.is_exit(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();

            // first wait for delayed items to finish
            m_delayed_items.wait();

            // only now can signal exit when done for all queues (when no more delayed items can be pushed)
            m_jobs_queues.signal_exit_when_done();

            for (auto &[job_type, job_item] : m_jobs.m_queues) {
                std::unique_lock l(job_item.m_queue_items);
                m_jobs.m_queues_exit_condition.wait(l, [q = &job_item.m_queue_items]() -> bool {
                    return q->empty() || q->is_exit_force();
                });
            }

            // only now can signal exit when done for workers (when no more items exists)
            return m_scheduler.wait();
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
            auto delayed_status = m_delayed_items.wait_until(__atime);
            if (delayed_status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            // only now can signal exit when done for all queues (when no more delayed items can be pushed)
            m_jobs_queues.signal_exit_when_done();

            for (auto &[job_type, job_item] : m_jobs.m_queues) {
                std::unique_lock l(job_item.m_queue_items);

                auto status = m_jobs.m_queues_exit_condition.wait_until(l, __atime, [q = &job_item.m_queue_items]() -> bool {
                    return q->empty() || q->is_exit_force();
                });
                if (!status) {
                    return small::EnumLock::kTimeout;
                }
            }

            // only now can signal exit when done for workers  (when no more items exists)
            return m_scheduler.wait_until(__atime);
        }

    private:
        // some prevention
        jobs_engine(const jobs_engine &)            = delete;
        jobs_engine(jobs_engine &&)                 = delete;
        jobs_engine &operator=(const jobs_engine &) = delete;
        jobs_engine &operator=(jobs_engine &&__t)   = delete;

    private:
        //
        // inner thread function for executing items (should return if there are more items)
        //
        inline void do_action(const JobGroupT job_group, bool &has_items)
        {
            std::vector<T> vec_elems;
            T              elem{};

            for (auto job_type : items) {
                // m_job_queues can accessed without locking afterwards because it will not be modified
                auto it = m_jobs.m_queues.find(job_type);
                if (it == m_jobs.m_queues.end()) {
                    continue;
                }

                JobTypeQueueItem &job_item = it->second;

                int  max_count = std::max(job_item.m_config.bulk_count, 1);
                auto ret       = job_item.m_queue_items.wait_pop_front(vec_elems, max_count);
                if (ret == small::EnumLock::kExit) {
                    m_jobs.m_queues_exit_condition.notify_all();
                    return;
                }
                // update global counter
                for (std::size_t i = 0; i < vec_elems.size(); ++i) {
                    --m_jobs.m_total_count; // dec
                }

                // process specific job by type
                job_item.m_processing_function(job_type, vec_elems);

                // start another action
                job_action_end(job_type, job_item);
            }
        }

        //
        // inner thread function for delayed items
        //
        using JobDelayedItems  = std::pair<JobTypeT, JobElemT>;
        using JobQueueDelayedT = small::time_queue_thread<JobDelayedItems, small::jobs_engine<JobTypeT, JobElemT>>;
        friend JobQueueDelayedT;

        inline std::size_t push_back(std::vector<JobDelayedItems> &&items)
        {
            std::size_t count = 0;
            for (auto &item : items) {
                count += push_back(item.first, std::move(item.second));
            }
            return count;
        }

    private:
        //
        // members
        //
        using ProcessingFunction = std::function<void(const JobType job_type, const std::vector<T> &)>;

        struct JobTypeConfig
        {
            config_job_type<JobGroupT> m_config{};              // config for this job type (to which group it belongs)
            ProcessingFunction         m_processing_function{}; // processing Function
        };

        // configs
        struct JobEngineConfig
        {
            config_jobs_engine<PrioT>                       m_engine;                        // config for entire engine (threads, priorities, etc)
            config_job_group                                m_default_group{};               // default config for group
            config_job_type<JobTypeT>                       m_default_job_type{};            // default config for job type
            ProcessingFunction                              m_default_processing_function{}; // default processing function
            std::unordered_map<JobGroupT, config_job_group> m_jobs_groups;                   // config by job group
            std::unordered_map<JobTypeT, JobTypeConfig>     m_jobs_types;                    // config by job type
        };

        // struct JobImpl
        // {

        //     std::condition_variable_any m_queues_exit_condition; // condition to wait for when signal exit when done for queue items
        //
        // };

        JobEngineConfig                                         m_config;               // configs for all: engine, groups, job types
        small::jobs_queue<JobTypeT, JobElemT, JobGroupT, PrioT> m_jobs_queues;          // curent jobs queues (with grouping and priority) for job types
        JobQueueDelayedT                                        m_delayed_items{*this}; // queue of delayed items
        small::jobs_engine_scheduler<JobGroupT>                 m_scheduler{*this};     // scheduler for processing items (by group) using a pool of threads
    };
} // namespace small
