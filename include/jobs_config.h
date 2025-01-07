#pragma once

#include <optional>
#include <unordered_map>

#include "prio_queue.h"

#include "impl/jobs_item_impl.h"

namespace small {
    //
    // small class for jobs config
    // - setup how many threads to use overall, and the priorities
    // - for each job group how many threads to use
    // - for each job type how to process and to which group it belongs
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename JobsGroupT, typename JobsPrioT>
    struct jobs_config
    {
        using JobsItem = typename small::jobsimpl::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;

        // config for the entire jobs engine
        struct ConfigJobsEngine
        {
            int                                 m_threads_count{8};          // how many total threads for processing
            int                                 m_threads_count_finished{2}; // how many threads (out of total m_threads_count) to use for processing finished states
            small::config_prio_queue<JobsPrioT> m_config_prio{};
        };

        // config for the job group (where job types can be grouped)
        struct ConfigJobsGroup
        {
            int                                      m_threads_count{1};     // how many threads for processing (out of the global threads)
            int                                      m_bulk_count{1};        // how many objects are processed at once
            std::optional<std::chrono::milliseconds> m_delay_next_request{}; // if need to delay the next request processing to have some throtelling
        };

        // to be passed to processing function
        struct ConfigProcessing
        {
            std::optional<std::chrono::milliseconds> m_delay_next_request{}; // if need to delay the next request processing to have some throtelling
        };

        // functions
        using FunctionProcessing         = std::function<void(const std::vector<std::shared_ptr<JobsItem>> &, ConfigProcessing &config)>;
        using FunctionOnChildrenFinished = std::function<void(const std::vector<std::shared_ptr<JobsItem>> &)>;
        using FunctionFinished           = std::function<void(const std::vector<std::shared_ptr<JobsItem>> &)>;

        // config for an individual job type
        struct ConfigJobsType
        {
            JobsGroupT                               m_group{};                                  // job type group (multiple job types can be configured to same group)
            std::optional<std::chrono::milliseconds> m_timeout{};                                // if need to delay the next request processing to have some throtelling
            bool                                     m_has_function_processing{false};           // use default processing function
            bool                                     m_has_function_on_children_finished{false}; // use default function for children finished
            bool                                     m_has_function_finished{false};             // use default finished function
            FunctionProcessing                       m_function_processing{};                    // processing Function for jobs items
            FunctionOnChildrenFinished               m_function_on_children_finished{};          // function called for a parent when a child is finished
            FunctionFinished                         m_function_finished{};                      // function called when jobs items are finished
        };

        ConfigJobsEngine                                m_engine{};                                // config for entire engine (threads, priorities, etc)
        FunctionProcessing                              m_default_function_processing{};           // default processing function
        FunctionOnChildrenFinished                      m_default_function_on_children_finished{}; // default function to call for a parent when children are finished
        FunctionFinished                                m_default_function_finished{};             // default function to call when jobs items are finished
        std::unordered_map<JobsGroupT, ConfigJobsGroup> m_groups;                                  // config by jobs group
        std::unordered_map<JobsTypeT, ConfigJobsType>   m_types;                                   // config by jobs type

        //
        // add default processing function
        //
        inline void add_default_function_processing(FunctionProcessing function_processing)
        {
            m_default_function_processing = function_processing;
            apply_default_function_processing();
        }

        inline void add_default_function_on_children_finished(FunctionOnChildrenFinished function_on_children_finished)
        {
            m_default_function_on_children_finished = function_on_children_finished;
            apply_default_function_on_children_finished();
        }

        inline void add_default_function_finished(FunctionFinished function_finished)
        {
            m_default_function_finished = function_finished;
            apply_default_function_finished();
        }

        //
        // add job functions
        //
        inline void add_job_function_processing(const JobsTypeT &jobs_type, FunctionProcessing function_processing)
        {
            auto it_f = m_types.find(jobs_type);
            if (it_f == m_types.end()) {
                return;
            }
            it_f->second.m_has_function_processing = true;
            it_f->second.m_function_processing     = function_processing;
        }

        inline void add_job_function_on_children_finished(const JobsTypeT &jobs_type, FunctionOnChildrenFinished function_on_children_finished)
        {
            auto it_f = m_types.find(jobs_type);
            if (it_f == m_types.end()) {
                return;
            }
            it_f->second.m_has_function_on_children_finished = true;
            it_f->second.m_function_on_children_finished     = function_on_children_finished;
        }

        inline void add_job_function_finished(const JobsTypeT &jobs_type, FunctionFinished function_finished)
        {
            auto it_f = m_types.find(jobs_type);
            if (it_f == m_types.end()) {
                return;
            }
            it_f->second.m_has_function_finished = true;
            it_f->second.m_function_finished     = function_finished;
        }

        //
        // apply default function where it is not set a specific one
        //
        inline void apply_default_function_processing()
        {
            for (auto &[type, jobs_type_config] : m_types) {
                if (jobs_type_config.m_has_function_processing == false) {
                    jobs_type_config.m_function_processing = m_default_function_processing;
                }
            }
        }

        inline void apply_default_function_on_children_finished()
        {
            for (auto &[type, jobs_type_config] : m_types) {
                if (jobs_type_config.m_has_function_on_children_finished == false) {
                    jobs_type_config.m_function_on_children_finished = m_default_function_on_children_finished;
                }
            }
        }

        inline void apply_default_function_finished()
        {
            for (auto &[type, jobs_type_config] : m_types) {
                if (jobs_type_config.m_has_function_finished == false) {
                    jobs_type_config.m_function_finished = m_default_function_finished;
                }
            }
        }
    };
} // namespace small
