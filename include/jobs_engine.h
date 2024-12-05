#pragma once

#include <unordered_map>

#include "worker_thread.h"

// using qc = std::pair<int, std::string>;
// ...
// //

namespace small {
    //
    // small class for worker threads
    //

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
              m_default_job_type_config(config_default_job),
              m_default_processing_function(std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*item*/, std::forward<Args>(extra_parameters)...))

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
        // TODO
        // // size of active items
        // inline size_t   size        () { return m_queue_items.size();  }
        // // empty
        // inline bool     empty       () { return size() == 0; }
        // // clear
        // inline void     clear       () { m_queue_items.clear(); }

        // size of delayed items
        inline size_t   size_delayed() { return m_delayed_items.size();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { m_delayed_items.clear(); }
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
            m_workers.start_threads(threads_count);
        }

        //
        // config processing by job type
        // this should be done in the initial setup phase once

        // set default job type config and processing function
        template <typename _Callable, typename... Args>
        void add_default_job_type(const config_job_type config_default_job, _Callable function, Args... extra_parameters)
        {
            m_default_job_type_config     = config_default_job;
            m_default_processing_function = std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*item*/, std::forward<Args>(extra_parameters)...);
        }

        void add_job_type(const JobType job_type)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_job_queues[job_type] = {
                .m_job_type            = job_type,
                .m_config              = m_default_job_type_config,
                .m_processing_function = m_default_processing_function};
        }

        void add_job_type(const JobType job_type, const config_job_type config)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_job_queues[job_type] = {
                .m_job_type            = job_type,
                .m_config              = config,
                .m_processing_function = m_default_processing_function};
        }

        template <typename _Callable, typename... Args>
        void add_job_type(const JobType job_type, const config_job_type config, _Callable function, Args... extra_parameters)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_job_queues[job_type] = {
                .m_job_type            = job_type,
                .m_config              = config,
                .m_processing_function = std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*item*/, std::forward<Args>(extra_parameters)...)};
        }

        //
        // add items to be processed
        // push_back
        //
        inline void push_back(const JobType job_type, const T &t)
        {
            if (is_exit()) {
                return;
            }

            // m_job_queues can accessed without locking afterwards because it will not be modified
            auto it = m_job_queues.find(job_type);
            if (it == m_job_queues.end()) {
                return;
            }

            it->second.m_queue_items.push_back(t);
        }

        // push back with move semantics
        inline void push_back(const JobType job_type, T &&t)
        {
            if (is_exit()) {
                return;
            }

            // m_job_queues can accessed without locking afterwards because it will not be modified
            auto it = m_job_queues.find(job_type);
            if (it == m_job_queues.end()) {
                return;
            }

            it->second.m_queue_items.push_back(std::forward<T>(t));
        }

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobType job_type, const T &elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_for(__rtime, {job_type, elem});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobType job_type, const T &elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_until(__atime, {job_type, elem});
        }

        // push_back move semantics
        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobType job_type, T &&elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_for(__rtime, {job_type, std::forward<T>(elem)});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobType job_type, T &&elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_until(__atime, {job_type, std::forward<T>(elem)});
        }

        // emplace_back
        template <typename... _Args>
        inline void emplace_back(const JobType job_type, _Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            // m_job_queues can accessed without locking afterwards because it will not be modified
            auto it = m_job_queues.find(job_type);
            if (it == m_job_queues.end()) {
                return;
            }

            it->second.m_queue_items.emplace_back(std::forward<_Args>(__args)...);
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline void emplace_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobType job_type, _Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.emplace_delay_for(__rtime, {job_type, std::forward<T>(__args)...});
        }

        template </* typename _Clock, typename _Duration, */ typename... _Args> // avoid time_casting from one clock to another
        inline void emplace_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const JobType job_type, _Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.emplace_delay_until(__atime, {job_type, std::forward<T>(__args)...});
        }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { m_workers.signal_exit_force(); }
        inline void signal_exit_when_done   ()  { m_workers.signal_exit_when_done(); }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_workers.is_exit_force(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();
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
            return m_workers.wait_until(__atime);
        }

    private:
        // some prevention
        jobs_engine(const jobs_engine &) = delete;
        jobs_engine(jobs_engine &&)      = delete;

        jobs_engine &operator=(const jobs_engine &) = delete;
        jobs_engine &operator=(jobs_engine &&__t)   = delete;

    private:
        //
        // inner thread function for active items
        //
        inline void thread_function(const std::vector<JobType> &items)
        {
            // std::vector<T> vec_elems;
            // const int      bulk_count = std::max(m_config.bulk_count, 1);
            // while (true) {
            //     // wait
            //     small::EnumLock ret = m_queue_items.wait_pop_front(vec_elems, bulk_count);

            //     if (ret == small::EnumLock::kExit) {
            //         // force stop
            //         break;
            //     } else if (ret == small::EnumLock::kTimeout) {
            //         // nothing to do
            //     } else if (ret == small::EnumLock::kElement) {
            //         // process
            //         m_processing_function(vec_elems); // bind the std::placeholders::_1
            //     }
            //     small::sleepMicro(1);
            // }
        }

        //
        // inner thread function for delayed items
        //
        inline void thread_function_delayed()
        {
            // std::vector<T> vec_elems;
            // const int      bulk_count = std::max(m_config.bulk_count, 1);
            // while (true) {
            //     // wait
            //     small::EnumLock ret = m_delayed_items.wait_pop(vec_elems, bulk_count);

            //     if (ret == small::EnumLock::kExit) {
            //         m_queue_items.signal_exit_when_done();
            //         // force stop
            //         break;
            //     } else if (ret == small::EnumLock::kTimeout) {
            //         // nothing to do
            //     } else if (ret == small::EnumLock::kElement) {
            //         // put them to active items queue // TODO add support for push vec
            //         for (auto &elem : vec_elems) {
            //             push_back(std::move(elem));
            //         }
            //     }
            //     small::sleepMicro(1);
            // }
        }

    private:
        //
        // members
        //
        using ProcessingFunction = std::function<void(const std::vector<T> &)>;

        struct JobTypeQueueItem
        {
            JobType              m_job_type{};            // job type
            config_job_type      m_config{};              // config for this job type
            ProcessingFunction   m_processing_function{}; // processing Function
            small::lock_queue<T> m_queue_items{};         // queue of items
        };

        struct JobWorkerThreadFunction
        {
            void operator()(small::worker_thread<JobType> &, const std::vector<JobType> &items, small::jobs_engine<JobType, T> *pThis) const
            {
                pThis->thread_function(items);
            }
        };

        config_jobs_engine                            m_config;                                                         // config
        small::worker_thread<JobType>                 m_workers{{.threads_count = 0}, JobWorkerThreadFunction(), this}; // pool of thread workers
        std::unordered_map<JobType, JobTypeQueueItem> m_job_queues;                                                     // queue of items by type
        config_job_type                               m_default_job_type_config;                                        // default config for a job type
        ProcessingFunction                            m_default_processing_function{};                                  // default processing function
        small::time_queue<std::pair<JobType, T>>      m_delayed_items{};                                                // queue of delayed items
    }; // namespace small
} // namespace small
