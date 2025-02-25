#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "impl/util_timeout_impl.h"

//
// set_timeout / set_timeout - helping function to execute a function after a specified timeout interval
//
// auto timeoutID = small::set_timeout(std::chrono::milliseconds(1000), [&]() {
//     // ....
// });
// ...
// /*auto ret =*/ small::clear_timeout(timeoutID);
//

namespace small {
    //
    // set_timeout - helping function to execute a function after a specified timeout interval
    // returns the timeoutID with which the timeout can be cancelled if not executed
    //
    template <typename _Callable, typename... Args>
    inline unsigned long long set_timeout(const typename std::chrono::system_clock::duration& timeout, _Callable function_timeout, Args... function_parameters)
    {
        auto& timeout_engine = small::timeoutimpl::get_timeout_engine();
        return timeout_engine.set_timeout(timeout, function_timeout, function_parameters...);
    }

    inline bool clear_timeout(const unsigned long long& timeoutID)
    {
        auto& timeout_engine = small::timeoutimpl::get_timeout_engine();
        return timeout_engine.clear_timeout(timeoutID);
    }

    //
    // set_interval - helping function to execute a function after a specified timeout interval and then at regular intervals
    // returns the intervalID with which the interval can be cancelled if not executed
    //
    template <typename _Callable, typename... Args>
    inline unsigned long long set_interval(const typename std::chrono::system_clock::duration& timeout, _Callable function_interval, Args... function_parameters)
    {
        auto& timeout_engine = small::timeoutimpl::get_timeout_engine();
        return timeout_engine.set_interval(timeout, function_interval, function_parameters...);
    }

    inline bool clear_interval(const unsigned long long& intervalID)
    {
        auto& timeout_engine = small::timeoutimpl::get_timeout_engine();
        return timeout_engine.clear_interval(intervalID);
    }

    //
    // wait functions
    //
    namespace timeout {
        //
        // wait for threads to finish processing
        //
        inline void signal_exit_force()
        {
            return small::timeoutimpl::get_timeout_engine().signal_exit_force();
        }

        inline small::EnumLock wait()
        {
            return small::timeoutimpl::get_timeout_engine().wait();
        }

        // wait some time then signal exit
        template <typename _Rep, typename _Period>
        inline small::EnumLock wait_for(const std::chrono::duration<_Rep, _Period>& __rtime)
        {
            return small::timeoutimpl::get_timeout_engine().wait_for(__rtime);
        }

        // wait until then signal exit
        template <typename _Clock, typename _Duration>
        inline small::EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration>& __atime)
        {
            return small::timeoutimpl::get_timeout_engine().wait_until(__atime);
        }
    } // namespace timeout

} // namespace small