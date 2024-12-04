#pragma once

#include <atomic>
#include <deque>

#include "base_lock.h"

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
        lock_queue() = default;

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
        // clear only removes elements from queue but does not reset the event
        //
        inline void clear()
        {
            std::unique_lock l(m_lock);
            m_queue.clear();
        }

        // reset the event and clears all elements
        inline void reset()
        {
            std::unique_lock l(m_lock);
            clear();
            m_lock.reset_exit_force();
            m_lock.reset_exit_when_done();
        }

        // clang-format off
        // use it as locker (std::unique_lock<small:m_LockQueue<T>> m...)
        inline void lock        () { m_lock.lock(); }
        inline void unlock      () { m_lock.unlock(); }
        inline bool try_lock    () { return m_lock.try_lock(); }
        // clang-format on

        //
        // push_back
        //
        inline void push_back(const T &elem)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            m_queue.push_back(elem);
            m_lock.notify_one();
        }

        // push_back move semantics
        inline void push_back(T &&elem)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            m_queue.push_back(std::forward<T>(elem));
            m_lock.notify_one();
        }

        // emplace_back
        template <typename... _Args>
        inline void emplace_back(_Args &&...__args)
        {
            if (is_push_forbidden()) {
                return;
            }

            std::unique_lock l(m_lock);
            m_queue.emplace_back(std::forward<_Args>(__args)...);
            m_lock.notify_one();
        }

        //
        // exit
        //
        inline void signal_exit_force()
        {
            std::unique_lock l(m_lock);
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
            std::unique_lock l(m_lock);
            m_lock.signal_exit_when_done(); // will notify_all
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

                // wait for a notification
                m_lock.wait(l);

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

                // wait for a notification
                m_lock.wait(l);

                // check if there is a new element
            }
        }

        //
        // wait pop_front_for and return that element
        //
        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, T *elem)
        {
            using __dur    = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_front_until(std::chrono::system_clock::now() + __reltime, elem);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, std::vector<T> &vec_elems, int max_count = 1)
        {
            using __dur    = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_front_until(std::chrono::system_clock::now() + __reltime, vec_elems, max_count);
        }

        //
        // wait until
        //
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, T *elem)
        {
            std::unique_lock l(m_lock);

            for (; true;) {
                // check
                auto ret_flag = test_and_get_front(elem);

                if (ret_flag == Flags::kExit_Force || ret_flag == Flags::kExit_When_Done) {
                    return EnumLock::kExit;
                }

                if (ret_flag == Flags::kElement) {
                    return EnumLock::kElement;
                }

                // wait for a notification
                auto ret_w = m_lock.wait_until(l, __atime);
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
                // check for elements
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

                // wait for a notification
                auto ret_w = m_lock.wait_until(l, __atime);
                if (ret_w == EnumLock::kExit) {
                    return EnumLock::kExit;
                }
                if (ret_w == EnumLock::kTimeout) {
                    return EnumLock::kTimeout;
                }

                // check if there is a new element
            }
        }

    private:
        //
        // check if push is allowed/forbidden
        //
        inline bool is_push_forbidden()
        {
            return is_exit_force() || is_exit_when_done();
        }

        //
        // check for front element
        //
        enum class Flags : unsigned int
        {
            kNone           = 0,
            kExit_Force     = 1,
            kExit_When_Done = 2,
            kElement        = 3,
        };

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

                return Flags::kNone;
            }

            // get elem
            if (elem) {
                *elem = std::move(m_queue.front());
            }
            m_queue.pop_front();

            return Flags::kElement;
        }

    private:
        // some prevention
        lock_queue(const lock_queue &) = delete;
        lock_queue(lock_queue &&)      = delete;

        lock_queue &operator=(const lock_queue &) = delete;
        lock_queue &operator=(lock_queue &&__t)   = delete;

    private:
        //
        // members
        //
        small::base_lock m_lock;  // locker
        std::deque<T>    m_queue; // queue
    };
} // namespace small
