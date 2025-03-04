#pragma once

#include "impl_common.h"

#include "../worker_thread.h"

namespace small::jobsimpl {

    //
    // helper class for jobs_engine to execute group of jobs (parent caller must implement 'do_action')
    //
    template <typename JobGroupT, typename ParentCallerT>
    class jobs_thread_pool
    {
    public:
        //
        // jobs_thread_pool
        //
        explicit jobs_thread_pool(ParentCallerT& parent_caller)
            : m_parent_caller(parent_caller)
        {
        }

        ~jobs_thread_pool()
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
        // use it as locker (std::unique_lock<small:jobs_thread_pool<T>> m...)
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
        inline void config_jobs_group(const JobGroupT& job_group, const int& threads_count)
        {
            m_scheduler[job_group].m_threads_count = threads_count;
        }

        //
        // when items are added to be processed in parent class the start scheduler should be called
        // to trigger action (if needed for the new job group)
        //
        inline void jobs_schedule(const JobGroupT& job_group)
        {
            auto it = m_scheduler.find(job_group); // map is not changed, so can be access without locking
            if (it == m_scheduler.end()) {
                return;
            }

            // even if here it is considered that there are items and something will be scheduled,
            // the actual check if work will still exists will be done in do_action of parent
            auto& stats = it->second;
            jobs_action_start(job_group, true /*has items*/, std::chrono::milliseconds(0) /*delay*/, stats);
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
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period>& __rtime)
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
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration>& __atime)
        {
            return m_workers.wait_until(__atime);
        }

    private:
        // some prevention
        jobs_thread_pool(const jobs_thread_pool&)            = delete;
        jobs_thread_pool(jobs_thread_pool&&)                 = delete;
        jobs_thread_pool& operator=(const jobs_thread_pool&) = delete;
        jobs_thread_pool& operator=(jobs_thread_pool&& __t)  = delete;

    private:
        struct JobGroupStats
        {
            int m_threads_count{};
            int m_running{}; // how many requests are currently running
        };

        //
        // to trigger action (if needed for the new job group)
        //
        inline void jobs_action_start(const JobGroupT& job_group, const bool has_items, const std::chrono::milliseconds& delay_next_request, JobGroupStats& stats)
        {
            if (!has_items) {
                return;
            }

            std::unique_lock l(*this);

            // move from queue to action
            bool needs_runners = stats.m_running < stats.m_threads_count;
            if (needs_runners) {
                ++stats.m_running;
                if (delay_next_request.count() > 0) {
                    m_workers.push_back_delay_for(delay_next_request, job_group);
                } else {
                    m_workers.push_back(job_group);
                }
            }
        }

        //
        // job action ended
        //
        inline void jobs_action_end(const JobGroupT& job_group, const bool has_items, const std::chrono::milliseconds& delay_next_request)
        {
            auto it = m_scheduler.find(job_group); // map is not changed, so can be access without locking
            if (it == m_scheduler.end()) {
                return;
            }

            std::unique_lock l(*this);

            auto& stats = it->second;
            --stats.m_running;

            jobs_action_start(job_group, has_items, delay_next_request, stats);
        }

        //
        // inner thread function for scheduler
        //
        inline void thread_function(const std::vector<JobGroupT>& items)
        {
            for (auto job_group : items) {

                std::chrono::milliseconds delay_next_request{};

                auto ret       = m_parent_caller.do_action(job_group, delay_next_request);
                bool has_items = ret == small::EnumLock::kElement;

                // has items does not mean that there are still items in the queue
                // it means that items were processed in this iteration
                // as such an execution will be scheduled no matter what
                // and next processing time will check to see if there are items or not

                // for delay is important items -> delay -> next item -> delay ....
                //                                       -> empty ... when new items arrive delay was already processed

                // start another action
                jobs_action_end(job_group, has_items, delay_next_request);
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
            void operator()(small::worker_thread<JobGroupT>&, const std::vector<JobGroupT>& items, jobs_thread_pool<JobGroupT, ParentCallerT>* pThis) const
            {
                pThis->thread_function(items);
            }
        };

        std::unordered_map<JobGroupT, JobGroupStats> m_scheduler;
        small::worker_thread<JobGroupT>              m_workers{{.threads_count = 0}, JobWorkerThreadFunction(), this};
        ParentCallerT&                               m_parent_caller; // parent jobs engine
    };
} // namespace small::jobsimpl
