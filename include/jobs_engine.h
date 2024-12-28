#pragma once

#include <unordered_map>

#include "jobs_engine_thread_pool.h"

// // example
// using qc = std::pair<int, std::string>;
//
// enum JobsType
// {
//     job1,
//     job2
// };
//
// small::jobs_engine<JobsType, qc> jobs(
//     {.threads_count = 0 /*dont start any thread yet*/},  // job engine config
//     {.threads_count = 1, .bulk_count = 1},               // default group config
//     {.group = JobsType::job1},                            // default job type config
//     [](auto &j /*this*/, const auto jobs_type, const auto &items) {
//         for (auto &[i, s] : items) {
//             ...
//         }
//     });
//
// // add specific function for job1
// jobs.add_job_group(JobsType::job1, {.threads_count = 2} );
//
// jobs.add_jobs_type(JobsType::job1, {.group = JobsType::job1}, [](auto &j /*this*/, const auto jobs_type, const auto &items, auto b /*extra param b*/) {
//     for(auto &[i, s]:items){
//         ...
//     }
// }, 5 /*param b*/);
//
// // use default config and default function for job2
// jobs.add_jobs_type(JobsType::job2);
// // manual start threads
// jobs.start_threads(3);
//
// // push
// jobs.push_back(small::EnumPriorities::kNormal, JobsType::job1, {1, "a"});
// jobs.push_back(small::EnumPriorities::kNormal, JobsType::job2, {2, "b"});
//
//
// auto ret = jobs.wait_for(std::chrono::milliseconds(0)); // wait to finished
// ... check ret
// jobs.wait(); // wait here for jobs to finish due to exit flag
//

namespace small {

    // config the entire jobs engine
    template <typename JobsPrioT = EnumPriorities>
    struct config_jobs_engine
    {
        int                                 threads_count{8}; // how many total threads for processing
        small::config_prio_queue<JobsPrioT> config_prio{};
    };

    // config an individual job type
    template <typename JobsGroupT>
    struct config_jobs_type
    {
        JobsGroupT group{}; // job type group (multiple job types can be configured to same group)
    };

    // config the job group (where job types can be grouped)
    struct config_jobs_group
    {
        int threads_count{1}; // how many threads for processing (out of the global threads)
        int bulk_count{1};    // how many objects are processed at once
    };

    // a job can be in the following states
    enum class EnumJobsState : unsigned int
    {
        kNone = 0,
        kInProgress,
        kFinished,
        kFailed,
        kCancelled,
        kTimeout
    };

    //
    // small class for jobs where job items have a type (which are further grouped by group), priority, request and response
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename JobsGroupT = JobsTypeT, typename JobsPrioT = EnumPriorities>
    class jobs_engine
    {
    public:
        using JobsID           = unsigned long long;
        using JobsQueue        = small::prio_queue<JobsID, JobsPrioT>;
        using JobDelayedItems  = std::tuple<JobsPrioT, JobsTypeT, JobsID>;
        using ThisJobsEngine   = small::jobs_engine<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT>;
        using JobQueueDelayedT = small::time_queue_thread<JobDelayedItems, ThisJobsEngine>;
        using TimeClock        = typename small::time_queue<JobDelayedItems>::TimeClock;
        using TimeDuration     = typename small::time_queue<JobDelayedItems>::TimeDuration;

        // a job item
        struct JobsItem
        {
            JobsID        id{};       // job unique id
            JobsTypeT     type{};     // job type
            EnumJobsState state{};    // job state
            int           progress{}; // progress 0-100 for state kInProgress
            JobsRequestT  request{};  // request needed for processing function
            JobsResponseT response{}; // where the results are saved (for the finished callback if exists)
        };
        using ProcessingFunction = std::function<void(const std::vector<JobsItem *> &)>;

        //
        // jobs_engine
        //
        explicit jobs_engine(const config_jobs_engine<JobsPrioT> &config_engine = {})
            : m_config{
                  .m_engine{config_engine}}
        {
            // auto start threads if count > 0 otherwise threads should be manually started
            if (m_config.threads_count) {
                start_threads(m_config.threads_count);
            }
        }

