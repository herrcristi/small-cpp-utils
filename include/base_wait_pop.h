#pragma once

#include <atomic>
#include <deque>
#include <queue>

#include "base_lock.h"

namespace small {

    // wait flags
    enum class WaitFlags : unsigned int
    {
        kWait           = 0,
        kExit_Force     = 1,
        kExit_When_Done = 2,
        kElement        = 3,
    };

    //
    // base class for the wait_pop functions (parent caller must implement test_and_get function)
    //
    template <typename T, typename ParentCallerT>
    class base_wait_pop
    {
    public:
        using TimeClock    = std::chrono::system_clock;
        using TimeDuration = TimeClock::duration;
        using TimePoint    = std::chrono::time_point<TimeClock>;

        //
        // base_wait_pop
        //
        explicit base_wait_pop(ParentCallerT &parent_caller)
            : m_parent_caller(parent_caller)
        {
        }

        base_wait_pop(const base_wait_pop &o) : base_wait_pop() { operator=(o); };
        base_wait_pop(base_wait_pop &&o) noexcept : base_wait_pop() { operator=(std::move(o)); };

        inline base_wait_pop &operator=(const base_wait_pop &)
        {
            return *this;
        }
        inline base_wait_pop &operator=(base_wait_pop &&) noexcept
        {
            return *this;
        }

        // clang-format off
        // use it as locker (std::unique_lock l...)
        inline void lock        () { m_lock.lock(); }
        inline void unlock      () { m_lock.unlock(); }
        inline bool try_lock    () { return m_lock.try_lock(); }
        
        inline void notify_one  () { return m_lock.notify_one(); }
        inline void notify_all  () { return m_lock.notify_all(); }
        // clang-format on

        // clang-format off
        //
        // exit
        //
        inline void signal_exit_force   ()  { m_lock.signal_exit_force(); /*will notify_all*/ }
        inline void reset_exit_force    ()  { m_lock.reset_exit_force(); }
        inline bool is_exit_force       ()  { return m_lock.is_exit_force(); }

        inline void signal_exit_when_done() { m_lock.signal_exit_when_done(); /*will notify all*/ }
        inline void reset_exit_when_done()  { m_lock.reset_exit_when_done(); }
        inline bool is_exit_when_done   ()  { return m_lock.is_exit_when_done(); }

        inline bool is_exit             ()  { return is_exit_force() || is_exit_when_done(); }
        // clang-format on

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

                if (ret_flag == WaitFlags::kExit_Force || ret_flag == WaitFlags::kExit_When_Done) {
                    return EnumLock::kExit;
                }

                if (ret_flag == WaitFlags::kElement) {
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

            T         elem{};
            TimePoint time_wait_until{};
            for (; true;) {
                // check queue and element
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get(&elem, &time_wait_until);

                    if (ret_flag == WaitFlags::kExit_Force) {
                        return EnumLock::kExit;
                    }

                    if (ret_flag == WaitFlags::kExit_When_Done) {
                        // return what was collected until now
                        return vec_elems.size() ? EnumLock::kElement : EnumLock::kExit;
                    }

                    if (ret_flag != WaitFlags::kElement) {
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
            using __dur    = TimeDuration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_pop_until(TimeClock::now() + __reltime, elem);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_for(const std::chrono::duration<_Rep, _Period> &__rtime, std::vector<T> &vec_elems, int max_count = 1)
        {
            using __dur    = TimeDuration;
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

                if (ret_flag == WaitFlags::kExit_Force || ret_flag == WaitFlags::kExit_When_Done) {
                    return EnumLock::kExit;
                }

                if (ret_flag == WaitFlags::kElement) {
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

            T         elem{};
            TimePoint time_wait_until{};
            for (; true;) {
                // check queue and element
                for (int i = 0; i < max_count; ++i) {
                    auto ret_flag = test_and_get(&elem, &time_wait_until);

                    if (ret_flag == WaitFlags::kExit_Force) {
                        return EnumLock::kExit;
                    }

                    if (ret_flag == WaitFlags::kExit_When_Done) {
                        // return what was collected until now
                        return vec_elems.size() ? EnumLock::kElement : EnumLock::kExit;
                    }

                    if (ret_flag != WaitFlags::kElement) {
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
        // call the parent
        //
        inline small::WaitFlags test_and_get(T *elem, TimePoint *time_wait_until)
        {
            *time_wait_until = TimeClock::now() + std::chrono::minutes(60);
            return m_parent_caller.test_and_get(elem, time_wait_until);
        }

    private:
        //
        // members
        //
        mutable small::base_lock m_lock;          // locker
        ParentCallerT           &m_parent_caller; // parent caller
    };
} // namespace small
