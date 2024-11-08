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
// q.wait();
//

namespace small {
    // enum
    enum class EnumEventQueue
    {
        kQueue_Element,
        kQueue_Timeout,
        kQueue_Exit,
    };

    // queue with events
    template <typename T>
    class event_queue
    {
    public:
        // size
        inline size_t size()
        {
            std::unique_lock<small::event> mlock(event_);
            return queue_.size();
        }
        // empty
        inline bool empty() const { return size() == 0; }

        // clear
        inline void clear()
        {
            std::unique_lock<small::event> mlock(event_);
            queue_.clear();
        }
        // reset
        inline void reset()
        {
            clear();
            exit_flag_ = false;
            event_.set_event_type(EventType::kEvent_Automatic);
            event_.reset_event();
        }

        // use it as locker (std::unique_lock<small:event_queue<T>> m...)
        inline void lock() { event_.lock(); }
        inline void unlock() { event_.unlock(); }
        inline bool try_lock() { return event_.try_lock(); }

        // push_back
        inline void push_back(const T &t)
        {
            if (is_exit()) {
                return;
            }
            {
                std::unique_lock<small::event> mlock(event_);
                queue_.push_back(std::forward<T>(t));
            }
            event_.set_event();
        }
        inline void push_back(T &&t)
        {
            if (is_exit()) {
                return;
            }
            {
                std::unique_lock<small::event> mlock(event_);
                queue_.push_back(std::forward<T>(t));
            }
            event_.set_event();
        }
        // emplace_back
        template <typename... _Args>
        inline void emplace_back(_Args &&...__args)
        {
            if (is_exit()) {
                return;
            }
            {
                std::unique_lock<small::event> mlock(event_);
                queue_.emplace_back(std::forward<_Args>(__args)...);
            }
            event_.set_event();
        }

        // exit
        inline void signal_exit()
        {
            exit_flag_.store(true);
            event_.set_event_type(EventType::kEvent_Manual);
            event_.set_event(); /*event_.notify_all();*/
        }
        inline bool is_exit() { return exit_flag_.load() == true; }

        // wait pop_front and return that element
        inline EnumEventQueue wait_pop_front(T *elem)
        {
            if (exit_flag_.load() == true)
                return EnumEventQueue::kQueue_Exit;

            // wait
            event_.wait([&]() -> bool {
                return test_and_get_front(elem);
            });

            if (exit_flag_.load() == true)
                return EnumEventQueue::kQueue_Exit;

            return EnumEventQueue::kQueue_Element;
        }

        // wait pop_front_for and return that element
        template <typename _Rep, typename _Period>
        inline EnumEventQueue wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, T *elem)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime)
                ++__reltime;
            return wait_pop_front_until(std::chrono::system_clock::now() + __reltime, elem);
        }

        // wait until
        template <typename _Clock, typename _Duration>
        inline EnumEventQueue wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, T *elem)
        {
            if (exit_flag_.load() == true)
                return EnumEventQueue::kQueue_Exit;

            // wait
            bool ret = event_.wait_until(__atime, [&]() -> bool {
                return test_and_get_front(elem);
            });

            if (exit_flag_.load() == true)
                return EnumEventQueue::kQueue_Exit;

            return ret ? EnumEventQueue::kQueue_Element : EnumEventQueue::kQueue_Timeout;
        }

    private:
        // check for front element
        inline bool test_and_get_front(T *elem)
        {
            if (exit_flag_.load() == true)
                return true;

            if (queue_.empty())
                return false;

            if (elem)
                *elem = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

    private:
        // queue
        std::deque<T> queue_;
        // event
        small::event event_;
        // exit flag
        std::atomic<bool> exit_flag_ = false;
    };
} // namespace small