        template <typename _Callable, typename... Args>
        jobs_engine(const config_jobs_engine<JobsPrioT> config_engine, const config_jobs_group config_default_group, const config_jobs_type<JobsGroupT> &config_default_jobs_type, _Callable processing_function, Args... extra_parameters)
            : m_config{
                  .m_engine{config_engine},
                  .m_default_group{config_default_group},
                  .m_default_jobs_type{config_default_jobs_type},
                  .m_default_processing_function{std::bind(std::forward<_Callable>(processing_function), std::ref(*this), std::placeholders::_1 /*jobs_item*/, std::forward<Args>(extra_parameters)...)}}

        {
            if (m_config.m_engine.threads_count) {
                start_threads(m_config.m_engine.threads_count);
            }
        }

        ~jobs_engine()
        {
            wait();
        }

        // size of active items
        inline size_t size()
        {
            std::unique_lock l(m_lock);
            return m_jobs.size();
        }
        // empty
        inline bool empty() { return size() == 0; }
        // clear
        inline void clear()
        {
            std::unique_lock l(m_lock);
            m_jobs.clear();

            m_delayed_items.clear();
            m_thread_pool.clear();

            for (auto &[group, q] : m_groups_queues) {
                q.clear();
            }
        }

        // clang-format off
        // size of working items
        inline size_t   size_processing () { return m_thread_pool.size();  }
        // empty
        inline bool     empty_processing() { return size_processing() == 0; }
        // clear
        inline void     clear_processing() { m_thread_pool.clear(); }

        // size of delayed items
        inline size_t   size_delayed() { return m_delayed_items.queue().size();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { m_delayed_items.queue().clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_engine<T>> m...)
        inline void     lock        () { m_lock.lock(); }
        inline void     unlock      () { m_lock.unlock(); }
        inline bool     try_lock    () { return m_lock.try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            m_config.m_engine.threads_count = threads_count;
            m_delayed_items.start_threads();
            m_thread_pool.start_threads(threads_count);
        }

        //
        // config processing by job type
        // THIS SHOULD BE DONE IN THE INITIAL SETUP PHASE ONCE
        //
        //
        // set default job group, job type config and processing function
        //
        template <typename _Callable, typename... Args>
        inline void add_default_config(const config_jobs_group &config_default_group, const config_jobs_type<JobsGroupT> &config_default_jobs_type, _Callable processing_function, Args... extra_parameters)
        {
            m_config.m_default_group               = config_default_group;
            m_config.m_default_jobs_type           = config_default_jobs_type;
            m_config.m_default_processing_function = std::bind(std::forward<_Callable>(processing_function), std::ref(*this), std::placeholders::_1 /*jobs_item*/, std::forward<Args>(extra_parameters)...);
        }

        //
        // config groups
        //
        inline void add_jobs_group(const JobsGroupT &job_group)
        {
            add_jobs_group(job_group, m_config.m_default_group);
        }

        inline void add_jobs_group(const JobsGroupT &job_group, const config_jobs_group &config_group)
        {
            m_config.m_groups[job_group] = config_group;
            m_thread_pool.add_job_group(job_group, config_group.threads_count);
        }

        inline void add_jobs_groups(const std::vector<std::pair<JobsGroupT, config_jobs_group>> &job_group_configs)
        {
            for (auto &[job_group, config_group] : job_group_configs) {
                add_jobs_group(job_group, config_group);
            }
        }

        //
        // config job typpes
        //
        void add_jobs_type(const JobsTypeT &jobs_type)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_config.m_types[jobs_type] = {
                .m_config              = m_config.m_default_jobs_type,
                .m_processing_function = m_config.m_default_processing_function};
        }

        inline void add_jobs_type(const JobsTypeT &jobs_type, const config_jobs_type<JobsGroupT> &config)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_config.m_types[jobs_type] = {
                .m_config              = config,
                .m_processing_function = m_config.m_processing_function};
        }

