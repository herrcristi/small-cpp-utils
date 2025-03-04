#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <queue>

#include "lock_queue.h"

namespace small {

    //
    // add threads to process items from queue (parent caller must implement 'config' and 'process_items')
    //
    template <typename T, typename ParentCallerT>
    class lock_queue_thread
    {
    public:
        //
        // lock_queue_thread
        //
        explicit lock_queue_thread(ParentCallerT& parent_caller)
            : m_parent_caller(parent_caller)

        {
            // threads must be manually started
        }

        //
        // queue
        //
        inline small::lock_queue<T>& queue()
        {
            return m_lock_queue;
        }

        //
        // start threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            std::unique_lock l(m_lock_queue);

            m_threads_futures.resize(static_cast<std::size_t>(threads_count));
            for (auto& tf : m_threads_futures) {
                if (!tf.valid()) {
                    tf = std::async(std::launch::async, &lock_queue_thread::thread_function, this);
                }
            }
        }

        //
        // wait
        //
        inline EnumLock wait()
        {
            m_lock_queue.signal_exit_when_done();
            for (const auto& th : m_threads_futures) {
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
            m_lock_queue.signal_exit_when_done();

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
        // inner thread function
        //
        inline void thread_function()
        {
            std::vector<T> vec_elems;
            const int      bulk_count = std::max<>(m_parent_caller.config().bulk_count, 1);
            for (; true; small::sleep_micro(1)) {
                // wait
                small::EnumLock ret = m_lock_queue.wait_pop_front(vec_elems, bulk_count);

                if (ret == small::EnumLock::kExit) {
                    // force stop
                    break;
                } else if (ret == small::EnumLock::kTimeout) {
                    // nothing to do
                } else if (ret == small::EnumLock::kElement) {
                    m_parent_caller.process_items(std::move(vec_elems));
                }
            }
        }

    private:
        // some prevention
        lock_queue_thread(const lock_queue_thread&)            = delete;
        lock_queue_thread(lock_queue_thread&&)                 = delete;
        lock_queue_thread& operator=(const lock_queue_thread&) = delete;
        lock_queue_thread& operator=(lock_queue_thread&& __t)  = delete;

    private:
        //
        // members
        //
        small::lock_queue<T>           m_lock_queue;      // a time priority queue for delayed items
        std::vector<std::future<void>> m_threads_futures; // threads futures (needed to wait for)
        ParentCallerT&                 m_parent_caller;   // active queue where to push
    };
} // namespace small
