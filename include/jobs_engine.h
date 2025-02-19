#pragma once

#include <unordered_map>
#include <unordered_set>

#include "impl/jobs_item_impl.h"
#include "impl/jobs_queue_impl.h"
#include "impl/jobs_state_impl.h"
#include "impl/jobs_thread_pool_impl.h"
#include "jobs_config.h"

// enum class JobsType
// {
//     kJobsType1,
//     kJobsType2
// };
// enum class JobsGroupType
// {
//     kJobsGroup1
// };
//
// using Request = std::pair<int, std::string>;
// using JobsEng = small::jobs_engine<JobsType, Request, int /*response*/, JobsGroupType>;
//
//
// JobsEng::JobsConfig config{
//     .m_engine                      = {.m_threads_count = 0 /*dont start any thread yet*/},   // overall config with default priorities
//     .m_groups                      = {
//          {JobsGroupType::kJobsGroup1, {.m_threads_count = 1}}},                              // config by jobs group
//     .m_types                       = {
//         {JobsType::kJobsType1, {.m_group = JobsGroupType::kJobsGroup1}},
//         {JobsType::kJobsType2, {.m_group = JobsGroupType::kJobsGroup1}},
//     }};
//
// // create jobs engine
// JobsEng jobs(config);
//
// jobs.config_default_function_processing([](auto &j /*this jobs engine*/, const auto &jobs_items) {
//     for (auto &item : jobs_items) {
//         ...
//     }
// });
//
// // add specific function for job1
// jobs.config_jobs_function_processing(JobsType::kJobsType1, [](auto &j /*this jobs engine*/, const auto &jobs_items, auto b /*extra param b*/) {
//     for (auto &item : jobs_items) {
//         ...
//     }
// }, 5 /*param b*/);
//
// JobsEng::JobsID              jobs_id{};
// std::vector<JobsEng::JobsID> jobs_ids;
//
// // push
// jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, {1, "normal"}, &jobs_id);
// jobs.queue().push_back_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {100, "delay normal"}, &jobs_id);
//
// jobs.start_threads(3); // manual start threads
//
// // jobs.signal_exit_force();
// jobs.wait(); // wait here for jobs to finish due to exit flag
//
// for a more complex example see examples/examples_jobs_engine.h

namespace small {

    //
    // small class for jobs where job items have a type (which are further grouped by group), priority, request and response
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename JobsGroupT = JobsTypeT, typename JobsPrioT = EnumPriorities>
    class jobs_engine
    {
    public:
        using ThisJobsEngine             = typename small::jobs_engine<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        using JobsConfig                 = typename small::jobs_config<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        using JobsItem                   = typename small::jobsimpl::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;
        using JobsQueue                  = typename small::jobsimpl::jobs_queue<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT, ThisJobsEngine>;
        using JobsState                  = typename small::jobsimpl::jobs_state_impl<JobsTypeT, JobsRequestT, JobsResponseT, ThisJobsEngine>;
        using JobsID                     = typename JobsItem::JobsID;
        using TimeClock                  = typename JobsQueue::TimeClock;
        using TimeDuration               = typename JobsQueue::TimeDuration;
        using FunctionProcessing         = typename JobsConfig::FunctionProcessing;
        using FunctionOnChildrenFinished = typename JobsConfig::FunctionOnChildrenFinished;
        using FunctionFinished           = typename JobsConfig::FunctionFinished;

    public:
        //
        // jobs_engine
        //
        explicit jobs_engine(const JobsConfig& config = {})
            : m_config{config}
        {
            apply_config();
        }

        explicit jobs_engine(JobsConfig&& config)
            : m_config{config}
        {
            apply_config();
        }

        ~jobs_engine()
        {
            wait();
        }

        // size of active items
        inline size_t size() { return m_queue.size(); }
        // empty
        inline bool empty() { return size() == 0; }
        // clear
        inline void clear()
        {
            std::unique_lock l(*this);
            m_queue.clear();
            m_thread_pool.clear();
        }