        template <typename _Callable, typename... Args>
        inline void add_jobs_type(const JobsTypeT jobs_type, const config_jobs_type<JobsGroupT> &config, _Callable processing_function, Args... extra_parameters)
        {
            // m_job_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
            m_config.m_types[jobs_type] = {
                .m_config              = config,
                .m_processing_function = std::bind(std::forward<_Callable>(processing_function), std::ref(*this), std::placeholders::_1 /*jobs_items*/, std::forward<Args>(extra_parameters)...)};
        }

        //
        // get
        //
        inline JobsItem *jobs_get(const JobsID &jobs_id)
        {
            std::unique_lock l(m_lock);

            auto it_j = m_jobs.find(jobs_id);
            return it_j != m_jobs.end() ? &it_j->second : nullptr;
        }

        // TODO add function for progress and state

        //
        // add items to be processed
        // push_back
        //
        inline std::size_t push_back(const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &job_req, JobsID *jobs_id = nullptr)
        {
            return push_back(priority, {.type = jobs_type, .request = job_req}, jobs_id);
        }

        inline std::size_t push_back(const JobsPrioT &priority, const JobsItem &jobs_item, JobsID *jobs_id = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }

            return jobs_activate(priority, jobs_item.type, id);
        }

        inline std::size_t push_back(const JobsPrioT &priority, const std::vector<JobsItem> &jobs_items, std::vector<JobsID> *jobs_ids)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_lock);

            std::size_t count = 0;
            if (jobs_ids) {
                jobs_ids->reserve(jobs_items.size());
                jobs_ids->clear();
            }
            JobsID jobs_id{};
            for (auto &jobs_item : jobs_items) {
                auto ret = push_back(priority, jobs_item, &jobs_id);
                if (ret) {
                    if (jobs_ids) {
                        jobs_ids->push_back(jobs_id);
                    }
                }
                count += ret;
            }
            return count;
        }

        // push_back move semantics
        inline std::size_t push_back(const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back(priority, {.type = jobs_type, .request = std::forward<JobsRequestT>(jobs_req)}, jobs_id);
        }

        inline std::size_t push_back(const JobsPrioT &priority, JobsItem &&jobs_item, JobsID *jobs_id = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            auto id = jobs_add(std::forward<JobsItem>(jobs_item));
            if (jobs_id) {
                *jobs_id = id;
            }

            return jobs_activate(priority, jobs_item.type, id);
        }

        inline std::size_t push_back(const JobsPrioT &priority, std::vector<JobsItem> &&jobs_items, std::vector<JobsID> *jobs_ids)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_lock);

            std::size_t count = 0;
            if (jobs_ids) {
                jobs_ids->reserve(jobs_items.size());
                jobs_ids->clear();
            }
            JobsID jobs_id{};
            for (auto &&jobs_item : jobs_items) {
                auto ret = push_back(priority, std::forward<JobsItem>(jobs_item), &jobs_id);
                if (ret) {
                    if (jobs_ids) {
                        jobs_ids->push_back(jobs_id);
                    }
                }
                count += ret;
            }
            return count;
        }

        // no emplace_back do to returning the jobs_id

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_for(__rtime, priority, {.type = jobs_type, .request = jobs_req}, jobs_id);
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsItem &jobs_item, JobsID *jobs_id = nullptr)
        {
            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_for(__rtime, {priority, jobs_item.type, id});
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_for(__rtime, priority, {.type = jobs_type, .request = std::forward<JobsRequestT>(jobs_req)}, jobs_id);
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, JobsItem &&jobs_item, JobsID *jobs_id = nullptr)
        {
            auto id = jobs_add(std::forward<JobsItem>(jobs_item));
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_for(__rtime, {priority, jobs_item.type, id});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_back_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_until(__atime, priority, {.type = jobs_type, .request = jobs_req}, jobs_id);
        }

        inline std::size_t push_back_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsItem &jobs_item, JobsID *jobs_id = nullptr)
        {
            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_until(__atime, {priority, jobs_item.type, id});
        }

        inline std::size_t push_back_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_until(__atime, priority, {.type = jobs_type, .request = std::forward<JobsRequestT>(jobs_req)}, jobs_id);
        }

        inline std::size_t push_back_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, JobsItem &&jobs_item, JobsID *jobs_id = nullptr)
        {
            auto id = jobs_add(std::forward<JobsItem>(jobs_item));
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_until(__atime, {priority, jobs_item.type, id});
        }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { 
            m_thread_pool.signal_exit_force(); 
            m_delayed_items.queue().signal_exit_force(); 
            
            m_lock.signal_exit_force();
            for (auto &[group, q] : m_groups_queues) {
                q.signal_exit_force();
            }
        }
        inline void signal_exit_when_done   ()  { m_delayed_items.queue().signal_exit_when_done(); /*when the delayed will be finished will signal the active queue items to exit when done, then the processing pool */ }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_delayed_items.queue().is_exit_force(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();

            // first wait for delayed items to finish
            m_delayed_items.wait();

            // only now can signal exit when done for all queues (when no more delayed items can be pushed)
            for (auto &[group, q] : m_groups_queues) {
                q.signal_exit_when_done();
            }
            for (auto &[group, q] : m_groups_queues) {
                q.wait();
            }

            // only now can signal exit when done for workers (when no more items exists)
            return m_thread_pool.wait();
        }

        // wait some time then signal exit
        template <typename _Rep, typename _Period>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period> &__rtime)
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
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime)
        {
            signal_exit_when_done();

            // first wait for delayed items to finish
            auto delayed_status = m_delayed_items.wait_until(__atime);
            if (delayed_status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            // only now can signal exit when done for all queues (when no more delayed items can be pushed)
            for (auto &[group, q] : m_groups_queues) {
                q.signal_exit_when_done();
            }

            for (auto &[group, q] : m_groups_queues) {
                auto status = q.wait_until(__atime);
                if (status == small::EnumLock::kTimeout) {
                    return small::EnumLock::kTimeout;
                }
            }

            // only now can signal exit when done for workers  (when no more items exists)
            return m_thread_pool.wait_until(__atime);
        }

    private:
        // some prevention
        jobs_engine(const jobs_engine &)            = delete;
        jobs_engine(jobs_engine &&)                 = delete;
        jobs_engine &operator=(const jobs_engine &) = delete;
        jobs_engine &operator=(jobs_engine &&__t)   = delete;

    private:
        //
        // inner thread function for executing items (should return if there are more items)
        //

        friend small::jobs_engine_thread_pool<JobsGroupT, ThisJobsEngine>;

        inline EnumLock do_action(const JobsGroupT &jobs_group, bool *has_items)
        {
            *has_items = false;

            // get bulk_count
            auto it_cfg_grp = m_config.m_groups.find(jobs_group);
            if (it_cfg_grp == m_config.m_groups.end()) {
                return small::EnumLock::kExit;
            }

            int bulk_count = std::max(it_cfg_grp->second.bulk_count, 1);

            // get items to process
            auto it_q = m_groups_queues.find(jobs_group);
            if (it_q == m_groups_queues.end()) {
                return small::EnumLock::kExit;
            }
            auto &q = it_q->second;

            std::vector<JobsID> vec_ids;
            auto                ret = q.wait_pop_front_for(std::chrono::nanoseconds(0), vec_ids, bulk_count);
            if (ret == small::EnumLock::kElement) {
                *has_items = true;

                // split by type
                std::unordered_map<JobsTypeT, std::vector<JobsItem *>> elems_by_type;
                for (auto &jobs_id : vec_ids) {
                    auto *jobs_item = jobs_get(jobs_id);
                    if (!jobs_item) {
                        continue;
                    }
                    elems_by_type[jobs_item->type].reserve(vec_ids.size());
                    elems_by_type[jobs_item->type].push_back(jobs_item);
                }

                // process specific job by type
                for (auto &[jobs_type, jobs_items] : elems_by_type) {
                    auto it_cfg_type = m_config.m_types.find(jobs_type);
                    if (it_cfg_type == m_config.m_types.end()) {
                        continue;
                    }

                    // process specific jobs by type
                    it_cfg_type->second.m_processing_function(std::move(jobs_items));
                }
            }

            return ret;
        }

        //
        // add job items
        //
        inline JobsID
        jobs_add(const JobsItem &jobs_item)
        {
            std::unique_lock l(m_lock);

            JobsID id     = ++m_jobs_seq_id;
            m_jobs[id]    = jobs_item;
            m_jobs[id].id = id;

            return id;
        }

        inline JobsID jobs_add(JobsItem &&jobs_item)
        {
            std::unique_lock l(m_lock);

            JobsID id    = ++m_jobs_seq_id;
            jobs_item.id = id;
            m_jobs.emplace(id, std::forward<JobsItem>(jobs_item));

            return id;
        }

        inline void jobs_del(const JobsID &jobs_id)
        {
            std::unique_lock l(m_lock);
            m_jobs.erase(jobs_id);
        }

        // activate the jobs
        //
        // get type queue from the group queues
        //
        inline JobsQueue *get_type_queue(const JobsTypeT &jobs_type)
        {
            // optimization to get the queue from the type
            // (instead of getting the group from type from m_config.m_types and then getting the queue from the m_groups_queues)
            auto it_q = m_types_queues.find(jobs_type);
            if (it_q == m_types_queues.end()) {
                return nullptr;
            }

            return it_q->second;
        }

        inline std::size_t jobs_activate(const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsID &jobs_id)
        {
            std::size_t ret = 0;

            // get queue add add
            auto *q = get_type_queue(jobs_type);
            if (q) {
                ret = q->push_back(priority, jobs_id);
            }

            if (ret) {
                m_thread_pool.job_start(m_config.m_types[jobs_type].m_config.group);
            } else {
                jobs_del(jobs_id);
            }
            return ret;
        }

        //
        // inner thread function for delayed items
        //
        friend JobQueueDelayedT;

        inline std::size_t push_back(std::vector<JobDelayedItems> &&items)
        {
            std::size_t count = 0;
            for (auto &[priority, jobs_type, jobs_id] : items) {
                count += jobs_activate(priority, jobs_type, jobs_id);
            }
            return count;
        }

    private:
        //
        // members
        //
        struct JobsTypeConfig
        {
            config_jobs_type<JobsGroupT> m_config{};              // config for this job type (to which group it belongs)
            ProcessingFunction           m_processing_function{}; // processing Function
        };

        // configs
        struct JobEngineConfig
        {
            config_jobs_engine<JobsPrioT>                     m_engine;                        // config for entire engine (threads, priorities, etc)
            config_jobs_group                                 m_default_group{};               // default config for group
            config_jobs_type<JobsGroupT>                      m_default_jobs_type{};           // default config for jobs type
            ProcessingFunction                                m_default_processing_function{}; // default processing function
            std::unordered_map<JobsGroupT, config_jobs_group> m_groups;                        // config by jobs group
            std::unordered_map<JobsTypeT, JobsTypeConfig>     m_types;                         // config by jobs type
            // TODO add default processing children and default finish function
        };

        JobEngineConfig m_config; // configs for all: engine, groups, job types

        mutable small::base_lock                   m_lock;          // global locker
        std::atomic<JobsID>                        m_jobs_seq_id{}; // to get the next jobs id
        std::unordered_map<JobsID, JobsItem>       m_jobs;          // current jobs
        std::unordered_map<JobsGroupT, JobsQueue>  m_groups_queues; // map of queues by group
        std::unordered_map<JobsTypeT, JobsQueue *> m_types_queues;  // optimize to have queues by type (which reference queues by group)

        JobQueueDelayedT m_delayed_items{*this}; // queue of delayed items

        small::jobs_engine_thread_pool<JobsGroupT, ThisJobsEngine> m_thread_pool{*this}; // scheduler for processing items (by group) using a pool of threads
    };
} // namespace small
