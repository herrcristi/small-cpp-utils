#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "base_lock.h"

// use it as an event
//
// small::event e;
// ...
// {
//     std::unique_lock<small::event> mlock( e ); // use it as a locker
//     ...
// }
// ...
// e.set_event();
// ...
//
// // on some thread
// e.wait();
// // or
// e.wait( [&]() -> bool {
//     return /*some conditions*/ ? true : false; // see event_queue how it uses this
// } );
// ...
// ...
// // create a manual event
// small::event e( small::EventType::kManual );
//
namespace small {
    //
    // event type
    //
    enum class EventType
    {
        kAutomatic,
        kManual
    };

    //
    // event class based on mutex lock
    //
    class event
    {
    public:
        //
        // event
        //
        event(const EventType &event_type = EventType::kAutomatic)
            : m_event_type(event_type)
        {
        }

        // clang-format off
        // use it as locker (std::unique_lock<small:event> m...)
        inline void     lock            () { m_lock.lock();   }
        inline void     unlock          () { m_lock.unlock(); }
        inline bool     try_lock        () { return m_lock.try_lock(); }
        // clang-format on

        //
        // set type
        //
        inline void set_event_type(const EventType &event_type = EventType::kAutomatic)
        {
            std::unique_lock l(m_lock);
            m_event_type = event_type;
        }

        //
        // set event
        //
        inline void set_event()
        {
            std::unique_lock l(m_lock);
            m_event_value = true;

            bool notify_all = m_event_type == EventType::kManual;
            if (notify_all) {
                m_lock.notify_all();
            } else {
                m_lock.notify_one();
            }
        }

        //
        // reset event
        //
        inline void reset_event()
        {
            std::unique_lock l(m_lock);
            m_event_value = false;
        }

        //
        // wait for event to be set
        //
        inline EnumLock wait()
        {
            std::unique_lock l(m_lock);

            while (test_event_and_reset() == false) {
                auto ret_w = m_lock.wait();
                if (ret_w == EnumLock::kExit) {
                    return EnumLock::kExit;
                }
            }

            return EnumLock::kElement;
        }

        //
        // wait with condition
        //
        template <typename _Predicate>
        inline EnumLock wait(_Predicate __p)
        {
            for (; true; std::this_thread::sleep_for(std::chrono::milliseconds(1))) {
                std::unique_lock l(m_lock);

                // both conditions must be met

                // wait for event first
                // and dont consume the event if condition is not met
                while (test_event() == false) {
                    auto ret_w = m_lock.wait();
                    if (ret_w == EnumLock::kExit) {
                        return EnumLock::kExit;
                    }
                }

                if (__p()) {
                    // reset event
                    test_event_and_reset();
                    return EnumLock::kElement;
                }

                // event is signaled but condition not met, just throttle
                // and allow others to use the event
                m_lock.notify_one();
                // release lock and just sleep for a bit
            }

            return EnumLock::kTimeout;
        }

        //
        // wait for (it uses wait_until)
        //
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

        //
        // wait_for with condition
        //
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

        //
        // wait until
        //
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime)
        {
            std::unique_lock l(m_lock);

            while (test_event_and_reset() == false) {
                auto ret_w = m_lock.wait_until(__atime);
                if (ret_w == EnumLock::kTimeout) {
                    // one last check
                    return test_event_and_reset() == true ? EnumLock::kElement : EnumLock::kTimeout /*still timeout*/;
                }
            }
            return EnumLock::kElement;
        }

        //
        // wait_until (with condition)
        //
        template <typename _Clock, typename _Duration, typename _Predicate>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime, _Predicate __p)
        {
            for (; true; std::this_thread::sleep_for(std::chrono::milliseconds(1))) {
                std::unique_lock l(m_lock);

                // both conditions must be met

                // wait for event first
                // and dont consume the event if condition is not met
                while (test_event() == false) {
                    auto ret = m_lock.wait_until(__atime);
                    if (ret == EnumLock::kTimeout) {
                        // one last check
                        return __p() && test_event_and_reset() == true ? EnumLock::kElement : EnumLock::kTimeout /*still timeout*/;
                    }
                }

                if (__p()) {
                    // reset event
                    test_event_and_reset();
                    return EnumLock::kElement;
                }

                // event is signaled but condition not met, just throttle
                // and allow others to use the event
                m_lock.notify_one();
                // release lock and just sleep for a bit
            }

            return EnumLock::kTimeout;
        }

    private:
        //
        // test event
        //
        inline bool test_event()
        {
            return m_event_value.load() == true;
        }

        //
        // test event and consume it
        //
        inline bool test_event_and_reset()
        {
            if (m_event_value.load() == true) {
                // automatic event resets the state
                if (m_event_type == EventType::kAutomatic) {
                    m_event_value = false;
                }
                return true;
            }
            return false;
        }

    private:
        //
        // members
        //
        small::base_lock m_lock;                       // locker
        EventType m_event_type{EventType::kAutomatic}; // for manual event
        std::atomic_bool m_event_value{};              // event state
    };
} // namespace small
