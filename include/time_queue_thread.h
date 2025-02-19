#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <queue>

#include "time_queue.h"
#include "util_time.h"

namespace small {

    //
    // on separate thread when items from time queue become accessible
    // they are pushed to active queue (parent caller must implement 'push_back')
    //
    template <typename T, typename ParentCallerT>
    class time_queue_thread
    {
    public:
        //
        // time_queue_thread
        //
        explicit time_queue_thread(ParentCallerT& parent_caller)
            : m_parent_caller(parent_caller)
        {
            // threads must be manually started
        }

        //
        // queue
        //
        inline small::time_queue<T>& queue()
        {
            return m_time_queue;
        }

        //
        // start threads
        //
        inline void start_threads()
        {
            std::unique_lock l(m_time_queue);

            const int threads_count = 1;
            m_threads_futures.resize(threads_count);
            for (auto& tf : m_threads_futures) {
                if (!tf.valid()) {
                    tf = std::async(std::launch::async, &time_queue_thread::thread_function, this);
                }
            }
        }

        //
        // wait
        //
        inline EnumLock wait()
        {
            m_time_queue.signal_exit_when_done();
            for (auto& th : m_threads_futures) {
                th.wait();
            }
            return small::EnumLock::kExit;
        }

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

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration>& __atime)
        {
            m_time_queue.signal_exit_when_done();

            for (auto& th : m_threads_futures) {
                auto ret = th.wait_until(__atime);
                if (ret == std::future_status::timeout) {
                    return small::EnumLock::kTimeout;
                }
            }

            return small::EnumLock::kExit;
        }

    private:
        //
        // inner thread function for delayed items
        //
        inline void thread_function()
        {
            std::vector<T> vec_elems;
            const int      bulk_count = 1;
            for (; true; small::sleepMicro(1)) {
                // wait
                small::EnumLock ret = m_time_queue.wait_pop(vec_elems, bulk_count);

                if (ret == small::EnumLock::kExit) {
                    // force stop
                    break;
                } else if (ret == small::EnumLock::kTimeout) {
                    // nothing to do
                } else if (ret == small::EnumLock::kElement) {
                    // put them to active items queue
                    m_parent_caller.push_back(std::move(vec_elems));
                }
            }
        }

    private:
        // some prevention
        time_queue_thread(const time_queue_thread&)            = delete;
        time_queue_thread(time_queue_thread&&)                 = delete;
        time_queue_thread& operator=(const time_queue_thread&) = delete;
        time_queue_thread& operator=(time_queue_thread&& __t)  = delete;

    private:
        //
        // members
        //
        small::time_queue<T>           m_time_queue;      // a time priority queue for delayed items
        std::vector<std::future<void>> m_threads_futures; // threads futures (needed to wait for)
        ParentCallerT&                 m_parent_caller;   // active queue where to push
    };
} // namespace small
