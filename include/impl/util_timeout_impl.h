#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "../jobs_engine.h"

namespace small::timeoutimpl {
    //
    // add implementation for timeout and interval
    // using the jobs engine
    //
    class timeout_engine
    {
    public:
        enum class JobsTimeoutType
        {
            kJobsTimeout = 0,
            kJobsInterval,
        };

        enum class JobsTimeoutGroupType
        {
            kJobsGroupDefault,
        };

        using TimeoutDuration   = typename std::chrono::system_clock::duration;
        using TimeoutRequest    = std::pair<TimeoutDuration, std::function<void()>>; // save the duration for the interval
        using TimeoutResponse   = bool;
        using JobsTimeoutEngine = small::jobs_engine<JobsTimeoutType, TimeoutRequest, TimeoutResponse, JobsTimeoutGroupType>;

        //
        // timeout engine
        //
        timeout_engine()
        {
            init_engine();
        }

        ~timeout_engine() = default;

        //
        // init the timeout engine
        //
        void init_engine()
        {
            constexpr int threads_count = 4; // just like in node.js

            m_timeout_engine.set_config(
                {.m_engine = {.m_threads_count = threads_count,
                              .m_config_prio   = {.priorities = {{small::EnumPriorities::kNormal, 1}}}}, // overall config with default priorities

                 // config by jobs group
                 .m_groups = {
                     {JobsTimeoutGroupType::kJobsGroupDefault, {.m_threads_count = threads_count}},
                 },

                 // config by jobs type
                 .m_types = {{JobsTimeoutType::kJobsTimeout, {.m_group = JobsTimeoutGroupType::kJobsGroupDefault}},   // timeout
                             {JobsTimeoutType::kJobsInterval, {.m_group = JobsTimeoutGroupType::kJobsGroupDefault}}}} // interval
            );

            // timeout function
            m_timeout_engine.config_jobs_function_processing(
                JobsTimeoutType::kJobsTimeout, [](auto& /*this engine*/, const auto& items, auto& /* config */) {
                    for (auto& item : items) {
                        auto& [timeout, timeout_function] = item->m_request;
                        // call timeout function
                        timeout_function();
                        item->m_response = true;
                    }
                });

            // interval function
            m_timeout_engine.config_jobs_function_processing(
                JobsTimeoutType::kJobsInterval,
                [&](auto& t /*this engine*/, const auto& items, auto& /* config */) {
                    for (auto& item : items) {
                        auto& [timeout, interval_function] = item->m_request;
                        // call interval function
                        interval_function();
                        item->m_response = true;

                        // auto reschedule the interval (if it is not cancelled while executing)
                        std::unique_lock l(t);

                        auto it_interval = m_jobs_original_intervals_ids.find(item->m_id);
                        if (it_interval == m_jobs_original_intervals_ids.end()) {
                            continue;
                        }
                        auto user_interval_id = it_interval->second;
                        m_jobs_original_intervals_ids.erase(it_interval);

                        JobsTimeoutEngine::JobsID interval_id{};
                        t.queue().push_back_and_start_delay_for(
                            timeout,
                            small::EnumPriorities::kNormal,
                            JobsTimeoutType::kJobsInterval,
                            {timeout, interval_function},
                            &interval_id);

                        // save the new job id
                        m_user_intervals_ids[user_interval_id]     = interval_id;
                        m_jobs_original_intervals_ids[interval_id] = user_interval_id;
                    }
                });
        }

        //
        // get engine
        //
        inline JobsTimeoutEngine& get()
        {
            return m_timeout_engine;
        }

        //
        // timeout
        //
        template <typename _Callable, typename... Args>
        inline JobsTimeoutEngine::JobsID set_timeout(const typename std::chrono::system_clock::duration& timeout, _Callable function_timeout, Args... function_parameters)
        {
            JobsTimeoutEngine::JobsID timeout_id{};

            auto ret = m_timeout_engine.queue().push_back_and_start_delay_for(
                timeout,
                small::EnumPriorities::kNormal,
                JobsTimeoutType::kJobsTimeout,
                {timeout, std::bind(function_timeout, function_parameters...)},
                &timeout_id);

            return ret ? timeout_id : 0;
        }

        inline bool clear_timeout(const JobsTimeoutEngine::JobsID& timeout_id)
        {
            return m_timeout_engine.state().jobs_cancelled(timeout_id);
        }

        //
        // interval
        //
        template <typename _Callable, typename... Args>
        inline JobsTimeoutEngine::JobsID set_interval(const typename std::chrono::system_clock::duration& timeout, _Callable function_interval, Args... function_parameters)
        {
            JobsTimeoutEngine::JobsID interval_id{};

            auto ret = m_timeout_engine.queue().push_back_and_start_delay_for(
                timeout,
                small::EnumPriorities::kNormal,
                JobsTimeoutType::kJobsInterval,
                {timeout, std::bind(function_interval, function_parameters...)},
                &interval_id);

            // save the current interval id returned to user and the current job id
            std::unique_lock l(m_timeout_engine);
            m_user_intervals_ids[interval_id]          = interval_id;
            m_jobs_original_intervals_ids[interval_id] = interval_id;

            return ret ? interval_id : 0;
        }

        inline bool clear_interval(const JobsTimeoutEngine::JobsID& interval_id)
        {
            // get the current job id from the interval id returned to user
            // and erase it
            JobsTimeoutEngine::JobsID current_jobid = 0;
            {
                std::unique_lock l(m_timeout_engine);

                auto it = m_user_intervals_ids.find(interval_id);
                if (it == m_user_intervals_ids.end()) {
                    return false;
                }

                current_jobid = it->second;
                m_user_intervals_ids.erase(it);
                m_jobs_original_intervals_ids.erase(current_jobid);
            }

            return m_timeout_engine.state().jobs_cancelled(current_jobid);
        }

        //
        // wait for threads to finish processing
        //
        inline void signal_exit_force()
        {
            return m_timeout_engine.signal_exit_force();
        }

        inline small::EnumLock wait()
        {
            return m_timeout_engine.wait();
        }

        // wait some time then signal exit
        template <typename _Rep, typename _Period>
        inline small::EnumLock wait_for(const std::chrono::duration<_Rep, _Period>& __rtime)
        {
            return m_timeout_engine.wait_for(__rtime);
        }

        // wait until then signal exit
        template <typename _Clock, typename _Duration>
        inline small::EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration>& __atime)
        {
            return m_timeout_engine.wait_until(__atime);
        }

    private:
        //
        // members
        //
        using IntervalMap = std::unordered_map<JobsTimeoutEngine::JobsID /*original interval id returned to user*/, JobsTimeoutEngine::JobsID /*current job id*/>;

        JobsTimeoutEngine m_timeout_engine;
        IntervalMap       m_user_intervals_ids;          // key=original interval id returned to user and value=current job id
        IntervalMap       m_jobs_original_intervals_ids; // key=current job id and value=original interval id returned to user
    };

    //
    // get global timeout engine
    //
    inline auto& get_timeout_engine()
    {
        static timeout_engine timeout_engine;
        return timeout_engine;
    }

} // namespace small::timeoutimpl