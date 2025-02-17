#pragma once

#include <unordered_map>

#include "../prio_queue.h"
#include "../time_queue_thread.h"

#include "jobs_item_impl.h"

namespace small::jobsimpl {
    //
    // small queue helper class for jobs (parent caller must implement 'jobs_add', 'jobs_schedule', 'jobs_finished', 'jobs_cancelled')
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename JobsGroupT, typename JobsPrioT, typename ParentCallerT>
    class jobs_queue
    {
    public:
        using JobsItem  = typename small::jobsimpl::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;
        using JobsID    = typename JobsItem::JobsID;
        using JobsQueue = small::prio_queue<JobsID, JobsPrioT>;

        using ThisJobsQueue = jobs_queue<JobsTypeT, JobsRequestT, JobsResponseT, JobsGroupT, JobsPrioT, ParentCallerT>;

        using JobDelayedItems  = std::pair<JobsPrioT, JobsID>;
        using JobQueueDelayedT = small::time_queue_thread<JobDelayedItems, ThisJobsQueue>;

        using TimeClock    = typename small::time_queue<JobDelayedItems>::TimeClock;
        using TimeDuration = typename small::time_queue<JobDelayedItems>::TimeDuration;

    private:
        friend ParentCallerT;

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
        inline bool config_jobs_type(const JobsTypeT &jobs_type, const JobsGroupT &jobs_group)
        {
            auto it_g = m_groups_queues.find(jobs_group);
            if (it_g == m_groups_queues.end()) {
                return false;
            }

            m_types_queues[jobs_type] = &it_g->second;
            return true;
        }

        //
        // only this part is public
        //
    public:
        //
        // add items to be processed
        // push_back only add the jobs item but does not start it
        //
        inline std::size_t push_back(const JobsTypeT &jobs_type, const JobsRequestT &job_req, JobsID *jobs_id)
        {
            // this job should be manually started by calling jobs_start
            return push_back(std::make_shared<JobsItem>(jobs_type, job_req), jobs_id);
        }

        inline std::size_t push_back(std::shared_ptr<JobsItem> jobs_item, JobsID *jobs_id)
        {
            if (is_exit()) {
                return 0;
            }

            // this job should be manually started by calling jobs_start
            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }

            return 1;
        }

