#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace small {
    //
    // return type
    //
    enum class EnumLock
    {
        kElement, // as no timeout
        kTimeout,
        kExit,
    };

    //
    // helper class for locking combines mutex + condition variable
    //
    class base_lock
    {
    public:
        // clang-format off
        // use it as locker (std::unique_lock m...)
        inline void lock        () { m_lock.lock(); }
        inline void unlock      () { m_lock.unlock(); }
        inline bool try_lock    () { return m_lock.try_lock(); }
        // clang-format on

        //
        // exit
        //
        inline void signal_exit_force()
        {
            std::unique_lock mlock(m_lock);
            m_is_exit_force.store(true);
            m_condition.notify_all();
        }
        inline void reset_exit_force()
        {
            m_is_exit_force.store(false);
        }
        inline bool is_exit_force()
        {
            return m_is_exit_force.load() == true;
        }

        inline void signal_exit_when_done()
        {
            std::unique_lock mlock(m_lock);
            m_is_exit_when_done.store(true);
            m_condition.notify_all();
        }
        inline void reset_exit_when_done()
        {
            m_is_exit_when_done.store(false);
        }
        inline bool is_exit_when_done()
        {
            return m_is_exit_when_done.load() == true;
        }

        //
        // notify
        //
        inline void notify_one()
        {
            return m_condition.notify_one();
        }

        inline void notify_all()
        {
            return m_condition.notify_all();
        }

        //
        // wait requires lock ( std::unique_lock mlock( ... ); )
        //
        inline EnumLock wait()
        {
            if (is_exit_force()) {
                return EnumLock::kExit;
            }

            m_condition.wait(m_lock);

            return is_exit_force() ? EnumLock::kExit : EnumLock::kElement;
        }

        //
        // wait requires lock ( std::unique_lock mlock( ... ); )
        //
        template <typename _Predicate>
        inline EnumLock wait(_Predicate __p)
        {
            if (is_exit_force()) {
                return EnumLock::kExit;
            }

            m_condition.wait(m_lock, __p);

            return is_exit_force() ? EnumLock::kExit : EnumLock::kElement;
        }

        // wait for requires lock ( std::unique_lock mlock( ... ); )
        template <typename _Rep, typename _Period>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period> &__rtime)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_until(std::chrono::system_clock::now() + __reltime);
        }

        template <typename _Rep, typename _Period, typename _Predicate>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period> &__rtime, _Predicate __p)
        {
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }

            return wait_until(std::chrono::system_clock::now() + __reltime, std::move(__p));
        }

        // wait until requires lock ( std::unique_lock mlock( ... ); )
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime)
        {
            if (is_exit_force()) {
                return EnumLock::kExit;
            }

            auto ret_w = m_condition.wait_until(m_lock, __atime);

            if (is_exit_force()) {
                return EnumLock::kExit;
            }

            return ret_w == std::cv_status::timeout ? EnumLock::kTimeout : EnumLock::kElement;
        }

        template <typename _Clock, typename _Duration, typename _Predicate>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime, _Predicate __p)
        {
            if (is_exit_force()) {
                return EnumLock::kExit;
            }

            while (!__p()) {
                auto ret_w = m_condition.wait_until(m_lock, __atime);

                if (is_exit_force()) {
                    return EnumLock::kExit;
                }

                if (ret_w == std::cv_status::timeout) {
                    return EnumLock::kTimeout;
                }
            }

            return EnumLock::kElement;
        }

    private:
        //
        // members
        //
        std::recursive_mutex m_lock;             // mutex locker
        std::condition_variable_any m_condition; // condition

        std::atomic<bool> m_is_exit_force{false};     // force exit
        std::atomic<bool> m_is_exit_when_done{false}; // exit when done (used by queue when no more elements after this condition is set)
    };
} // namespace small
