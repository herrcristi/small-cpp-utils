#pragma once

#include <atomic>
#include <deque>
#include <queue>

#include "base_queue_wait.h"

// a queue with time events so we can wait for items to be available at specific time moments
//
// small::time_queue<int> q;
// ...
// q.push_delay_for( std::chrono::seconds(1), 1 );
// ...
//
// // on some thread
// int e = 0;
// auto ret = q.wait_pop( &e ); // ret can be small::EnumLock::kExit, small::EnumLock::kTimeout or ret == small::EnumLock::kElement
// //auto ret = q.wait_pop_for( std::chrono::minutes( 1 ), &e );
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
    // queue which have elements time controlled
    //
    template <typename T>
    class time_queue
    {
    public:
        using BaseQueueWait = small::base_queue_wait<T, small::time_queue<T>>;
        using TimeClock     = typename BaseQueueWait::TimeClock;
        using TimeDuration  = typename BaseQueueWait::TimeDuration;
        using TimePoint     = typename BaseQueueWait::TimePoint;

        //
        // time_queue
        //
        time_queue() = default;
        time_queue(const time_queue& o) : time_queue() { operator=(o); };
        time_queue(time_queue&& o) noexcept : time_queue() { operator=(std::move(o)); };

        inline time_queue& operator=(const time_queue& o)
        {
            std::scoped_lock l(m_wait, o.m_wait);
            m_wait  = o.m_wait;
            m_queue = o.m_queue;
            return *this;
        }
        inline time_queue& operator=(time_queue&& o) noexcept
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
        // clear only removes elements from queue
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
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline std::size_t push_delay_for(const std::chrono::duration<_Rep, _Period>& __rtime, const T& elem)
        {
            using __dur    = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return push_delay_until(TimeClock::now() + __reltime, elem);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_delay_until(const std::chrono::time_point<TimeClock, TimeDuration>& __atime, const T& elem)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock  l(m_wait);
            auto_notification n(this);
            m_queue.push({__atime, elem});
            return 1;
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_delay_for(const std::chrono::duration<_Rep, _Period>& __rtime, const std::vector<T>& elems)
        {
            using __dur    = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return push_delay_until(TimeClock::now() + __reltime, elems);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_delay_until(const std::chrono::time_point<TimeClock, TimeDuration>& __atime, const std::vector<T>& elems)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock  l(m_wait);
            auto_notification n(this);

            std::size_t count = 0;
            for (auto& elem : elems) {
                m_queue.push({__atime, elem});
                ++count;
            }
            return count;
        }

        // push_back move semantics
        template <typename _Rep, typename _Period>
        inline std::size_t push_delay_for(const std::chrono::duration<_Rep, _Period>& __rtime, T&& elem)
        {
            using __dur    = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return push_delay_until(TimeClock::now() + __reltime, std::forward<T>(elem));
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_delay_until(const std::chrono::time_point<TimeClock, TimeDuration>& __atime, T&& elem)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock  l(m_wait);
            auto_notification n(this);
            m_queue.push({__atime, std::forward<T>(elem)});
            return 1;
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_delay_for(const std::chrono::duration<_Rep, _Period>& __rtime, std::vector<T>&& elems)
        {
            using __dur    = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return push_delay_until(TimeClock::now() + __reltime, std::forward<std::vector<T>>(elems));
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_delay_until(const std::chrono::time_point<TimeClock, TimeDuration>& __atime, std::vector<T>&& elems)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock  l(m_wait);
            auto_notification n(this);

            std::size_t count = 0;
            for (auto& elem : elems) {
                m_queue.push({__atime, std::forward<T>(elem)});
                ++count;
            }
            return count;
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline std::size_t emplace_delay_for(const std::chrono::duration<_Rep, _Period>& __rtime, _Args&&... __args)
        {
            using __dur    = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return emplace_delay_until(TimeClock::now() + __reltime, std::forward<_Args>(__args)...);
        }

        template </* typename _Clock, typename _Duration, */ typename... _Args> // avoid time_casting from one clock to another
        inline std::size_t emplace_delay_until(const std::chrono::time_point<TimeClock, TimeDuration>& __atime, _Args&&... __args)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock  l(m_wait);
            auto_notification n(this);
            m_queue.emplace(__atime, T(std::forward<_Args>(__args)...));
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
        // wait pop and return that element
        //
        inline EnumLock wait_pop(T* elem)
        {
            return m_wait.wait_pop(elem);
        }

        inline EnumLock wait_pop(std::vector<T>& vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop(vec_elems, max_count);
        }

        // wait pop_for and return that element
        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_for(const std::chrono::duration<_Rep, _Period>& __rtime, T* elem)
        {
            return m_wait.wait_pop_for(__rtime, elem);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_for(const std::chrono::duration<_Rep, _Period>& __rtime, std::vector<T>& vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop_for(__rtime, vec_elems, max_count);
        }

        // wait until
        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline EnumLock wait_pop_until(const std::chrono::time_point<TimeClock, TimeDuration>& __atime, T* elem)
        {
            return m_wait.wait_pop_until(__atime, elem);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline EnumLock wait_pop_until(const std::chrono::time_point<TimeClock, TimeDuration>& __atime, std::vector<T>& vec_elems, int max_count = 1)
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

    protected:
        //
        // compute if notification is needed
        //
        inline TimePoint get_next_time()
        {
            // time to wait until the top element (or default if queue is empty)
            return m_queue.size() ? m_queue.top().first : (TimeClock::now() + std::chrono::minutes(10));
        }

        // notify all (called from auto_notification)
        inline void notify_all()
        {
            m_wait.notify_all();
        }
        struct auto_notification
        {
            explicit auto_notification(small::time_queue<T>* tq) : m_time_queue(*tq)
            {
                m_old_time = m_time_queue.get_next_time();
            }
            ~auto_notification()
            {
                auto new_time = m_time_queue.get_next_time();
                if (new_time < m_old_time) {
                    m_time_queue.notify_all(); // notify all to recompute the time
                }
            }

            small::time_queue<T>& m_time_queue;
            TimePoint             m_old_time; // time of the first elem (if exists) or some default
        };

        //
        // test and get
        //
        friend BaseQueueWait;

        inline small::WaitFlags test_and_get(T* elem, typename BaseQueueWait::TimePoint* time_wait_until, bool* is_empty_after_get)
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

                // default wait time
                *time_wait_until = get_next_time();
                return small::WaitFlags::kWait;
            }

            // check time
            *time_wait_until = get_next_time();
            if (*time_wait_until > TimeClock::now()) {
                *is_empty_after_get = false;
                return small::WaitFlags::kWait;
            }

            // get elem
            if (elem) {
                *elem = std::move(m_queue.top().second);
            }
            m_queue.pop();

            *is_empty_after_get = m_queue.empty();

            return small::WaitFlags::kElement;
        }

    private:
        //
        // members
        //
        mutable BaseQueueWait m_wait{*this}; // implements locks & wait

        using PriorityQueueElemT = std::pair<TimePoint, T>;
        struct CompPriorityQueueElemT
        {
            bool operator()(const PriorityQueueElemT& l, const PriorityQueueElemT& r) const
            {
                return l.first > r.first;
            }
        };

        std::priority_queue<PriorityQueueElemT, std::vector<PriorityQueueElemT>, CompPriorityQueueElemT> m_queue; // priority queue
    };
} // namespace small
