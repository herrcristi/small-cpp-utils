#pragma once

#include <atomic>
#include <deque>

#include "base_queue_wait.h"

// a queue with events so we can wait for items to be available
//
// small::lock_queue<int> q;
// ...
// q.push_back( 1 );
// ...
//
// // on some thread
// int e = 0;
// auto ret = q.wait_pop_front( &e ); // ret can be small::EnumLock::kExit, small::EnumLock::kTimeout or ret == small::EnumLock::kElement
// //auto ret = q.wait_pop_front_for( std::chrono::minutes( 1 ), &e );
// if ( ret == small::EnumLock::kElement )
// {
//      // do something with e
//      ...
// }
//
// ...
// // on main thread no more processing (aborting work)
// q.signal_exit_force(); // q.signal_exit_when_done()
//

namespace small {

    //
    // queue which is event controlled
    //
    template <typename T>
    class lock_queue
    {
    public:
        //
        // lock_queue
        //
        lock_queue() = default;
        lock_queue(const lock_queue& o) : lock_queue() { operator=(o); };
        lock_queue(lock_queue&& o) noexcept : lock_queue() { operator=(std::move(o)); };

        inline lock_queue& operator=(const lock_queue& o)
        {
            std::scoped_lock l(m_wait, o.m_wait);
            m_wait  = o.m_wait;
            m_queue = o.m_queue;
            return *this;
        }
        inline lock_queue& operator=(lock_queue&& o) noexcept
        {
            std::scoped_lock l(m_wait, o.m_wait);
            m_wait  = std::move(o.m_wait);
            m_queue = std::move(o.m_queue);
            return *this;
        }

        //
        // size
        //
        inline size_t size()
        {
            std::unique_lock l(m_wait);
            return m_queue.size();
        }

        inline bool empty() { return size() == 0; }

        //
        // clear only removes elements from queue but does not reset the event
        //
        inline void clear()
        {
            std::unique_lock l(m_wait);
            m_queue.clear();
        }

        // clang-format off
        // use it as locker
        inline void lock        () { m_wait.lock(); }
        inline void unlock      () { m_wait.unlock(); }
        inline bool try_lock    () { return m_wait.try_lock(); }
        // clang-format on

        //
        // push_back
        //
        inline std::size_t push_back(const T& elem)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_wait);
            m_queue.push_back(elem);
            m_wait.notify_one();
            return 1;
        }

        inline std::size_t push_back(const std::vector<T>& elems)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_wait);

            std::size_t count = 0;
            for (auto& elem : elems) {
                m_queue.push_back(elem);
                ++count;
            }
            m_wait.notify_all();
            return count;
        }

        // push_back move semantics
        inline std::size_t push_back(T&& elem)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_wait);
            m_queue.push_back(std::forward<T>(elem));
            m_wait.notify_one();
            return 1;
        }

        inline std::size_t push_back(std::vector<T>&& elems)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_wait);

            std::size_t count = 0;
            for (auto& elem : elems) {
                m_queue.push_back(std::forward<T>(elem));
                ++count;
            }
            m_wait.notify_all();
            return count;
        }

        // emplace_back
        template <typename... _Args>
        inline std::size_t emplace_back(_Args&&... __args)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_wait);
            m_queue.emplace_back(std::forward<_Args>(__args)...);
            m_wait.notify_one();
            return 1;
        }

        // clang-format off
        //
        // exit
        //
        inline void signal_exit_force   ()  { m_wait.signal_exit_force(); }
        inline bool is_exit_force       ()  { return m_wait.is_exit_force(); }

        inline void signal_exit_when_done() { m_wait.signal_exit_when_done(); }
        inline bool is_exit_when_done   ()  { return m_wait.is_exit_when_done(); }
        
        inline bool is_exit             ()  { return is_exit_force() || is_exit_when_done(); }
        // clang-format on

        //
        // wait and return that element
        //
        inline EnumLock wait_pop_front(T* elem)
        {
            return m_wait.wait_pop(elem);
        }

        inline EnumLock wait_pop_front(std::vector<T>& vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop(vec_elems, max_count);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period>& __rtime, T* elem)
        {
            return m_wait.wait_pop_for(__rtime, elem);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period>& __rtime, std::vector<T>& vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop_for(__rtime, vec_elems, max_count);
        }

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration>& __atime, T* elem)
        {
            return m_wait.wait_pop_until(__atime, elem);
        }

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration>& __atime, std::vector<T>& vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop_until(__atime, vec_elems, max_count);
        }

        //
        // wait for queue to become empty (and signal_exit_when_done)
        //
        inline EnumLock wait()
        {
            return m_wait.wait();
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period>& __rtime)
        {
            return m_wait.wait_for(__rtime);
        }

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration>& __atime)
        {
            return m_wait.wait_until(__atime);
        }

    private:
        using BaseQueueWait = small::base_queue_wait<T, small::lock_queue<T>>;
        friend BaseQueueWait;

        //
        // check for front element
        //
        inline small::WaitFlags test_and_get(T* elem, typename BaseQueueWait::TimePoint* /* time_wait_until */, bool* is_empty_after_get)
        {
            *is_empty_after_get = true;

            if (is_exit_force()) {
                return small::WaitFlags::kExit_Force;
            }

            // check queue size
            if (m_queue.empty()) {
                if (is_exit_when_done()) {
                    // exit
                    return small::WaitFlags::kExit_When_Done;
                }

                return small::WaitFlags::kWait;
            }

            // get elem
            if (elem) {
                *elem = std::move(m_queue.front());
            }
            m_queue.pop_front();

            *is_empty_after_get = m_queue.empty();

            return small::WaitFlags::kElement;
        }

    private:
        //
        // members
        //
        mutable BaseQueueWait m_wait{*this}; // implements locks & wait
        std::deque<T>         m_queue;       // queue
    };
} // namespace small
