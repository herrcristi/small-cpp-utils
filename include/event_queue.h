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
// // on main thread no more processing (aborting work)
// q.signal_exit_force(); // q.signal_exit_when_done()
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

        inline bool empty() { return size() == 0; }

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
            m_is_exit_force = false;
            m_is_exit_when_done = false;
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
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock mlock(m_event);
            m_queue.push_back(t);
            m_event.set_event();
        }

        // push_back move semantics
        inline void push_back(T &&t)
        {
            if (is_push_forbidden()) {
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
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock mlock(m_event);
            m_queue.emplace_back(std::forward<_Args>(__args)...);
            m_event.set_event();
        }

        //
        // exit
        //
        inline void signal_exit_force()
        {
            std::unique_lock mlock(m_event);
            m_is_exit_force.store(true);
            m_event.set_event(); /*will notify_all() because is manual*/
        }
        inline bool is_exit_force()
        {
            return m_is_exit_force.load() == true;
        }

        inline void signal_exit_when_done()
        {
            std::unique_lock mlock(m_event);
            m_is_exit_when_done.store(true);
            m_event.set_event(); /*will notify_all() because is manual*/
        }
        inline bool is_exit_when_done()
        {
            return m_is_exit_when_done.load() == true;
        }

        //
        // wait pop_front and return that element
        //
        inline EnumEventQueue wait_pop_front(T *elem)
        {
            if (is_exit_force()) {
                return EnumEventQueue::kQueue_Exit;
            }

            for (; true;) {
                // wait for event to be set
                // multiple threads may wait and when element is pushed all are awaken
                // but not all will have an element to process
                m_event.wait();

                // check queue and element
                std::unique_lock mlock(m_event);
                auto ret_flag = test_and_get_front(elem);

                if (ret_flag == Flags::kFlags_Exit_Force || ret_flag == Flags::kFlags_Exit_When_Done) {
                    return EnumEventQueue::kQueue_Exit;
                }

                if (ret_flag == Flags::kFlags_Element) {
                    return EnumEventQueue::kQueue_Element;
                }

                // continue to wait
            }
        }

        inline EnumEventQueue wait_pop_front(std::vector<T> &vec_elems, int max_count = 1)
        {
            if (is_exit_force()) {
                return EnumEventQueue::kQueue_Exit;
            }

            vec_elems.clear();
            vec_elems.reserve(max_count);
            for (; true;) {
                // wait for event to be set
                // multiple threads may wait and when element is pushed all are awaken
                // but not all will have an element to process
                m_event.wait();

                // check queue and element

                std::unique_lock mlock(m_event);

                T elem{};
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get_front(&elem);

                    if (ret_flag == Flags::kFlags_Exit_Force) {
                        return EnumEventQueue::kQueue_Exit;
                    }

                    if (ret_flag == Flags::kFlags_Exit_When_Done) {
                        // return what was collected until now
                        return vec_elems.size() ? EnumEventQueue::kQueue_Element : EnumEventQueue::kQueue_Exit;
                    }

                    if (ret_flag != Flags::kFlags_Element) {
                        break;
                    }

                    vec_elems.emplace_back(std::move(elem));
                }

                if (vec_elems.size()) {
                    return EnumEventQueue::kQueue_Element;
                }

                // continue to wait
            }
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
            if (is_exit_force()) {
                return EnumEventQueue::kQueue_Exit;
            }

            for (; true;) {
                // wait for event to be set
                // multiple threads may wait and when element is pushed all are awaken
                // but not all will have an element to process
                auto ret = m_event.wait_until(__atime);
                if (ret == std::cv_status::timeout) {
                    return EnumEventQueue::kQueue_Timeout;
                }

                // check queue and element
                std::unique_lock mlock(m_event);

                auto ret_flag = test_and_get_front(elem);

                if (ret_flag == Flags::kFlags_Exit_Force || ret_flag == Flags::kFlags_Exit_When_Done) {
                    return EnumEventQueue::kQueue_Exit;
                }

                if (ret_flag == Flags::kFlags_Element) {
                    return EnumEventQueue::kQueue_Element;
                }

                // continue to wait
            }
        }

        template <typename _Clock, typename _Duration>
        inline EnumEventQueue wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, std::vector<T> &vec_elems, int max_count = 1)
        {
            if (is_exit_force()) {
                return EnumEventQueue::kQueue_Exit;
            }

            vec_elems.clear();
            vec_elems.reserve(max_count);
            for (; true;) {
                // wait for event to be set
                // multiple threads may wait and when element is pushed all are awaken
                // but not all will have an element to process
                auto ret = m_event.wait_until(__atime);
                if (ret == std::cv_status::timeout) {
                    return EnumEventQueue::kQueue_Timeout;
                }

                // check queue and element
                std::unique_lock mlock(m_event);

                T elem{};
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get_front(&elem);

                    if (ret_flag == Flags::kFlags_Exit_Force) {
                        return EnumEventQueue::kQueue_Exit;
                    }

                    if (ret_flag == Flags::kFlags_Exit_When_Done) {
                        // return what was collected until now
                        return vec_elems.size() ? EnumEventQueue::kQueue_Element : EnumEventQueue::kQueue_Exit;
                    }

                    if (ret_flag != Flags::kFlags_Element) {
                        break;
                    }

                    vec_elems.emplace_back(std::move(elem));
                }

                if (vec_elems.size()) {
                    return EnumEventQueue::kQueue_Element;
                }

                // continue to wait
            }
        }

    private:
        //
        // check if push is allowed/forbidden
        //
        inline bool
        is_push_forbidden()
        {
            return is_exit_force() || is_exit_when_done();
        }

        //
        // check for front element with bitwise
        //
        enum class Flags : unsigned int
        {
            kFlags_None = 0,
            kFlags_Exit_Force = 1,
            kFlags_Exit_When_Done = 2,
            kFlags_Element = 3,
        };

        inline Flags test_and_get_front(T *elem)
        {
            if (is_exit_force()) {
                return Flags::kFlags_Exit_Force;
            }

            // check queue size
            if (m_queue.empty()) {
                if (is_exit_when_done()) {
                    // exit but dont reset event to allow other threads to exit
                    return Flags::kFlags_Exit_When_Done;
                }

                // reset event
                m_event.reset_event();
                return Flags::kFlags_None;
            }

            // get elem
            if (elem) {
                *elem = std::move(m_queue.front());
            }
            m_queue.pop_front();

            // reset event if empty queue
            if (m_queue.empty() && !is_exit_when_done()) {
                m_event.reset_event();
            }

            return Flags::kFlags_Element;
        }

    private:
        //
        // members
        //
        std::deque<T> m_queue;                          // queue
        small::event m_event{EventType::kEvent_Manual}; // event
        std::atomic<bool> m_is_exit_force{false};       // force exit
        std::atomic<bool> m_is_exit_when_done{false};   // exit when queue is zero
    };
} // namespace small
