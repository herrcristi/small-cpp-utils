#pragma once

#include <unordered_map>

#include "../worker_thread.h"

namespace small::jobsimpl {

    //
    // helper class for jobs_engine to execute group of jobs (parent caller must implement 'do_action')
    //
    template <typename JobGroupT, typename ParentCallerT>
    class jobs_engine_thread_pool
    {
    public:
        //
        // jobs_engine_thread_pool
        //
        explicit jobs_engine_thread_pool(ParentCallerT &parent_caller)
            : m_parent_caller(parent_caller)
        {
        }

        ~jobs_engine_thread_pool()
        {
            wait();
        }

        // clang-format off
        // size of active items
        inline size_t   size        () { return m_workers.size(); }
        // empty
        inline bool     empty       () { return size() == 0; }
        // clear
        inline void     clear       () { m_workers.clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_engine_thread_pool<T>> m...)
        inline void     lock        () { m_workers.lock(); }
        inline void     unlock      () { m_workers.unlock(); }
        inline bool     try_lock    () { return m_workers.try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            m_workers.start_threads(threads_count);
        }

        //
        // config processing by job group type
        // this should be done in the initial setup phase once
        //
        inline void config_jobs_group(const JobGroupT &job_group, const int &threads_count)
        {
            m_scheduler[job_group].m_threads_count = threads_count;
        }

        //
        // when items are added to be processed in parent class the start scheduler should be called
        // to trigger action (if needed for the new job group)
        //
        inline void jobs_start(const JobGroupT &job_group)
        {
            auto it = m_scheduler.find(job_group); // map is not changed, so can be access without locking
            if (it == m_scheduler.end()) {
                return;
            }

            // even if here it is considered that there are items and something will be scheduled,
            // the actual check if work will still exists will be done in do_action of parent
            auto &stats = it->second;
            jobs_action_start(job_group, true, stats);
        }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { m_workers.signal_exit_force(); }
        inline void signal_exit_when_done   ()  { m_workers.signal_exit_when_done(); }
        inline bool is_exit                 ()  { return m_workers.is_exit(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
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
            return m_workers.wait_until(__atime);
        }

    private:
        // some prevention
        jobs_engine_thread_pool(const jobs_engine_thread_pool &)            = delete;
        jobs_engine_thread_pool(jobs_engine_thread_pool &&)                 = delete;
        jobs_engine_thread_pool &operator=(const jobs_engine_thread_pool &) = delete;
        jobs_engine_thread_pool &operator=(jobs_engine_thread_pool &&__t)   = delete;

    private:
        struct JobGroupStats
        {
            int m_threads_count{};
            int m_running{}; // how many requests are currently running
        };

        //
        // to trigger action (if needed for the new job group)
        //
        inline void jobs_action_start(const JobGroupT &job_group, const bool has_items, JobGroupStats &stats)
        {
            if (!has_items) {
                return;
            }

            std::unique_lock l(*this);

            // move from queue to action
            bool needs_runners = stats.m_running < stats.m_threads_count;
            if (needs_runners) {
                ++stats.m_running;
                m_workers.push_back(job_group);
            }
        }

        //
        // job action ended
        //
        inline void jobs_action_end(const JobGroupT &job_group, const bool has_items)
        {
            auto it = m_scheduler.find(job_group); // map is not changed, so can be access without locking
            if (it == m_scheduler.end()) {
                return;
            }

            std::unique_lock l(*this);

            auto &stats = it->second;
            --stats.m_running;

            jobs_action_start(job_group, has_items, stats);
        }

        //
        // inner thread function for scheduler
        //
        inline void thread_function(const std::vector<JobGroupT> &items)
        {
            for (auto job_group : items) {

                bool has_items = false;
                m_parent_caller.do_action(job_group, &has_items);

                // start another action
                jobs_action_end(job_group, has_items);
            }
        }

    private:
        //
        // members
        //

        //
        // pool of thread workers
        //
        struct JobWorkerThreadFunction
        {
            void operator()(small::worker_thread<JobGroupT> &, const std::vector<JobGroupT> &items, jobs_engine_thread_pool<JobGroupT, ParentCallerT> *pThis) const
            {
                pThis->thread_function(items);
            }
        };

        std::unordered_map<JobGroupT, JobGroupStats> m_scheduler;
        small::worker_thread<JobGroupT>              m_workers{{.threads_count = 0}, JobWorkerThreadFunction(), this};
        ParentCallerT                               &m_parent_caller; // parent jobs engine
    };
} // namespace small::jobsimpl
