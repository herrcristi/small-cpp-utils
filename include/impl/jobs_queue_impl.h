#pragma once

#include <unordered_map>

#include "../prio_queue.h"
#include "../time_queue_thread.h"

#include "jobs_item_impl.h"

namespace small::jobsimpl {
    //
    // small queue helper class for jobs (parent caller must implement 'jobs_activate', 'jobs_finished')
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename JobsGroupT, typename JobsPrioT, typename ParentCallerT>
    class jobs_queue
    {
    public:
        using JobsItem  = typename small::jobsimpl::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;
        using JobsID    = typename JobsItem::JobsID;
        using JobsQueue = small::prio_queue<JobsID, JobsPrioT>;

        using ThisJobsQueue = jobs_queue<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT, ParentCallerT>;

        using JobDelayedItems  = std::tuple<JobsPrioT, JobsTypeT, JobsID>;
        using JobQueueDelayedT = small::time_queue_thread<JobDelayedItems, ThisJobsQueue>;

        using TimeClock    = typename small::time_queue<JobDelayedItems>::TimeClock;
        using TimeDuration = typename small::time_queue<JobDelayedItems>::TimeDuration;

    public:
        //
        // jobs_queue
        //
        explicit jobs_queue(ParentCallerT &parent_caller)
            : m_parent_caller(parent_caller) {}

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

            for (auto &[group, q] : m_groups_queues) {
                q.clear();
            }
        }

        // clang-format off
        // size of delayed items
        inline size_t   size_delayed() { return m_delayed_items.queue().size();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { m_delayed_items.queue().clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock m...)
        inline void     lock        () { m_lock.lock(); }
        inline void     unlock      () { m_lock.unlock(); }
        inline bool     try_lock    () { return m_lock.try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            m_delayed_items.start_threads();
        }

        //
        // config groups
        // m_groups_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
        //
        inline void config_jobs_group(const JobsGroupT &job_group, const small::config_prio_queue<JobsPrioT> &config_prio)
        {
            m_groups_queues[job_group] = JobsQueue{config_prio};
        }

        //
        // config job types
        // m_types_queues will be initialized in the initial setup phase and will be accessed without locking afterwards
        //
        inline bool config_jobs_type(const JobsTypeT &jobs_type, const JobsGroupT &jobs_group, const std::optional<std::chrono::milliseconds> &jobs_timeout)
        {
            auto it_g = m_groups_queues.find(jobs_group);
            if (it_g == m_groups_queues.end()) {
                return false;
            }

            m_types_queues[jobs_type] = &it_g->second;
            if (jobs_timeout) {
                m_types_timeouts[jobs_type] = *jobs_timeout;
            }
            return true;
        }

        //
        // add items to be processed
        // push_back
        //
        inline std::size_t push_back(const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &job_req, JobsID *jobs_id = nullptr)
        {
            return push_back(priority, std::make_shared<JobsItem>(jobs_type, job_req), jobs_id);
        }

        inline std::size_t push_back(const JobsPrioT &priority, std::shared_ptr<JobsItem> jobs_item, JobsID *jobs_id = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }

            return jobs_activate(priority, jobs_item->m_type, id);
        }

