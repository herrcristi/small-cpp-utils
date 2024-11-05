#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

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
// small::event e( small::EventType::kEvent_Manual );
//
namespace small {
    enum class EventType
    {
        kEvent_Automatic,
        kEvent_Manual
    };

    // event class based on mutex
    class event
    {
    public:
        //
        // event
        //
        event(const EventType &event_type = EventType::kEvent_Automatic)
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
        inline void set_event_type(const EventType &event_type = EventType::kEvent_Automatic)
        {
            std::unique_lock<std::mutex> mlock(m_lock);
            m_event_type = event_type;
        }

        //
        // set event
        //
        inline void set_event()
        {
            std::unique_lock<std::mutex> mlock(m_lock);
            m_event_value = true;

            bool notify_all = m_event_type == EventType::kEvent_Manual;
            if (notify_all) {
                m_condition.notify_all();
            } else {
                m_condition.notify_one();
            }
        }

        //
        // reset event
        //
        inline void reset_event()
        {
            std::unique_lock<std::mutex> mlock(m_lock);
            m_event_value = false;
        }

        //
        // wait for event to be set
        //
        inline void wait()
        {
            std::unique_lock<std::mutex> mlock(m_lock);

            while (test_event_and_reset() == false) {
                m_condition.wait(mlock);
            }
        }

        //
        // wait with condition
        //
        template <typename _Predicate>
        inline void wait(_Predicate __p)
        {
            for (; true; std::this_thread::sleep_for(std::chrono::milliseconds(1))) {
                std::unique_lock<std::mutex> mlock(m_lock);

                // both conditions must be met

                // wait for event first
                // and dont consume the event if condition is not met
                while (test_event() == false) {
                    m_condition.wait(mlock);
                }

                if (__p()) {
                    // reset event
                    test_event_and_reset();
                    return;
                }

                // event is signaled but condition not met, just throttle
                // and allow others to use the event
                m_condition.notify_one();
                // release lock and just sleep for a bit
            }
        }

        //
        // wait for (it uses wait_until)
        //
        template <typename _Rep, typename _Period>
        inline std::cv_status wait_for(const std::chrono::duration<_Rep, _Period> &__rtime)
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
        inline std::cv_status wait_for(const std::chrono::duration<_Rep, _Period> &__rtime, _Predicate __p)
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
        inline std::cv_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime)
        {
            std::unique_lock<std::mutex> mlock(m_lock);

            while (test_event_and_reset() == false) {
                std::cv_status ret = m_condition.wait_until(mlock, __atime);
                if (ret == std::cv_status::timeout) {
                    // one last check
                    return test_event_and_reset() == true ? std::cv_status::no_timeout : std::cv_status::timeout /*still timeout*/;
                }
            }
            return std::cv_status::no_timeout;
        }

        //
        // wait_until (with condition)
        //
        template <typename _Clock, typename _Duration, typename _Predicate>
        inline std::cv_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime, _Predicate __p)
        {
            for (; true; std::this_thread::sleep_for(std::chrono::milliseconds(1))) {
                std::unique_lock<std::mutex> mlock(m_lock);

                // both conditions must be met

                // wait for event first
                // and dont consume the event if condition is not met
                while (test_event() == false) {
                    std::cv_status ret = m_condition.wait_until(mlock, __atime);
                    if (ret == std::cv_status::timeout) {
                        // one last check
                        return __p() && test_event_and_reset() == true ? std::cv_status::no_timeout : std::cv_status::timeout /*still timeout*/;
                    }
                }

                if (__p()) {
                    // reset event
                    test_event_and_reset();
                    return std::cv_status::no_timeout;
                }

                // event is signaled but condition not met, just throttle
                // and allow others to use the event
                m_condition.notify_one();
                // release lock and just sleep for a bit
            }

            return std::cv_status::timeout;
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
                if (m_event_type == EventType::kEvent_Automatic) {
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
        std::mutex m_lock;                                   // mutex locker
        std::condition_variable m_condition;                 // condition
        EventType m_event_type{EventType::kEvent_Automatic}; // for manual event
        std::atomic_bool m_event_value{};                    // event state
    };
} // namespace small
