#pragma once

#include <atomic>
#include <deque>
#include <queue>

#include "base_lock.h"

// a queue with time events so we can wait for items to be available at specific time moments
//
// small::time_queue<int> q;
// ...
// q.push_back_delay_for( std::chrono::seconds(1), 1 );
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
    // queue which have elements time controlled
    //
    template <typename T>
    class time_queue
    {
    public:
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
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const T &elem)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            push_back_delay_until(std::chrono::system_clock::now() + __reltime, elem);
        }

        template <typename _Clock, typename _Duration>
        inline void push_back_delay_until(const std::chrono::time_point<_Clock, _Duration> &__atime, T &elem)
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
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, T &&elem)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            push_back_delay_until(std::chrono::system_clock::now() + __reltime, std::forward<T>(elem));
        }

        template <typename _Clock, typename _Duration>
        inline void push_back_delay_until(const std::chrono::time_point<_Clock, _Duration> &__atime, T &&elem)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            auto_notification n(this);
            m_queue.push_back({__atime, std::forward<T>(elem)});
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline void emplace_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, _Args &&...__args)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            emplace_back_delay_until(std::chrono::system_clock::now() + __reltime, std::forward<_Args>(__args)...);
        }

        template <typename _Clock, typename _Duration, _Args &&...__args>
        inline void emplace_back_delay_until(const std::chrono::time_point<_Clock, _Duration> &__atime, _Args &&...__args)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            auto_notification n(this);
            m_queue.emplace_back(__atime, T(std::forward<_Args>(__args)...));
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
        // wait pop_front and return that element
        //
        inline EnumLock wait_pop_front(T *elem)
        {
            std::unique_lock l(m_lock);

            for (; true;) {
                // check queue and element
                auto ret_flag = test_and_get_front(elem);

                if (ret_flag == Flags::kExit_Force || ret_flag == Flags::kExit_When_Done) {
                    return EnumLock::kExit;
                }

                if (ret_flag == Flags::kElement) {
                    return EnumLock::kElement;
                }

                // wait for notification
                // TODO with interval
                auto ret_w = m_lock.wait(l);

                // check if there is a new element
            }
        }

        inline EnumLock wait_pop_front(std::vector<T> &vec_elems, int max_count = 1)
        {
            vec_elems.clear();
            vec_elems.reserve(max_count);

            std::unique_lock l(m_lock);

            T elem{};
            for (; true;) {
                // check queue and element
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get_front(&elem);

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
                // TODO with interval
                auto ret_w = m_lock.wait(l);

                // check if there is a new element
            }
        }

        // wait pop_front_for and return that element
        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, T *elem)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_front_until(std::chrono::system_clock::now() + __reltime, elem);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, std::vector<T> &vec_elems, int max_count = 1)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_front_until(std::chrono::system_clock::now() + __reltime, vec_elems, max_count);
        }

        // wait until
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, T *elem)
        {
            std::unique_lock l(m_lock);

            for (; true;) {
                // check queue and element
                auto ret_flag = test_and_get_front(elem);

                if (ret_flag == Flags::kExit_Force || ret_flag == Flags::kExit_When_Done) {
                    return EnumLock::kExit;
                }

                if (ret_flag == Flags::kElement) {
                    return EnumLock::kElement;
                }

                // wait for notification
                // TODO with interval
                auto ret_w = m_lock.wait_until(__atime);
                if (ret_w == EnumLock::kExit) {
                    return EnumLock::kExit;
                }
                if (ret_w == EnumLock::kTimeout) {
                    return EnumLock::kTimeout;
                }

                // check if there is a new element
            }
        }

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, std::vector<T> &vec_elems, int max_count = 1)
        {
            vec_elems.clear();
            vec_elems.reserve(max_count);

            std::unique_lock l(m_lock);

            T elem{};
            for (; true;) {
                // check queue and element
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get_front(&elem);

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
                // TODO with interval
                auto ret = m_lock.wait_until(__atime);
                if (ret_w == EnumLock::kExit) {
                    return EnumLock::kExit;
                }
                if (ret == EnumLock::kTimeout) {
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
        using WaitTime = unsigned long long;
        WaitTime get_wait_time()
        {
            // time to wait until the top element (or default if queue is empty)
            return 0;
        }

        void notify_all()
        {
            m_lock.notify_all();
        }
        struct auto_notification
        {
            auto_notification(small::time_queue &tq) : m_time_queue(tq)
            {
                m_old_wait_time = m_time_queue.get_wait_time();
            }
            ~auto_notification()
            {
                auto new_wait_time = m_time_queue.get_wait_time();
                if (m_old_wait_time != new_wait_time) {
                    m_time_queue.notify_all();
                }
            }

            small::time_queue &m_time_queue;
            WaitTime m_old_wait_time; // time to wait until the top element (or default if queue is empty)
        };
        friend auto_notification;

        //
        // check for front element
        //
        enum class Flags : unsigned int
        {
            kNone = 0,
            kExit_Force = 1,
            kExit_When_Done = 2,
            kElement = 3,
            kWait = 4,
        };

        // TODO return also the wait time
        inline Flags test_and_get_front(T *elem)
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

                // TODO default wait time
                get_wait_time();
                return Flags::kWait;
            }

            // TODO check current elem time

            // get elem
            if (elem) {
                *elem = std::move(m_queue.front());
            }
            m_queue.pop_front();

            return Flags::kElement;
        }

    private:
        //
        // members
        //
        small::base_lock m_lock; // locker

        using PriorityQueueElemT = std::pair<std::chrono::time_point, T>;
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
