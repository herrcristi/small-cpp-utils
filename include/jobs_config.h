#pragma once

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
        using JobsItem           = typename small::jobsimpl::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;
        using ProcessingFunction = std::function<void(const std::vector<std::shared_ptr<JobsItem>> &)>;
        // TODO add WaitChildrenFunction
        // TODO add FinishedFunction

        // config for the entire jobs engine
        struct ConfigJobsEngine
        {
            int                                 m_threads_count{8}; // how many total threads for processing
            small::config_prio_queue<JobsPrioT> m_config_prio{};
        };

        // config for an individual job type
        struct ConfigJobsType
        {
            JobsGroupT         m_group{};                        // job type group (multiple job types can be configured to same group)
            bool               m_has_processing_function{false}; // use default processing function
            ProcessingFunction m_processing_function{};          // processing Function
            // TODO add WaitChildrenFunction
            // TODO add FinishedFunction
        };

        // config for the job group (where job types can be grouped)
        struct ConfigJobsGroup
        {
            int m_threads_count{1}; // how many threads for processing (out of the global threads)
            int m_bulk_count{1};    // how many objects are processed at once
        };

        ConfigJobsEngine                                m_engine{};                      // config for entire engine (threads, priorities, etc)
        ProcessingFunction                              m_default_processing_function{}; // default processing function
        std::unordered_map<JobsGroupT, ConfigJobsGroup> m_groups;                        // config by jobs group
        std::unordered_map<JobsTypeT, ConfigJobsType>   m_types;                         // config by jobs type

        // default processing function
        inline void add_default_processing_function(ProcessingFunction processing_function)
        {
            m_default_processing_function = processing_function;
            apply_default_processing_function();
        }

        inline void add_job_processing_function(const JobsTypeT &jobs_type, ProcessingFunction processing_function)
        {
            auto it_f = m_types.find(jobs_type);
            if (it_f == m_types.end()) {
                return;
            }
            it_f->second.m_has_processing_function = true;
            it_f->second.m_processing_function     = processing_function;
        }
        // TODO add job WaitChildrenFunction
        // TODO add job FinishedFunction

        inline void apply_default_processing_function()
        {
            for (auto &[type, jobs_type_config] : m_types) {
                if (jobs_type_config.m_has_processing_function == false) {
                    jobs_type_config.m_processing_function = m_default_processing_function;
                }
            }
        }

        // TODO apply WaitChildrenFunction - the function must be passed as parameter from engine
        // TODO apply FinishedFunction - the function must be passed as parameter from engine
    };
} // namespace small
