#pragma once

#include <atomic>
#include <deque>

#include "event.h"

// a queue with events so we can wait for items to be available
//
// small::event_queue<int> q;
// ...
// q.push_back( 1 );
// ...
//
// // on some thread
// int e = 0;
// auto ret = q.wait_pop_front( &e ); // ret can be small::EnumEventQueue::kQueue_Exit, small::EnumEventQueue::kQueue_Timeout or ret == small::EnumEventQueue::kQueue_Element
// //auto ret = q.wait_pop_front_for( std::chrono::minutes( 1 ), &e );
// if ( ret == small::EnumEventQueue::kQueue_Element )
// {
//      // do something with e
//      ...
// }
//
// ...
// // on main thread no more processing
// q.signal_exit();
//

namespace small {
    //
    // return type
    //
    enum class EnumEventQueue
    {
        kQueue_Element,
        kQueue_Timeout,
        kQueue_Exit,
    };

    //
    // queue which is event controlled
    //
    template <typename T>
    class event_queue
    {
    public:
        //
        // size
        //
        inline size_t size()
        {
            std::unique_lock mlock(m_event);
            return m_queue.size();
        }

        inline bool empty() const { return size() == 0; }

        //
        // clear only removes elements from queue but does not reset the event
        //
        inline void clear()
        {
            std::unique_lock mlock(m_event);
            m_queue.clear();
        }

        // reset the event and clears all elements
        inline void reset()
        {
            std::unique_lock mlock(m_event);
            clear();
            m_exit_flag = false;
            m_event.set_event_type(EventType::kEvent_Automatic);
            m_event.reset_event();
        }

        // clang-format off
        // use it as locker (std::unique_lock<small:m_eventqueue<T>> m...)
        inline void lock        () { m_event.lock(); }
        inline void unlock      () { m_event.unlock(); }
        inline bool try_lock    () { return m_event.try_lock(); }
        // clang-format on

        //
        // push_back
        //
        inline void push_back(const T &t)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock mlock(m_event);
            m_queue.push_back(t);
            m_event.set_event();
        }

        // push_back move semantics
        inline void push_back(T &&t)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock mlock(m_event);
            m_queue.push_back(std::forward<T>(t));
            m_event.set_event();
        }

        // emplace_back
        template <typename... _Args>
        inline void emplace_back(_Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock mlock(m_event);
            m_queue.emplace_back(std::forward<_Args>(__args)...);
            m_event.set_event();
        }

        //
        // exit
        //
        inline void signal_exit()
        {
            std::unique_lock mlock(m_event);
            m_exit_flag.store(true);
            m_event.set_event_type(EventType::kEvent_Manual);
            m_event.set_event(); /*m_event.notify_all();*/
        }
        inline bool is_exit()
        {
            return m_exit_flag.load() == true;
        }

        //
        // wait pop_front and return that element
        //
        inline EnumEventQueue wait_pop_front(T *elem)
        {
            if (m_exit_flag.load() == true) {
                return EnumEventQueue::kQueue_Exit;
            }

            // wait
            m_event.wait([this, elem]() -> bool {
                return test_and_get_front(elem);
            });

            if (m_exit_flag.load() == true) {
                return EnumEventQueue::kQueue_Exit;
            }

            return EnumEventQueue::kQueue_Element;
        }

        // wait pop_front_for and return that element
        template <typename _Rep, typename _Period>
        inline EnumEventQueue wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, T *elem)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_front_until(std::chrono::system_clock::now() + __reltime, elem);
        }

        // wait until
        template <typename _Clock, typename _Duration>
        inline EnumEventQueue wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, T *elem)
        {
            if (m_exit_flag.load() == true) {
                return EnumEventQueue::kQueue_Exit;
            }

            // wait
            auto ret = m_event.wait_until(__atime, [this, elem]() -> bool {
                return test_and_get_front(elem);
            });

            if (m_exit_flag.load() == true) {
                return EnumEventQueue::kQueue_Exit;
            }

            return ret == std::cv_status::no_timeout ? EnumEventQueue::kQueue_Element : EnumEventQueue::kQueue_Timeout;
        }

    private:
        //
        // check for front element
        //
        inline bool test_and_get_front(T *elem)
        {
            if (m_exit_flag.load() == true) {
                return true;
            }

            if (m_queue.empty()) {
                return false;
            }

            if (elem) {
                *elem = std::move(m_queue.front());
            }
            m_queue.pop_front();
            return true;
        }

    private:
        //
        // members
        //
        std::deque<T> m_queue;                // queue
        small::event m_event;                 // event
        std::atomic<bool> m_exit_flag{false}; // exit flag
    };
} // namespace small