        // clang-format off
        // size of working items
        inline size_t   size_processing () { return m_thread_pool.size();  }
        // empty
        inline bool     empty_processing() { return size_processing() == 0; }
        // clear
        inline void     clear_processing() { m_thread_pool.clear(); }

        // size of delayed items
        inline size_t   size_delayed() { return queue().size_delayed();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { queue().clear_delayed(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_engine<T>> m...)
        inline void     lock        () { queue().lock(); }
        inline void     unlock      () { queue().unlock(); }
        inline bool     try_lock    () { return queue().try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            m_config.m_engine.m_threads_count = threads_count;
            m_queue.start_threads(threads_count);
            m_timeout_queue.start_threads();
            m_thread_pool.start_threads(threads_count);
        }

        //
        // config
        // THIS SHOULD BE DONE IN THE INITIAL SETUP PHASE ONCE
        //
        inline void set_config(const JobsConfig& config)
        {
            m_config = config;
            apply_config();
        }

        inline void set_config(JobsConfig&& config)
        {
            m_config = std::move(config);
            apply_config();
        }

        // override default jobs function
        template <typename _Callable, typename... Args>
        inline void config_default_function_processing(_Callable function_processing, Args... extra_parameters)
        {
            m_config.config_default_function_processing(
                std::bind(std::forward<_Callable>(function_processing),
                          std::ref(*this),
                          std::placeholders::_1 /*jobs_items*/,
                          std::placeholders::_2 /*config*/,
                          std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void config_default_function_children_finished(_Callable function_children_finished, Args... extra_parameters)
        {
            m_config.config_default_function_children_finished(
                std::bind(std::forward<_Callable>(function_children_finished),
                          std::ref(*this),
                          std::placeholders::_1 /*parent job*/,
                          std::placeholders::_2 /*child job*/,
                          std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void config_default_function_finished(_Callable function_finished, Args... extra_parameters)
        {
            m_config.config_default_function_finished(
                std::bind(std::forward<_Callable>(function_finished),
                          std::ref(*this),
                          std::placeholders::_1 /*jobs_items*/,
                          std::forward<Args>(extra_parameters)...));
        }

        // specific jobs functions
        template <typename _Callable, typename... Args>
        inline void config_jobs_function_processing(const JobsTypeT& jobs_type, _Callable function_processing, Args... extra_parameters)
        {
            m_config.config_jobs_function_processing(
                jobs_type,
                std::bind(std::forward<_Callable>(function_processing),
                          std::ref(*this),
                          std::placeholders::_1 /*jobs_items*/,
                          std::placeholders::_2 /*config*/,
                          std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void config_jobs_function_children_finished(const JobsTypeT& jobs_type, _Callable function_children_finished, Args... extra_parameters)
        {
            m_config.config_jobs_function_children_finished(
                jobs_type,
                std::bind(std::forward<_Callable>(function_children_finished),
                          std::ref(*this),
                          std::placeholders::_1 /*parent job*/,
                          std::placeholders::_2 /*child job*/,
                          std::forward<Args>(extra_parameters)...));
        }

        template <typename _Callable, typename... Args>
        inline void config_jobs_function_finished(const JobsTypeT& jobs_type, _Callable function_finished, Args... extra_parameters)
        {
            m_config.config_jobs_function_finished(
                jobs_type,
                std::bind(std::forward<_Callable>(function_finished),
                          std::ref(*this),
                          std::placeholders::_1 /*jobs_items*/,
                          std::forward<Args>(extra_parameters)...));
        }

        //
        // jobs functions
        //

        // clang-format off
        //
        // add items to jobs queue
        //
        inline JobsQueue& queue() { return m_queue; }

        // and some helper functions
        inline std::size_t jobs_start(const JobsPrioT& priority, const JobsID& jobs_id)                 { return queue().jobs_start(priority, jobs_id); }
        inline std::size_t jobs_start(const JobsPrioT& priority, const std::vector<JobsID>& jobs_ids)   { return queue().jobs_start(priority, jobs_ids); }

        inline std::shared_ptr<JobsItem>              jobs_get(const JobsID& jobs_id)                   { return queue().jobs_get(jobs_id); }
        inline std::vector<std::shared_ptr<JobsItem>> jobs_get(const std::vector<JobsID>& jobs_ids)     { return queue().jobs_get(jobs_ids); }

        inline std::size_t jobs_parent_child(const JobsID& parent_jobs_id, const JobsID& child_jobs_id) { return queue().jobs_parent_child(parent_jobs_id, child_jobs_id); }
        inline std::size_t jobs_parent_child(std::shared_ptr<JobsItem> parent_jobs_item, std::shared_ptr<JobsItem> child_jobs_item) { return queue().jobs_parent_child(parent_jobs_item, child_jobs_item); }
        // clang-format on

        //
        // set states for jobs items
        //
        inline JobsState& state() { return m_state; }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { m_thread_pool.signal_exit_force(); m_timeout_queue.queue().signal_exit_force(); m_queue.signal_exit_force(); }
        inline void signal_exit_when_done   ()  { m_queue.signal_exit_when_done(); /*when the delayed will be finished will signal the active queue items to exit when done, then the processing pool */ }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_queue.is_exit_force(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();

            // first wait for queue items to finish
            m_queue.wait();

            // only now can signal exit when done for workers (when no more items exists)
            m_thread_pool.wait();

            // the timeouts are no longer necessay, force exit
            m_timeout_queue.queue().signal_exit_force();
            m_timeout_queue.wait();

            return small::EnumLock::kExit;
        }

        // wait some time then signal exit
        template <typename _Rep, typename _Period>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period>& __rtime)
        {
            using __dur    = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if (__reltime < __rtime) {
                ++__reltime;
            }
            return wait_until(std::chrono::system_clock::now() + __reltime);
        }

        // wait until then signal exit
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration>& __atime)
        {
            signal_exit_when_done();

            // first wait for delayed items to finish
            auto delayed_status = m_queue.wait_until(__atime);
            if (delayed_status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            // only now can signal exit when done for workers  (when no more items exists)
            delayed_status = m_thread_pool.wait_until(__atime);
            if (delayed_status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            // the timeouts are no longer necessay, force exit
            m_timeout_queue.queue().signal_exit_force();
            m_timeout_queue.wait();

            return small::EnumLock::kExit;
        }

    private:
        // some prevention
        jobs_engine(const jobs_engine&)            = delete;
        jobs_engine(jobs_engine&&)                 = delete;
        jobs_engine& operator=(const jobs_engine&) = delete;
        jobs_engine& operator=(jobs_engine&& __t)  = delete;

    private:
        //
        // apply config
        //
        inline void apply_config()
        {
            // setup jobs groups
            for (auto& [jobs_group, jobs_group_config] : m_config.m_groups) {
                m_queue.config_jobs_group(jobs_group, m_config.m_engine.m_config_prio);
                m_thread_pool.config_jobs_group(jobs_group, jobs_group_config.m_threads_count);
            }

            // setup jobs types
            if (!m_config.m_default_function_finished) {
                m_config.m_default_function_finished = std::bind(&jobs_engine::jobs_on_finished, this, std::placeholders::_1 /*jobs_items*/);
            }
            if (!m_config.m_default_function_children_finished) {
                m_config.m_default_function_children_finished = std::bind(&jobs_engine::jobs_on_child_finished, this, std::placeholders::_1 /*jobs parent*/, std::placeholders::_2 /*jobs_child*/);
            }

            m_config.apply_default_function_processing();
            m_config.apply_default_function_children_finished();
            m_config.apply_default_function_finished();

            for (auto& [jobs_type, jobs_type_config] : m_config.m_types) {
                m_queue.config_jobs_type(jobs_type, jobs_type_config.m_group);
            }

            // auto start threads if count > 0 otherwise threads should be manually started
            if (m_config.m_engine.m_threads_count) {
                start_threads(m_config.m_engine.m_threads_count);
            }
        }

        //
        // get jobs to execute based on the group
        //
        inline EnumLock get_group_jobs(const JobsGroupT& jobs_group, std::vector<JobsID>& vec_ids, typename JobsConfig::ConfigProcessing& group_config)
        {
            // get bulk_count property
            auto it_cfg_grp = m_config.m_groups.find(jobs_group);
            if (it_cfg_grp == m_config.m_groups.end()) {
                return small::EnumLock::kExit;
            }

            int bulk_count = std::max(it_cfg_grp->second.m_bulk_count, 1);

            // delay request
            group_config.m_delay_next_request = it_cfg_grp->second.m_delay_next_request;

            // get items to process
            auto* q = m_queue.get_jobs_group_queue(jobs_group);
            if (!q) {
                return small::EnumLock::kExit;
            }

            auto ret = q->wait_pop_front_for(std::chrono::nanoseconds(0), vec_ids, bulk_count);
            return ret;
        }

        //
        // callbacks for jobs_thread_pool
        // inner thread function for executing items (should return if there are more items)
        //
        friend small::jobsimpl::jobs_thread_pool<JobsGroupT, ThisJobsEngine>;

        inline EnumLock do_action(const JobsGroupT& jobs_group, std::chrono::milliseconds& delay_next_request)
        {
            // get jobs for the group
            typename JobsConfig::ConfigProcessing group_config{}; // for delay request
            std::vector<JobsID>                   vec_ids;

            auto ret = get_group_jobs(jobs_group, vec_ids, group_config);
            if (ret != small::EnumLock::kElement) {
                return ret;
            }

            // split by type
            std::unordered_map<JobsTypeT, std::vector<std::shared_ptr<JobsItem>>> jobs_in_progress_by_type;
            {
                // get jobs
                std::vector<std::shared_ptr<JobsItem>> jobs_items = jobs_get(vec_ids);
                for (auto& jobs_item : jobs_items) {
                    jobs_in_progress_by_type[jobs_item->m_type].reserve(jobs_items.size());

                    // mark the item as in progress
                    jobs_item->set_state_inprogress();
                    // execute if it is still in progress
                    // (may be moved to higher states due to external factors like cancel, timeout, finish early due to other job, etc)
                    if (jobs_item->is_state_inprogress()) {
                        jobs_in_progress_by_type[jobs_item->m_type].push_back(jobs_item);
                    }
                }
            }

            // process specific job by type
            for (auto& [jobs_type, jobs_items] : jobs_in_progress_by_type) {
                auto it_cfg_type = m_config.m_types.find(jobs_type);
                if (it_cfg_type == m_config.m_types.end()) {
                    continue;
                }

                // process specific jobs by type
                typename JobsConfig::ConfigProcessing type_config{};
                it_cfg_type->second.m_function_processing(jobs_items, type_config);

                // get the max for config
                if (!group_config.m_delay_next_request) {
                    group_config.m_delay_next_request = type_config.m_delay_next_request;
                } else {
                    if (type_config.m_delay_next_request) {
                        group_config.m_delay_next_request = std::max(group_config.m_delay_next_request, type_config.m_delay_next_request);
                    }
                }

                // mark the item as in wait for children or finished
                // if in callback the state is set to failed, cancelled or timeout
                // setting to finish wont succeed because is less value than those
                state().jobs_waitforchildren(jobs_items);
            }

            // delay request
            delay_next_request = group_config.m_delay_next_request.value_or(std::chrono::milliseconds(0));

            return ret;
        }

        //
        // callbacks for jobs_queue
        // inner function for extra processing after addding the jobs into queue (called from queue)
        //
        friend JobsQueue;

        inline void jobs_add(std::shared_ptr<JobsItem> jobs_item)
        {
            // called from queue for extra processing when a jobs is added
            // check if needs to be added it to the timeout queue (only if job type has config a timeout)
            auto timeout = m_config.m_types[jobs_item->m_type].m_timeout;
            if (timeout) {
                m_timeout_queue.queue().push_delay_for(*timeout, jobs_item->m_id);
            }
        }

        inline bool jobs_cancelled(std::shared_ptr<JobsItem> jobs_item)
        {
            // called from queue for extra processing when a jobs is cancelled
            return state().jobs_state(jobs_item, small::jobsimpl::EnumJobsState::kCancelled);
        }

        //
        // inner function for activate the jobs from queue (called from queue)
        //
        inline void jobs_schedule(std::shared_ptr<JobsItem> jobs_item)
        {
            if (jobs_item) {
                m_thread_pool.jobs_schedule(m_config.m_types[jobs_item->m_type].m_group);
            }
        }

        //
        // callbacks for timeout_queue
        // inner thread function for timeout items (called from m_timeout_queue)
        //
        using JobsQueueTimeout = small::time_queue_thread<JobsID, ThisJobsEngine>;
        friend JobsQueueTimeout;

        inline std::size_t push_back(std::vector<JobsID>&& jobs_ids)
        {
            // this jobs has reached the timeout
            state().jobs_timeout(jobs_ids);
            return jobs_ids.size();
        }

        //
        // callbacks for jobs_state
        //
        friend JobsState;

        //
        // when a job is completed (finished/timeout/canceled/failed)
        //
        inline void jobs_completed(std::shared_ptr<JobsItem> jobs_item)
        {
            if (!jobs_item) {
                return;
            }

            // call the custom function from config (if exists, otherwise the default will be called)
            m_config.m_types[jobs_item->m_type].m_function_finished({jobs_item});

            // if it has parents call jobs_on_child_finished (or custom function) for each parent
            if (jobs_item->has_parents()) {
                // progress 100 will make the state be finished
                // but since the state can only increase
                // the completed callback will not be called again because the state will not change
                state().jobs_progress(jobs_item, 100);

                auto jobs_parents = jobs_get(jobs_item->m_parentIDs);
                for (auto& jobs_parent : jobs_parents) {
                    m_config.m_types[jobs_parent->m_type].m_function_children_finished(jobs_parent, jobs_item /*child*/);
                }
            } else {
                // delete only if there are no parents (+delete all children)
                m_queue.jobs_erase(jobs_item->m_id);
            }
        }

        inline void jobs_completed(const std::vector<std::shared_ptr<JobsItem>>& jobs_items)
        {
            for (auto& jobs_item : jobs_items) {
                jobs_completed(jobs_item);
            }
        }

        //
        // when is finished
        //
        inline void jobs_on_finished(const std::vector<std::shared_ptr<JobsItem>>& /* jobs_items */)
        {
            // by default nothing to do here, but it can be setup for each jobs type
        }

        //
        // after child is finished
        //
        inline void jobs_on_child_finished(std::shared_ptr<JobsItem> jobs_parent, std::shared_ptr<JobsItem> jobs_child)
        {
            //
            // compute parent state and progress based on children
            // if a children has failed/timeout/cancelled then parent is set to failed
            // if all children are finished then the parent is finished
            //
            if (jobs_parent->is_complete()) {
                return;
            }

            int                            jobs_progress = 0;
            small::jobsimpl::EnumJobsState jobs_state    = small::jobsimpl::EnumJobsState::kInProgress;
            state().get_children_states(jobs_parent, &jobs_state, &jobs_progress);

            if (JobsItem::is_state_complete(jobs_state)) {
                state().jobs_state(jobs_parent, jobs_state);
            } else {
                state().jobs_progress(jobs_parent, jobs_progress); // TODO this should be recursive child->parent->grand parent (taking into account state)
            }
        }

    private:
        //
        // members
        //
        JobsConfig                                                    m_config;
        JobsQueue                                                     m_queue{*this};
        JobsState                                                     m_state{*this};
        JobsQueueTimeout                                              m_timeout_queue{*this}; // for timeout elements
        small::jobsimpl::jobs_thread_pool<JobsGroupT, ThisJobsEngine> m_thread_pool{*this};   // for processing items (by group) using a pool of threads
    };
} // namespace small
