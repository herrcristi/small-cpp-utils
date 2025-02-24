#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "jobs_engine.h"

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
    namespace timeoutimpl {
        enum class JobsType
        {
            kJobsTimeout = 0,
            kJobsInterval,
        };

        enum class JobsGroupType
        {
            kJobsGroupDefault,
        };

        using TimeoutDuration = typename std::chrono::system_clock::duration;
        using TimeoutRequest  = std::tuple<TimeoutDuration, void* /*engine*/, std::function<void()>>; // save the duration for the interval
        using TimeoutResponse = bool;
        using JobsEng         = small::jobs_engine<JobsType, TimeoutRequest, TimeoutResponse, JobsGroupType>;

        // get the timeout engine
        inline auto& get_timeout_engine()
        {

            // this functions is defined without the engine params (it is here just for the example)
            static auto jobs_function_processing = [](const std::vector<std::shared_ptr<JobsEng::JobsItem>>& items, JobsEng::JobsConfig::ConfigProcessing& /* config */) {
                for (auto& item : items) {
                    auto& [timeout, engine, timeout_function] = item->m_request;
                    timeout_function();
                    item->m_response = true;

                    // reschedule the interval
                    if (item->m_type == JobsType::kJobsInterval) {
                        small::timeoutimpl::JobsEng::JobsID intervalID{};

                        auto* j = ((JobsEng*)engine);
                        j->queue().push_back_and_start_delay_for(
                            timeout,
                            small::EnumPriorities::kNormal,
                            small::timeoutimpl::JobsType::kJobsInterval,
                            {timeout, engine, timeout_function},
                            &intervalID);
                    }
                }
            };

            constexpr int threads_count = 4; // just like in node.js

            static JobsEng::JobsConfig config{
                .m_engine = {.m_threads_count = threads_count,
                             .m_config_prio   = {.priorities = {{small::EnumPriorities::kNormal, 1}}}}, // overall config with default priorities

                .m_default_function_processing = jobs_function_processing, // default processing function

                // config by jobs group
                .m_groups = {
                    {JobsGroupType::kJobsGroupDefault, {.m_threads_count = threads_count}},
                },

                // config by jobs type
                .m_types = {{JobsType::kJobsTimeout, {.m_group = JobsGroupType::kJobsGroupDefault}},    // timeout
                            {JobsType::kJobsInterval, {.m_group = JobsGroupType::kJobsGroupDefault}}}}; // interval

            static JobsEng jobs_timeout{config};

            return jobs_timeout;
        }
    } // namespace timeoutimpl

    //
    // set_timeout - helping function to execute a function after a specified timeout interval
    // returns the timeoutID with which the timeout can be cancelled if not executed
    //
    template <typename _Callable, typename... Args>
    inline unsigned long long set_timeout(const std::chrono::milliseconds& timeout, _Callable function_timeout, Args... function_parameters)
    {
        auto& jobs_timeout = timeoutimpl::get_timeout_engine();

        timeoutimpl::JobsEng::JobsID timeoutID{};

        auto ret = jobs_timeout.queue().push_back_and_start_delay_for(
            timeout,
            small::EnumPriorities::kNormal,
            timeoutimpl::JobsType::kJobsTimeout,
            {timeout, &jobs_timeout, std::bind(function_timeout, function_parameters...)},
            &timeoutID);

        return ret ? timeoutID : 0;
    }

    inline bool clear_timeout(const unsigned long long& timeoutID)
    {
        auto& jobs_timeout = timeoutimpl::get_timeout_engine();

        return jobs_timeout.state().jobs_cancelled(timeoutID);
    }

    //
    // set_interval - helping function to execute a function after a specified timeout interval and then at regular intervals
    // returns the intervalID with which the interval can be cancelled if not executed
    //
    template <typename _Callable, typename... Args>
    inline unsigned long long set_interval(const std::chrono::milliseconds& timeout, _Callable function_interval, Args... function_parameters)
    {
        auto& jobs_timeout = timeoutimpl::get_timeout_engine();

        timeoutimpl::JobsEng::JobsID intervalID{};

        auto ret = jobs_timeout.queue().push_back_and_start_delay_for(
            timeout,
            small::EnumPriorities::kNormal,
            timeoutimpl::JobsType::kJobsInterval,
            {timeout, &jobs_timeout, std::bind(function_interval, function_parameters...)},
            &intervalID);

        return ret ? intervalID : 0;
    }

    inline bool clear_interval(const unsigned long long& intervalID)
    {
        auto& jobs_timeout = timeoutimpl::get_timeout_engine();

        return jobs_timeout.state().jobs_cancelled(intervalID);
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
            return timeoutimpl::get_timeout_engine().signal_exit_force();
        }

        inline EnumLock wait()
        {
            return timeoutimpl::get_timeout_engine().wait();
        }

        // wait some time then signal exit
        template <typename _Rep, typename _Period>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period>& __rtime)
        {
            return timeoutimpl::get_timeout_engine().wait_for(__rtime);
        }

        // wait until then signal exit
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration>& __atime)
        {
            return timeoutimpl::get_timeout_engine().wait_until(__atime);
        }
    } // namespace timeout

} // namespace small