        inline std::size_t push_back(const JobsPrioT &priority, const std::vector<std::shared_ptr<JobsItem>> &jobs_items, std::vector<JobsID> *jobs_ids)
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
            return push_back(priority, std::make_shared<JobsItem>(jobs_type, std::forward<JobsRequestT>(jobs_req)), jobs_id);
        }

        // TODO add push_back_child()

        // no emplace_back do to returning the jobs_id

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_for(__rtime, priority, std::make_shared<JobsItem>(jobs_type, jobs_req), jobs_id);
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, std::shared_ptr<JobsItem> jobs_item, JobsID *jobs_id = nullptr)
        {
            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_for(__rtime, {priority, jobs_item->m_type, id});
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_for(__rtime, priority, std::make_shared<JobsItem>(jobs_type, std::forward<JobsRequestT>(jobs_req)), jobs_id);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_back_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_until(__atime, priority, std::make_shared<JobsItem>(jobs_type, jobs_req), jobs_id);
        }

        inline std::size_t push_back_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, std::shared_ptr<JobsItem> jobs_item, JobsID *jobs_id = nullptr)
        {
            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_until(__atime, {priority, jobs_item->m_type, id});
        }

        inline std::size_t push_back_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_delay_until(__atime, priority, std::make_shared<JobsItem>(jobs_type, std::forward<JobsRequestT>(jobs_req)), jobs_id);
        }

        // TODO add push_back_child_....()

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { 
            m_delayed_items.queue().signal_exit_force(); 
            
            m_lock.signal_exit_force();
            for (auto &[group, q] : m_groups_queues) {
                q.signal_exit_force();
            }
        }
        inline void signal_exit_when_done   ()  { m_delayed_items.queue().signal_exit_when_done(); /*when the delayed will be finished will signal the active queue items to exit when done*/ }
        
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

            return small::EnumLock::kExit;
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

            return small::EnumLock::kExit;
        }

    private:
        friend ParentCallerT;

        //
        // get group queue
        // called from parent jobs engine
        //
        inline JobsQueue *get_group_queue(const JobsGroupT &jobs_group)
        {
            auto it = m_groups_queues.find(jobs_group);
            return it != m_groups_queues.end() ? &it->second : nullptr;
        }

        //
        // get job items
        // called from parent jobs engine
        //
        inline std::vector<std::shared_ptr<JobsItem>> jobs_get(const std::vector<JobsID> &jobs_ids)
        {
            std::vector<std::shared_ptr<JobsItem>> jobs_items;
            jobs_items.reserve(jobs_ids.size());

            std::unique_lock l(m_lock);

            for (auto &jobs_id : jobs_ids) {
                auto it_j = m_jobs.find(jobs_id);
                if (it_j == m_jobs.end()) {
                    continue;
                }
                jobs_items.push_back(it_j->second);
            }

            return jobs_items; // will be moved
        }

        //
        // add jobs item
        //
        inline JobsID jobs_add(std::shared_ptr<JobsItem> jobs_item)
        {
            std::unique_lock l(m_lock);

            ++m_jobs_seq_id;

            JobsID id       = m_jobs_seq_id;
            jobs_item->m_id = id;
            m_jobs.emplace(id, jobs_item);

            // add it to the timeout queue
            auto it_timeout = m_types_timeouts.find(jobs_item->m_type);
            if (it_timeout != m_types_timeouts.end()) {
                m_timeout_queue.queue().push_delay_for(it_timeout->second, id);
            }

            return id;
        }

        //
        // activate the jobs
        //
        inline std::size_t jobs_activate(const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsID &jobs_id)
        {
            std::size_t ret = 0;

            // optimization to get the queue from the type
            // (instead of getting the group from type from m_config.m_types and then getting the queue from the m_groups_queues)
            auto it_q = m_types_queues.find(jobs_type);
            if (it_q != m_types_queues.end()) {
                auto *q = it_q->second;
                ret     = q->push_back(priority, jobs_id);
            }

            if (ret) {
                m_parent_caller.jobs_activate(jobs_type, jobs_id);
            } else {
                jobs_del(jobs_id);
            }
            return ret;
        }

        //
        // delete jobs item
        //
        inline void jobs_del(const JobsID &jobs_id)
        {
            std::unique_lock l(m_lock);
            m_jobs.erase(jobs_id);
        }

    private:
        // some prevention
        jobs_queue(const jobs_queue &)            = delete;
        jobs_queue(jobs_queue &&)                 = delete;
        jobs_queue &operator=(const jobs_queue &) = delete;
        jobs_queue &operator=(jobs_queue &&__t)   = delete;

    private:
        //
        // inner thread function for delayed items
        // called from m_delayed_items
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

        //
        // inner thread function for timeout items
        // called from m_timeout_queue
        //
        using JobsQueueTimeout = small::time_queue_thread<JobsID, ThisJobsQueue>;
        friend JobsQueueTimeout;

        inline std::size_t push_back(std::vector<JobsID> &&jobs_ids)
        {
            m_parent_caller.jobs_timeout(jobs_ids);
            return jobs_ids.size();
        }

    private:
        //
        // members
        //
        mutable small::base_lock                                 m_lock;           // global locker
        std::atomic<JobsID>                                      m_jobs_seq_id{};  // to get the next jobs id
        std::unordered_map<JobsID, std::shared_ptr<JobsItem>>    m_jobs;           // current jobs
        std::unordered_map<JobsGroupT, JobsQueue>                m_groups_queues;  // map of queues by group
        std::unordered_map<JobsTypeT, JobsQueue *>               m_types_queues;   // optimize to have queues by type (which reference queues by group)
        std::unordered_map<JobsTypeT, std::chrono::milliseconds> m_types_timeouts; // timeouts for types

        JobQueueDelayedT m_delayed_items{*this}; // queue of delayed items

        JobsQueueTimeout m_timeout_queue{*this}; // for timeout elements

        ParentCallerT &m_parent_caller; // jobs engine
    };
} // namespace small::jobsimpl