        inline std::size_t push_back(const std::vector<std::shared_ptr<JobsItem>> &jobs_items, std::vector<JobsID> *jobs_ids)
        {
            if (is_exit()) {
                return 0;
            }

            // this jobs should be manually started by calling jobs_start
            std::unique_lock l(m_lock);

            std::size_t count = 0;
            if (jobs_ids) {
                jobs_ids->reserve(jobs_items.size());
                jobs_ids->clear();
            }
            JobsID jobs_id{};
            for (auto &jobs_item : jobs_items) {
                auto ret = push_back(jobs_item, &jobs_id);
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
        inline std::size_t push_back(const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id)
        {
            // this job should be manually started by calling jobs_start
            return push_back(std::make_shared<JobsItem>(jobs_type, std::forward<JobsRequestT>(jobs_req)), jobs_id);
        }

        //
        // push back and start the job
        //
        inline std::size_t push_back_and_start(const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &job_req, JobsID *jobs_id = nullptr)
        {
            return push_back_and_start(priority, std::make_shared<JobsItem>(jobs_type, job_req), jobs_id);
        }

        inline std::size_t push_back_and_start(const JobsPrioT &priority, std::shared_ptr<JobsItem> jobs_item, JobsID *jobs_id = nullptr)
        {
            {
                std::unique_lock l(m_lock);

                JobsID id{};
                auto   ret = push_back(jobs_item, &id);
                if (!ret) {
                    return ret;
                }

                if (jobs_id) {
                    *jobs_id = id;
                }
            } // end lock

            // start the job (must be under no lock)
            return jobs_start(priority, jobs_item);
        }

        inline std::size_t push_back_and_start(const JobsPrioT &priority, const std::vector<std::shared_ptr<JobsItem>> &jobs_items, std::vector<JobsID> *jobs_ids = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            {
                std::unique_lock l(m_lock);

                std::vector<JobsID> ids;

                auto ret = push_back(jobs_items, &ids);
                if (!ret) {
                    return ret;
                }

                if (jobs_ids) {
                    *jobs_ids = std::move(ids);
                }
            } // end lock

            // start the jobs (must be under no lock)
            return jobs_start(priority, jobs_items);
        }

        // push_back move semantics
        inline std::size_t push_back_and_start(const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_and_start(priority, std::make_shared<JobsItem>(jobs_type, std::forward<JobsRequestT>(jobs_req)), jobs_id);
        }

        //
        // helper push_back a new job child and link with the parent
        //
        inline std::size_t push_back_child(const JobsID &parent_jobs_id, const JobsTypeT &child_jobs_type, const JobsRequestT &child_job_req, JobsID *child_jobs_id)
        {
            // this job should be manually started by calling jobs_start
            return push_back_child(parent_jobs_id, std::make_shared<JobsItem>(child_jobs_type, child_job_req), child_jobs_id);
        }

        inline std::size_t push_back_child(const JobsID &parent_jobs_id, std::shared_ptr<JobsItem> child_jobs_item, JobsID *child_jobs_id)
        {
            if (is_exit()) {
                return 0;
            }

            std::unique_lock l(m_lock);

            auto parent_jobs_item = jobs_get(parent_jobs_id);
            if (!parent_jobs_item) {
                return 0;
            }

            // this job should be manually started by calling jobs_start
            JobsID id{};
            auto   ret = push_back(child_jobs_item, &id);
            if (!ret) {
                return ret;
            }

            if (child_jobs_id) {
                *child_jobs_id = id;
            }

            jobs_parent_child(parent_jobs_item, child_jobs_item);
            return 1;
        }

        inline std::size_t push_back_child(const JobsID &parent_jobs_id, const std::vector<std::shared_ptr<JobsItem>> &children_jobs_items, std::vector<JobsID> *children_jobs_ids)
        {
            if (is_exit()) {
                return 0;
            }

            // this job should be manually started by calling jobs_start
            std::unique_lock l(m_lock);

            std::size_t count = 0;
            if (children_jobs_ids) {
                children_jobs_ids->reserve(children_jobs_items.size());
                children_jobs_ids->clear();
            }
            JobsID child_jobs_id{};
            for (auto &child_jobs_item : children_jobs_items) {
                auto ret = push_back_child(parent_jobs_id, child_jobs_item, &child_jobs_id);
                if (ret) {
                    if (children_jobs_ids) {
                        children_jobs_ids->push_back(child_jobs_id);
                    }
                }
                count += ret;
            }
            return count;
        }

        // push_back_child move semantics
        inline std::size_t push_back_child(const JobsID &parent_jobs_id, const JobsTypeT &child_jobs_type, JobsRequestT &&child_jobs_req, JobsID *child_jobs_id)
        {
            // this job should be manually started by calling jobs_start
            return push_back_child(parent_jobs_id, std::make_shared<JobsItem>(child_jobs_type, std::forward<JobsRequestT>(child_jobs_req)), child_jobs_id);
        }

        //
        // helper push_back a new job child and link with the parent and start the child job
        //
        inline std::size_t push_back_and_start_child(const JobsID &parent_jobs_id, const JobsPrioT &child_priority, const JobsTypeT &child_jobs_type, const JobsRequestT &child_job_req, JobsID *child_jobs_id = nullptr)
        {
            return push_back_and_start_child(parent_jobs_id, child_priority, std::make_shared<JobsItem>(child_jobs_type, child_job_req), child_jobs_id);
        }

        inline std::size_t push_back_and_start_child(const JobsID &parent_jobs_id, const JobsPrioT &child_priority, std::shared_ptr<JobsItem> child_jobs_item, JobsID *child_jobs_id = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            {
                std::unique_lock l(m_lock);

                JobsID id{};
                auto   ret = push_back_child(parent_jobs_id, child_jobs_item, &id);
                if (!ret) {
                    return ret;
                }

                if (child_jobs_id) {
                    *child_jobs_id = id;
                }
            } // end lock

            // start the job (must be under no lock)
            return jobs_start(child_priority, child_jobs_item);
        }

        inline std::size_t push_back_and_start_child(const JobsID &parent_jobs_id, const JobsPrioT &children_priority, const std::vector<std::shared_ptr<JobsItem>> &children_jobs_items, std::vector<JobsID> *children_jobs_ids = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            {
                std::unique_lock l(m_lock);

                std::vector<JobsID> ids;

                auto ret = push_back_child(parent_jobs_id, children_jobs_items, &ids);
                if (!ret) {
                    return ret;
                }

                if (children_jobs_ids) {
                    *children_jobs_ids = std::move(ids);
                }
            } // end lock

            // start the jobs
            return jobs_start(children_priority, children_jobs_items);
        }

        // push_back move semantics
        inline std::size_t push_back_and_start_child(const JobsID &parent_jobs_id, const JobsPrioT &child_priority, const JobsTypeT &child_jobs_type, JobsRequestT &&child_jobs_req, JobsID *child_jobs_id = nullptr)
        {
            return push_back_and_start_child(parent_jobs_id, child_priority, std::make_shared<JobsItem>(child_jobs_type, std::forward<JobsRequestT>(child_jobs_req)), child_jobs_id);
        }

        // no emplace_back do to returning the jobs_id

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline std::size_t push_back_and_start_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_and_start_delay_for(__rtime, priority, std::make_shared<JobsItem>(jobs_type, jobs_req), jobs_id);
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_back_and_start_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, std::shared_ptr<JobsItem> jobs_item, JobsID *jobs_id = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_for(__rtime, {priority, id});
        }

        template <typename _Rep, typename _Period>
        inline std::size_t push_back_and_start_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_and_start_delay_for(__rtime, priority, std::make_shared<JobsItem>(jobs_type, std::forward<JobsRequestT>(jobs_req)), jobs_id);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t push_back_and_start_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsTypeT &jobs_type, const JobsRequestT &jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_and_start_delay_until(__atime, priority, std::make_shared<JobsItem>(jobs_type, jobs_req), jobs_id);
        }

