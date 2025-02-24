#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "jobs_engine.h"

// set_timeout - helping function to execute a function after a specified timeout interval
//
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

        using TimeoutRequest  = std::function<void()>;
        using TimeoutResponse = bool;
        using JobsEng         = small::jobs_engine<JobsType, TimeoutRequest, TimeoutResponse, JobsGroupType>;

        // get the timeout engine
        inline auto& get_timeout_engine()
        {
            constexpr int threads_count = 4; // just like in node.js

            // this functions is defined without the engine params (it is here just for the example)
            static auto jobs_function_processing = [](const std::vector<std::shared_ptr<JobsEng::JobsItem>>& items, JobsEng::JobsConfig::ConfigProcessing& /* config */) {
                for (auto& item : items) {
                    item->m_request();
                    item->m_response = true;
                }
            };

            static JobsEng::JobsConfig config{
                .m_engine = {.m_threads_count = threads_count,
                             .m_config_prio   = {.priorities = {{small::EnumPriorities::kNormal, 1}}}}, // overall config with default priorities

                .m_default_function_processing = jobs_function_processing,

                // config by jobs group
                .m_groups = {
                    {JobsGroupType::kJobsGroupDefault, {.m_threads_count = threads_count}},
                },

                // config by jobs type
                .m_types = {{JobsType::kJobsTimeout, {.m_group = JobsGroupType::kJobsGroupDefault}},    // timeout
                            {JobsType::kJobsInterval, {.m_group = JobsGroupType::kJobsGroupDefault}}}}; // interval

            static small::jobs_engine<JobsType, TimeoutRequest, TimeoutResponse, JobsGroupType> jobs_timeout{config};

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
            std::bind(function_timeout, function_parameters...),
            &timeoutID);

        return ret ? timeoutID : 0;
    }

    inline bool clear_timeout(const unsigned long long& timeoutID)
    {
        auto& jobs_timeout = timeoutimpl::get_timeout_engine();

        return jobs_timeout.state().jobs_cancelled(timeoutID);
    }

} // namespace small