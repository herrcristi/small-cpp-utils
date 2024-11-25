#pragma once

#include <atomic>
#include <deque>
#include <queue>

#include "base_lock.h"

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
        using TimeClock = std::chrono::system_clock;
        using TimeDuration = TimeClock::duration;
        using TimePoint = std::chrono::time_point<TimeClock>;

        //
        // size
        //
        inline size_t size()
        {
            std::unique_lock l(m_lock);
            return m_queue.size();
        }

        inline bool empty() { return size() == 0; }

        //
        // clear only removes elements from queue
        //
        inline void clear()
        {
            std::unique_lock l(m_lock);
            m_queue.clear();
        }

        // clear only removes elements from queue and all flags
        inline void reset()
        {
            std::unique_lock l(m_lock);
            clear();
            m_lock.reset_exit_force();
            m_lock.reset_exit_when_done();
        }

        // clang-format off
        // use it as locker (std::unique_lock l...)
        inline void lock        () { m_lock.lock(); }
        inline void unlock      () { m_lock.unlock(); }
        inline bool try_lock    () { return m_lock.try_lock(); }
        // clang-format on

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline void push_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const T &elem)
        {
            using __dur = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            push_delay_until(TimeClock::now() + __reltime, elem);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const T &elem)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            auto_notification n(this);
            m_queue.push({__atime, elem});
        }

        // push_back move semantics
        template <typename _Rep, typename _Period>
        inline void push_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, T &&elem)
        {
            using __dur = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            push_delay_until(TimeClock::now() + __reltime, std::forward<T>(elem));
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, T &&elem)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            auto_notification n(this);
            m_queue.push({__atime, std::forward<T>(elem)});
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline void emplace_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, _Args &&...__args)
        {
            using __dur = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            emplace_delay_until(TimeClock::now() + __reltime, std::forward<_Args>(__args)...);
        }

        template </* typename _Clock, typename _Duration, */ typename... _Args> // avoid time_casting from one clock to another
        inline void emplace_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, _Args &&...__args)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            auto_notification n(this);
            m_queue.emplace(__atime, T(std::forward<_Args>(__args)...));
        }

        //
        // exit
        //
        inline void signal_exit_force()
        {
            m_lock.signal_exit_force(); // will notify_all
        }
        inline void reset_exit_force()
        {
            m_lock.reset_exit_force();
        }
        inline bool is_exit_force()
        {
            return m_lock.is_exit_force();
        }

        inline void signal_exit_when_done()
        {
            m_lock.signal_exit_when_done(); // will notify all
        }
        inline void reset_exit_when_done()
        {
            m_lock.reset_exit_when_done();
        }
        inline bool is_exit_when_done()
        {
            return m_lock.is_exit_when_done();
        }

        //
        // wait pop and return that element
        //
        inline EnumLock wait_pop(T *elem)
        {
            std::unique_lock l(m_lock);

            TimePoint time_wait_until{};
            for (; true;) {
                // check queue and element
                auto ret_flag = test_and_get(elem, &time_wait_until);

                if (ret_flag == Flags::kExit_Force || ret_flag == Flags::kExit_When_Done) {
                    return EnumLock::kExit;
                }

                if (ret_flag == Flags::kElement) {
                    return EnumLock::kElement;
                }

                // wait for notification
                m_lock.wait_until(l, time_wait_until);

                // check if there is a new element
            }
        }

        inline EnumLock wait_pop(std::vector<T> &vec_elems, int max_count = 1)
        {
            vec_elems.clear();
            vec_elems.reserve(max_count);

            std::unique_lock l(m_lock);

            T elem{};
            TimePoint time_wait_until{};
            for (; true;) {
                // check queue and element
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get(&elem, &time_wait_until);

                    if (ret_flag == Flags::kExit_Force) {
                        return EnumLock::kExit;
                    }

                    if (ret_flag == Flags::kExit_When_Done) {
                        // return what was collected until now
                        return vec_elems.size() ? EnumLock::kElement : EnumLock::kExit;
                    }

                    if (ret_flag != Flags::kElement) {
                        break;
                    }

                    vec_elems.emplace_back(std::move(elem));
                }

                if (vec_elems.size()) {
                    return EnumLock::kElement;
                }

                // wait for notification
                m_lock.wait_until(l, time_wait_until);

                // check if there is a new element
            }
        }

        // wait pop_for and return that element
        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_for(const std::chrono::duration<_Rep, _Period> &__rtime, T *elem)
        {
            using __dur = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_until(TimeClock::now() + __reltime, elem);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_for(const std::chrono::duration<_Rep, _Period> &__rtime, std::vector<T> &vec_elems, int max_count = 1)
        {
            using __dur = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_until(TimeClock::now() + __reltime, vec_elems, max_count);
        }

        // wait until
        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline EnumLock wait_pop_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, T *elem)
        {
            std::unique_lock l(m_lock);

            TimePoint time_wait_until{};
            for (; true;) {
                // check queue and element
                auto ret_flag = test_and_get(elem, &time_wait_until);

                if (ret_flag == Flags::kExit_Force || ret_flag == Flags::kExit_When_Done) {
                    return EnumLock::kExit;
                }

                if (ret_flag == Flags::kElement) {
                    return EnumLock::kElement;
                }

                // wait for notification
                auto min_time = std::min(__atime, time_wait_until);

                auto ret_w = m_lock.wait_until(l, min_time);
                if (ret_w == EnumLock::kExit) {
                    return EnumLock::kExit;
                }
                if (min_time == __atime && ret_w == EnumLock::kTimeout) {
                    return EnumLock::kTimeout;
                }

                // check if there is a new element
            }
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline EnumLock wait_pop_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, std::vector<T> &vec_elems, int max_count = 1)
        {
            vec_elems.clear();
            vec_elems.reserve(max_count);

            std::unique_lock l(m_lock);

            T elem{};
            TimePoint time_wait_until{};
            for (; true;) {
                // check queue and element
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get(&elem, &time_wait_until);

                    if (ret_flag == Flags::kExit_Force) {
                        return EnumLock::kExit;
                    }

                    if (ret_flag == Flags::kExit_When_Done) {
                        // return what was collected until now
                        return vec_elems.size() ? EnumLock::kElement : EnumLock::kExit;
                    }

                    if (ret_flag != Flags::kElement) {
                        break;
                    }

                    vec_elems.emplace_back(std::move(elem));
                }

                if (vec_elems.size()) {
                    return EnumLock::kElement;
                }

                // wait for notification
                auto min_time = std::min(__atime, time_wait_until);

                auto ret_w = m_lock.wait_until(l, min_time);
                if (ret_w == EnumLock::kExit) {
                    return EnumLock::kExit;
                }
                if (min_time == __atime && ret_w == EnumLock::kTimeout) {
                    return EnumLock::kTimeout;
                }

                // check if there is a new element
            }
        }

    protected:
        //
        // check if push is allowed/forbidden
        //
        inline bool is_push_forbidden()
        {
            return is_exit_force() || is_exit_when_done();
        }

        //
        // compute if notification is needed
        //
        TimePoint get_next_time()
        {
            // time to wait until the top element (or default if queue is empty)
            return m_queue.size() ? m_queue.top().first : (TimeClock::now() + std::chrono::minutes(10));
        }

        // notify all (called from auto_notification)
        void notify_all()
        {
            m_lock.notify_all();
        }
        struct auto_notification
        {
            auto_notification(small::time_queue<T> *tq) : m_time_queue(*tq)
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

            small::time_queue<T> &m_time_queue;
            TimePoint m_old_time; // time of the first elem (if exists) or some default
        };

        //
        // check for element
        //
        enum class Flags : unsigned int
        {
            kWait = 0,
            kExit_Force = 1,
            kExit_When_Done = 2,
            kElement = 3,
        };

        // test and get
        inline Flags test_and_get(T *elem, TimePoint *time_wait_until)
        {
            if (is_exit_force()) {
                return Flags::kExit_Force;
            }

            // check queue size
            if (m_queue.empty()) {
                if (is_exit_when_done()) {
                    // exit but dont reset event to allow other threads to exit
                    return Flags::kExit_When_Done;
                }

                // default wait time
                *time_wait_until = get_next_time();
                return Flags::kWait;
            }

            // check time
            *time_wait_until = get_next_time();
            if (*time_wait_until > TimeClock::now()) {
                return Flags::kWait;
            }

            // get elem
            if (elem) {
                *elem = std::move(m_queue.top().second);
            }
            m_queue.pop();

            return Flags::kElement;
        }

    private:
        //
        // members
        //
        small::base_lock m_lock; // locker

        using PriorityQueueElemT = std::pair<TimePoint, T>;
        struct CompPriorityQueueElemT
        {
            bool operator()(const PriorityQueueElemT &l, const PriorityQueueElemT &r) const
            {
                return l.first > r.first;
            }
        };

        std::priority_queue<PriorityQueueElemT, std::vector<PriorityQueueElemT>, CompPriorityQueueElemT> m_queue; // priority queue
    };
} // namespace small