        inline std::size_t push_back_and_start_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, std::shared_ptr<JobsItem> jobs_item, JobsID *jobs_id = nullptr)
        {
            if (is_exit()) {
                return 0;
            }

            auto id = jobs_add(jobs_item);
            if (jobs_id) {
                *jobs_id = id;
            }
            return m_delayed_items.queue().push_delay_until(__atime, {priority, id});
        }

        inline std::size_t push_back_and_start_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsTypeT &jobs_type, JobsRequestT &&jobs_req, JobsID *jobs_id = nullptr)
        {
            return push_back_and_start_delay_until(__atime, priority, std::make_shared<JobsItem>(jobs_type, std::forward<JobsRequestT>(jobs_req)), jobs_id);
        }

        //
        // jobs start with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline std::size_t jobs_start_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const JobsPrioT &priority, const JobsID &jobs_id)
        {
            if (is_exit()) {
                return 0;
            }

            return m_delayed_items.queue().push_delay_for(__rtime, {priority, jobs_id});
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline std::size_t jobs_start_delay_until(const std::chrono::time_point<TimeClock, TimeDuration> &__atime, const JobsPrioT &priority, const JobsID &jobs_id)
        {
            if (is_exit()) {
                return 0;
            }

            return m_delayed_items.queue().push_delay_until(__atime, {priority, jobs_id});
        }

    private:
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
        //
        // get jobs group queue
        //
        inline JobsQueue *get_jobs_group_queue(const JobsGroupT &jobs_group)
        {
            auto it = m_groups_queues.find(jobs_group);
            return it != m_groups_queues.end() ? &it->second : nullptr;
        }

        //
        // get jobs type queue
        //
        inline JobsQueue *get_jobs_type_queue(const JobsTypeT &jobs_type)
        {
            // optimization to get the queue from the type
            // (instead of getting the group from type from m_config.m_types and then getting the queue from the m_groups_queues)
            auto it = m_types_queues.find(jobs_type);
            return it != m_types_queues.end() ? it->second : nullptr;
        }

        //
        // get job items
        //
        inline std::shared_ptr<JobsItem> jobs_get(const JobsID &jobs_id)
        {
            std::unique_lock l(m_lock);

            auto it_j = m_jobs.find(jobs_id);
            if (it_j == m_jobs.end()) {
                return nullptr;
            }
            return it_j->second;
        }

        inline std::vector<std::shared_ptr<JobsItem>> jobs_get(const std::vector<JobsID> &jobs_ids)
        {
            std::vector<std::shared_ptr<JobsItem>> jobs_items;
            jobs_items.reserve(jobs_ids.size());

            std::unique_lock l(m_lock);

            for (auto &jobs_id : jobs_ids) {
                auto jobs_item = jobs_get(jobs_id);
                if (jobs_item) {
                    jobs_items.push_back(jobs_item);
                }
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

            // call parent for extra processing
            m_parent_caller.jobs_add(jobs_item);

            return id;
        }

        //
        // start the jobs
        //
        inline std::size_t jobs_start(const JobsPrioT &priority, const JobsID &jobs_id)
        {
            auto jobs_item = jobs_get(jobs_id);
            if (!jobs_item) {
                return 0;
            }
            return jobs_start(priority, jobs_item);
        }

        inline std::size_t jobs_start(const JobsPrioT &priority, std::shared_ptr<JobsItem> jobs_item)
        {
            std::size_t ret = 0;
            if (!jobs_item || !jobs_item->m_id) {
                return ret;
            }

            auto *q = get_jobs_type_queue(jobs_item->m_type);
            if (q) {
                ret = q->push_back(priority, jobs_item->m_id);
            }

            if (ret) {
                m_parent_caller.jobs_schedule(jobs_item);
            } else {
                // call parent for extra processing and erasing
                m_parent_caller.jobs_cancelled(jobs_item);
            }
            return ret;
        }

        inline std::size_t jobs_start(const JobsPrioT &priority, const std::vector<JobsID> &jobs_ids)
        {
            return jobs_start(priority, jobs_get(jobs_ids));
        }

        inline std::size_t jobs_start(const JobsPrioT &priority, const std::vector<std::shared_ptr<JobsItem>> &jobs_items)
        {
            std::size_t count = 0;
            for (auto &jobs_item : jobs_items) {
                auto ret = jobs_start(priority, jobs_item);
                if (ret) {
                    ++count;
                }
            }
            return count;
        }

        //
        // erase jobs item
        //
        inline void jobs_erase(const JobsID &jobs_id)
        {
            std::unique_lock l(m_lock);

            auto jobs_item = jobs_get(jobs_id);
            if (!jobs_item) {
                // already deleted
                return;
            }
            // if not a final state, set it to cancelled (in case it is executing at this point)
            if (!JobsItem::is_state_complete(jobs_item->get_state())) {
                jobs_item->set_state_cancelled();
            }

            m_jobs.erase(jobs_id);

            // recursive delete all children
            for (auto &child_jobs_id : jobs_item->m_childrenIDs) {
                jobs_erase(child_jobs_id);
            }
        }

        //
        // set relationship parent-child
        //
        inline std::size_t jobs_parent_child(const JobsID &parent_jobs_id, const JobsID &child_jobs_id)
        {
            std::unique_lock l(m_lock);

            auto parent_jobs_item = jobs_get(parent_jobs_id);
            if (!parent_jobs_item) {
                return 0;
            }
            auto child_jobs_item = jobs_get(child_jobs_id);
            if (!child_jobs_item) {
                return 0;
            }

            jobs_parent_child(parent_jobs_item, child_jobs_item);
            return 1;
        }

        inline void jobs_parent_child(std::shared_ptr<JobsItem> parent_jobs_item, std::shared_ptr<JobsItem> child_jobs_item)
        {
            std::unique_lock l(m_lock);
            parent_jobs_item->add_child(child_jobs_item->m_id);
            child_jobs_item->add_parent(parent_jobs_item->m_id);
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
            for (auto &[priority, jobs_id] : items) {
                count += jobs_start(priority, jobs_id);
            }
            return count;
        }

    private:
        //
        // members
        //
        mutable small::base_lock                              m_lock;          // global locker
        std::atomic<JobsID>                                   m_jobs_seq_id{}; // to get the next jobs id
        std::unordered_map<JobsID, std::shared_ptr<JobsItem>> m_jobs;          // current jobs
        std::unordered_map<JobsGroupT, JobsQueue>             m_groups_queues; // map of queues by group
        std::unordered_map<JobsTypeT, JobsQueue *>            m_types_queues;  // optimize to have queues by type (which reference queues by group)

        JobQueueDelayedT m_delayed_items{*this}; // queue of delayed items

        ParentCallerT &m_parent_caller; // jobs engine
    };
} // namespace small::jobsimpl
