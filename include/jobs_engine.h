#pragma once

#include <unordered_map>

#include "jobs_queue.h"
#include "worker_thread.h"

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
//     {.threads_count = 0 /*dont start any thread yet*/},
//     {.threads_count = 1, .bulk_count = 1},
//     [](auto &j /*this*/, const auto job_type, const auto &items) {
//         for (auto &[i, s] : items) {
//             ...
//         }
//     });
//
// // add specific function for job1
// jobs.add_job_type(JobType::job1, {.threads_count = 2}, [](auto &j /*this*/, const auto job_type, const auto &items, auto b /*extra param b*/) {
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
    struct config_jobs_engine
    {
        int threads_count{1}; // how many threads for processing
    };

    // config an individual job type
    struct config_job_type
    {
        int threads_count{1}; // how many threads for processing (out of the global threads)
        int bulk_count{1};    // how many objects are processed at once
    };

    //
    // small class for jobs
    //
    template <typename JobType, typename T>
    class jobs_engine
    {
    public:
        //
        // jobs_engine
        //
        explicit jobs_engine(const config_jobs_engine config_engine)
            : m_config(config_engine)
        {
            // auto start threads if count > 0 otherwise threads should be manually started
            if (m_config.threads_count) {
                start_threads(m_config.threads_count);
            }
        }

        template <typename _Callable, typename... Args>
        jobs_engine(const config_jobs_engine config_engine, const config_job_type config_default_job, _Callable function, Args... extra_parameters)
            : m_config(config_engine),
              m_default{
                  .m_config{config_default_job},
                  .m_processing_function{std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*job_type*/, std::placeholders::_2 /*items*/, std::forward<Args>(extra_parameters)...)}}

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
        inline size_t   size        () { return m_jobs.m_total_count.load();  }
        // empty
        inline bool     empty       () { return size() == 0; }
        // clear
        inline void     clear       () 
        { 
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            for(auto &[job_type, job_item]: m_jobs.m_queues) {
                job_item.m_queue_items.clear(); // has its own mutex
            }
        }
        
        
        // size of working items
        inline size_t   size_processing () { return m_workers.size();  }
        // empty
        inline bool     empty_processing() { return size_processing() == 0; }
        // clear
        inline void     clear_processing() { m_workers.clear(); }

        // size of delayed items
        inline size_t   size_delayed() { return m_delayed_items.queue().size();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { m_delayed_items.queue().clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_engine<T>> m...)
        inline void     lock        () { m_workers.lock(); }
        inline void     unlock      () { m_workers.unlock(); }
        inline bool     try_lock    () { return m_workers.try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            m_config.threads_count = threads_count;
            m_delayed_items.start_threads();
            m_workers.start_threads(threads_count);
        }

        //
        // config processing by job type
        // this should be done in the initial setup phase once

        // set default job type config and processing function
        template <typename _Callable, typename... Args>
        void add_default_job_type(const config_job_type config_default_job, _Callable function, Args... extra_parameters)
        {
            // avoid config if engine has started to take items
            if (m_jobs.m_started) {
                return;
            }
            m_default.m_config              = config_default_job;
            m_default.m_processing_function = std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*job_type*/, std::placeholders::_2 /*items*/, std::forward<Args>(extra_parameters)...);
        }

        void add_job_type(const JobType job_type)
        {
            // avoid config if engine has started to take items
            if (m_jobs.m_started) {
                return;
            }

            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_jobs.m_queues[job_type] = {
                .m_config              = m_default.m_config,
                .m_processing_function = m_default.m_processing_function};
        }

        void add_job_type(const JobType job_type, const config_job_type config)
        {
            // avoid config if engine has started to take items
            if (m_jobs.m_started) {
                return;
            }

            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_jobs.m_queues[job_type] = {
                .m_config              = config,
                .m_processing_function = m_default.m_processing_function};
        }

        template <typename _Callable, typename... Args>
        void add_job_type(const JobType job_type, const config_job_type config, _Callable function, Args... extra_parameters)
        {
            // avoid config if engine has started to take items
            if (m_jobs.m_started) {
                return;
            }

            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_jobs.m_queues[job_type] = {
                .m_config              = config,
                .m_processing_function = std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*job_type*/, std::placeholders::_2 /*items*/, std::forward<Args>(extra_parameters)...)};
        }

        //
        // add items to be processed
        // push_back
        // TODO add std::pair<T>
        // TODO add std::vector<T>, etc
        inline std::size_t push_back(const JobType job_type, const T &t)
        {
            // m_job_queues can accessed without locking afterwards because it will not be modified
            auto it = m_jobs.m_queues.find(job_type);
            if (it == m_jobs.m_queues.end()) {
                return 0;
            }

            m_jobs.m_started = true; // mark as started to avoid config later
            auto &job_item   = it->second;

            if (job_item.m_queue_items.is_exit()) {
                return 0;
            }

            std::unique_lock l(job_item.m_queue_items);
            ++m_jobs.m_total_count;                           // inc before adding
            auto count = job_item.m_queue_items.push_back(t); // TODO count
            job_action_start(job_type, job_item);
            return count;
        }

        // push back with move semantics
        inline std::size_t push_back(const JobType job_type, T &&t)
        {
            // m_job_queues can accessed without locking afterwards because it will not be modified
            auto it = m_jobs.m_queues.find(job_type);
            if (it == m_jobs.m_queues.end()) {
                return 0;
            }

            m_jobs.m_started = true; // mark as started to avoid config later
            auto &job_item   = it->second;

            if (job_item.m_queue_items.is_exit()) {
                return 0;
            }

            std::unique_lock l(job_item.m_queue_items);
            ++m_jobs.m_total_count;                                            // inc before adding
            auto count = job_item.m_queue_items.push_back(std::forward<T>(t)); // TODO inc above or decrease
            job_action_start(job_type, job_item);
            return count;
        }
        // emplace_back
        template <typename... _Args>
        inline std::size_t emplace_back(const JobType job_type, _Args &&...__args)
        {
            // m_job_queues can accessed without locking afterwards because it will not be modified
            auto it = m_jobs.m_queues.find(job_type);
            if (it == m_jobs.m_queues.end()) {
                return 0;
            }

            m_jobs.m_started = true; // mark as started to avoid config later
            auto &job_item   = it->second;

            if (job_item.m_queue_items.is_exit()) {
                return 0;
            }

            std::unique_lock l(job_item.m_queue_items);
            ++m_jobs.m_total_count;                                                           // inc before adding
            auto count = job_item.m_queue_items.emplace_back(std::forward<_Args>(__args)...); // TODO count
            job_action_start(job_type, job_item);
            return count;
        }

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobType job_type, const T &elem)
        {
            return m_delayed_items.queue().push_delay_for(__rtime, {job_type, elem});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobType job_type, const T &elem)
        {
            return m_delayed_items.queue().push_delay_until(__atime, {job_type, elem});
        }

        // push_back move semantics
        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobType job_type, T &&elem)
        {
            return m_delayed_items.queue().push_delay_for(__rtime, {job_type, std::forward<T>(elem)});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobType job_type, T &&elem)
        {
            return m_delayed_items.queue().push_delay_until(__atime, {job_type, std::forward<T>(elem)});
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline std::size_t emplace_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobType job_type, _Args &&...__args)
        {
            return m_delayed_items.queue().push_delay_for(__rtime, {job_type, T{std::forward<_Args>(__args)...}});
        }

        template </* typename _Clock, typename _Duration, */ typename... _Args> // avoid time_casting from one clock to another
        inline std::size_t emplace_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobType job_type, _Args &&...__args)
        {
            return m_delayed_items.queue().push_delay_until(__atime, {job_type, T{std::forward<_Args>(__args)...}});
        }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { 
            m_workers.signal_exit_force(); 
            m_delayed_items.queue().signal_exit_force(); 
            for(auto &[job_type, job_item]: m_jobs.m_queues) {
                job_item.m_queue_items.signal_exit_force();
            }
        }
        inline void signal_exit_when_done   ()  { m_delayed_items.queue().signal_exit_when_done(); /*when the delayed will be finished will signal the queue items to exit when done*/ }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_workers.is_exit() || m_delayed_items.queue().is_exit_force(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();

            m_delayed_items.wait();

            // only now can signal exit when done for queue items (when no more delayed items can be pushed)
            for (auto &[job_type, job_item] : m_jobs.m_queues) {
                job_item.m_queue_items.signal_exit_when_done();
            }

            for (auto &[job_type, job_item] : m_jobs.m_queues) {
                std::unique_lock l(job_item.m_queue_items);
                m_jobs.m_queues_exit_condition.wait(l, [q = &job_item.m_queue_items]() -> bool {
                    return q->empty() || q->is_exit_force();
                });
            }

            // only now can signal exit when done for workers  (when no more items exists)
            return m_workers.wait();
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

            auto delayed_status = m_delayed_items.wait_until(__atime);
            if (delayed_status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            // only now can signal exit when done for queue items (when no more delayed items can be pushed)
            for (auto &[job_type, job_item] : m_jobs.m_queues) {
                job_item.m_queue_items.signal_exit_when_done();
            }

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
            return m_workers.wait_until(__atime);
        }

    private:
        // some prevention
        jobs_engine(const jobs_engine &)            = delete;
        jobs_engine(jobs_engine &&)                 = delete;
        jobs_engine &operator=(const jobs_engine &) = delete;
        jobs_engine &operator=(jobs_engine &&__t)   = delete;

    private:
        //
        // trigger action (if needed for the new job type)
        //
        struct JobTypeQueueItem;
        inline void job_action_start(const JobType job_type, JobTypeQueueItem &job_item)
        {
            std::unique_lock l(job_item.m_queue_items);

            bool has_items     = job_item.m_queue_items.size() > 0;
            bool needs_runners = job_item.m_processing_stats.m_running < job_item.m_config.threads_count;

            // move from queue to action
            if (has_items && needs_runners) {
                ++job_item.m_processing_stats.m_running;
                m_workers.push_back(job_type);
            }

            if (!has_items) {
                m_jobs.m_queues_exit_condition.notify_all();
            }
        }

        inline void job_action_end(const JobType job_type, JobTypeQueueItem &job_item)
        {
            std::unique_lock l(job_item.m_queue_items);

            --job_item.m_processing_stats.m_running;
            job_action_start(job_type, job_item);
        }

        //
        // inner thread function for active items
        //
        inline void thread_function(const std::vector<JobType> &items)
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
        using JobDelayedItems = std::pair<JobType, T>;
        using JobDelayedT     = small::time_queue_thread<JobDelayedItems, small::jobs_engine<JobType, T>>;
        friend JobDelayedT;

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

        struct JobTypeDefault
        {
            config_job_type    m_config{};              // config default config
            ProcessingFunction m_processing_function{}; // default processing Function
        };

        struct JobTypeStats
        {
            int m_running{}; // how many requests are currently running
        };

        struct JobTypeQueueItem
        {
            config_job_type      m_config{};              // config for this job type
            JobTypeStats         m_processing_stats;      // keep track of execution stats to help schedule
            ProcessingFunction   m_processing_function{}; // processing Function
            small::lock_queue<T> m_queue_items{};         // queue of items
        };

        struct JobQueues
        {
            std::unordered_map<JobType, JobTypeQueueItem> m_queues;                // queue of items by type
            std::condition_variable_any                   m_queues_exit_condition; // condition to wait for when signal exit when done for queue items
            std::atomic<bool>                             m_started{};             // when this flag is set no more config is possible
            std::atomic<long long>                        m_total_count;           // count of all jobs types
        };

        config_jobs_engine m_config;               // config
        JobTypeDefault     m_default;              // default config and default processing function
        JobQueues          m_jobs;                 // curent queues by type
        JobDelayedT        m_delayed_items{*this}; // queue of delayed items

        //
        // pool of thread workers
        //
        struct JobWorkerThreadFunction
        {
            void operator()(small::worker_thread<JobType> &, const std::vector<JobType> &items, small::jobs_engine<JobType, T> *pThis) const
            {
                pThis->thread_function(items);
            }
        };

        small::worker_thread<JobType> m_workers{{.threads_count = 0}, JobWorkerThreadFunction(), this};
    }; // namespace small
} // namespace small
