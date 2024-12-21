#pragma once

#include <atomic>
#include <deque>
#include <unordered_map>

#include "base_lock.h"
#include "prio_queue.h"

// a queue for jobs which have a job type and a job info
//
// enum JobType {
//      kJob1
// };
// enum JobGroupType {
//      kJobGroup1
// };
//
// small::jobs_queue<JobType, int, JobGroupType> q;
// q.set_job_type_group( JobType::kJob1, JobGroupType::kJobGroup1 );
// ...
// q.push_back( small::EnumPriorities::kNormal, JobType::kJob1, 1 );
// ...
//
// // on some thread
// std::pair<JobType, int> e{};
// auto ret = q.wait_pop_front( JobGroupType::kJobGroup1, &e );
// // or wait_pop_front_for( std::chrono::minutes( 1 ), JobGroupType::kJobGroup1, &e );
// // ret can be small::EnumLock::kExit, small::EnumLock::kTimeout or ret == small::EnumLock::kElement
// if ( ret == small::EnumLock::kElement )
// {
//      // do something with e
//      ...
// }
//
// ...
// // on main thread no more processing (aborting work)
// q.signal_exit_force(); // q.signal_exit_when_done()
//

namespace small {
    //
    // queue for jobs who have a job type, priority and a job info elem
    // the job types can be grouped by group
    //
    template <typename JobTypeT, typename JobElemT, typename JobGroupT = JobTypeT, typename PrioT = EnumPriorities>
    class jobs_queue
    {
        using JobTypeQueue = small::prio_queue<std::pair<JobTypeT, JobElemT>, PrioT>;

    public:
        //
        // jobs_queue
        //
        explicit jobs_queue(const small::config_prio_queue<PrioT> &config = {})
            : m_prio_config{config}
        {
        }
        jobs_queue(const jobs_queue &o) : jobs_queue() { operator=(o); };
        jobs_queue(jobs_queue &&o) noexcept : jobs_queue() { operator=(std::move(o)); };

        inline jobs_queue &operator=(const jobs_queue &o)
        {
            std::scoped_lock l(m_lock, o.m_lock);
            m_total_count       = o.m_total_count;
            m_jobs_types_groups = o.m_jobs_types_groups;
            m_jobs_group_queues = o.m_jobs_group_queues;
            // rebuild m_jobs_types_queues
            m_jobs_types_queues.clear();
            for (auto &[job_type, job_group] : m_jobs_types_groups) {
                m_jobs_types_queues[job_type] = &m_jobs_group_queues[job_group];
            }
            return *this;
        }
        inline jobs_queue &operator=(jobs_queue &&o) noexcept
        {
            std::scoped_lock l(m_lock, o.m_lock);
            m_total_count       = o.m_total_count;
            m_jobs_types_groups = std::move(o.m_jobs_types_groups);
            m_jobs_group_queues = std::move(o.m_jobs_group_queues);
            m_jobs_types_queues = std::move(o.m_jobs_types_queues);
            return *this;
        }

        //
        // config, map the job_type to a group
        //
        inline void set_job_type_group(const JobTypeT job_type, const JobGroupT job_group)
        {
            // m_jobs_group_queues will be initialized only here so later it can be accessed even without locking
            m_jobs_types_groups[job_type] = job_group;

            auto it_group = m_jobs_group_queues.find(job_group);
            if (it_group == m_jobs_group_queues.end()) {
                m_jobs_group_queues[job_group] = JobTypeQueue{m_prio_config};
            }
            m_jobs_types_queues[job_type] = &m_jobs_group_queues[job_group];
        }

        //
        // size
        //
        inline size_t size()
        {
            return m_total_count.load();
        }

        inline bool empty() { return size() == 0; }

        inline size_t size(const JobGroupT job_group)
        {
            auto it = m_jobs_group_queues.find(job_group);
            return it != m_jobs_group_queues.end() ? it->second.size() : 0;
        }

        inline bool empty(const JobGroupT job_group) { return size(job_group) == 0; }

        //
        // removes elements
        //
        inline void clear()
        {
            for (auto &[job_group, q] : m_jobs_group_queues) {
                std::unique_lock l(q);
                m_total_count.fetch_sub(q.size());
                q.clear();
            }
        }

        inline void clear(const JobGroupT job_group)
        {
            auto it = m_jobs_group_queues.find(job_group);
            if (it != m_jobs_group_queues.end()) {
                std::unique_lock l(it->second);
                m_total_count.fetch_sub(it->second.size());
                it->second.clear();
            }
        }

        // clang-format off
        // use it as locker 
        inline void lock        () { m_lock.lock(); }
        inline void unlock      () { m_lock.unlock(); }
        inline bool try_lock    () { return m_lock.try_lock(); }
        // clang-format on

        //
        // push_back
        //
        inline std::size_t push_back(const PrioT priority, const JobTypeT job_type, const JobElemT &elem)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_job_type_group_queue(job_type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->push_back(priority, {job_type, elem});
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        inline std::size_t push_back(const PrioT priority, const std::pair<JobTypeT, JobElemT> &pair_elem)
        {
            if (is_exit()) {
                return 0;
            }

            auto job_type = pair_elem.first;

            // get queue
            auto q = get_job_type_group_queue(job_type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->push_back(priority, pair_elem);
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        inline std::size_t push_back(const PrioT priority, const JobTypeT job_type, const std::vector<JobElemT> &elems)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_job_type_group_queue(job_type);
            if (!q) {
                return 0;
            }

            std::size_t count = 0;
            for (auto &elem : elems) {
                ++m_total_count; // increase before adding
                auto ret = q->push_back(priority, {job_type, elem});
                if (!ret) {
                    --m_total_count;
                }
                count += ret;
            }
            return count;
        }

        // push_back move semantics
        inline std::size_t push_back(const PrioT priority, const JobTypeT job_type, JobElemT &&elem)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_job_type_group_queue(job_type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->push_back(priority, {job_type, std::forward<JobElemT>(elem)});
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        inline std::size_t push_back(const PrioT priority, std::pair<JobTypeT, JobElemT> &&pair_elem)
        {
            if (is_exit()) {
                return 0;
            }

            auto job_type = pair_elem.first;

            // get queue
            auto q = get_job_type_group_queue(job_type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->push_back(priority, std::forward<std::pair<JobTypeT, JobElemT>>(pair_elem));
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        inline std::size_t push_back(const PrioT priority, const JobTypeT job_type, std::vector<JobElemT> &&elems)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_job_type_group_queue(job_type);
            if (!q) {
                return 0;
            }

            std::size_t count = 0;
            for (auto &elem : elems) {
                ++m_total_count; // increase before adding
                auto ret = q->push_back(priority, {job_type, std::forward<JobElemT>(elem)});
                if (!ret) {
                    --m_total_count;
                }
                count += ret;
            }
            return count;
        }

        // emplace_back
        template <typename... _Args>
        inline std::size_t emplace_back(const PrioT priority, const JobTypeT job_type, _Args &&...__args)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_job_type_group_queue(job_type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->emplace_back(priority, job_type, JobElemT{std::forward<_Args>(__args)...});
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        //
        // exit
        //
        inline void signal_exit_force()
        {
            std::unique_lock l(m_lock);
            m_lock.signal_exit_force();
            for (auto &[job_group, q] : m_jobs_group_queues) {
                q.signal_exit_force();
            }
        }
        inline bool is_exit_force()
        {
            return m_lock.is_exit_force();
        }

        inline void signal_exit_when_done()
        {
            std::unique_lock l(m_lock);
            m_lock.signal_exit_when_done();
            for (auto &[job_group, q] : m_jobs_group_queues) {
                q.signal_exit_when_done();
            }
        }
        inline bool is_exit_when_done()
        {
            return m_lock.is_exit_when_done();
        }

        inline bool is_exit()
        {
            return is_exit_force() || is_exit_when_done();
        }

        //
        // wait pop_front and return that element
        //
        inline EnumLock wait_pop_front(const JobGroupT                job_group,
                                       std::pair<JobTypeT, JobElemT> *elem)
        {
            // get queue
            auto it_q = m_jobs_group_queues.find(job_group);
            if (it_q == m_jobs_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto ret = it_q->second.wait_pop_front(elem);
            if (ret == small::EnumLock::kElement) {
                --m_total_count; // decrease total count
            }

            return ret;
        }

        inline EnumLock wait_pop_front(const JobGroupT                             job_group,
                                       std::vector<std::pair<JobTypeT, JobElemT>> &vec_elems,
                                       int                                         max_count = 1)
        {
            // get queue
            auto it_q = m_jobs_group_queues.find(job_group);
            if (it_q == m_jobs_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto ret = it_q->second.wait_pop_front(vec_elems, max_count);
            if (ret == small::EnumLock::kElement) {
                for (std::size_t i = 0; i < vec_elems.size(); ++i) {
                    --m_total_count; // decrease total count
                }
            }

            return ret;
        }

        //
        // wait pop_front_for and return that element
        //
        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime,
                                           const JobGroupT                             job_group,
                                           std::pair<JobTypeT, JobElemT>              *elem)
        {
            // get queue
            auto it_q = m_jobs_group_queues.find(job_group);
            if (it_q == m_jobs_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto ret = it_q->second.wait_pop_front_for(__rtime, elem);
            if (ret == small::EnumLock::kElement) {
                --m_total_count; // decrease total count
            }

            return ret;
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime,
                                           const JobGroupT                             job_group,
                                           std::vector<std::pair<JobTypeT, JobElemT>> &vec_elems,
                                           int                                         max_count = 1)
        {
            // get queue
            auto it_q = m_jobs_group_queues.find(job_group);
            if (it_q == m_jobs_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto ret = it_q->second.wait_pop_front_for(__rtime, vec_elems, max_count);
            if (ret == small::EnumLock::kElement) {
                for (std::size_t i = 0; i < vec_elems.size(); ++i) {
                    --m_total_count; // decrease total count
                }
            }

            return ret;
        }

        //
        // wait until
        //
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime,
                                             const JobGroupT                                   job_group,
                                             std::pair<JobTypeT, JobElemT>                    *elem)
        {
            // get queue
            auto it_q = m_jobs_group_queues.find(job_group);
            if (it_q == m_jobs_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto ret = it_q->second.wait_pop_front_until(__atime, elem);
            if (ret == small::EnumLock::kElement) {
                --m_total_count; // decrease total count
            }

            return ret;
        }

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime,
                                             const JobGroupT                                   job_group,
                                             std::vector<std::pair<JobTypeT, JobElemT>>       &vec_elems,
                                             int                                               max_count = 1)
        {
            // get queue
            auto it_q = m_jobs_group_queues.find(job_group);
            if (it_q == m_jobs_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto ret = it_q->second.wait_pop_front_until(__atime, vec_elems, max_count);
            if (ret == small::EnumLock::kElement) {
                for (std::size_t i = 0; i < vec_elems.size(); ++i) {
                    --m_total_count; // decrease total count
                }
            }

            return ret;
        }

        //
        // wait for queues to becom empty
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();

            for (auto &[job_group, q] : m_jobs_group_queues) {
                while (!q.empty() && !q.is_exit_force()) {
                    std::this_thread::yield();
                }
                // std::unique_lock l(q);
                // m_queues_exit_condition.wait(l, [q = &q]() -> bool {
                //     return q->empty() || q->is_exit_force();
                // });
            }

            return small::EnumLock::kExit;
        }

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

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_until(const std::chrono::time_point<_Clock, _Duration> &__atime)
        {
            signal_exit_when_done();

            for (auto &[job_group, q] : m_jobs_group_queues) {
                while (!q.empty() && !q.is_exit_force()) {
                    std::this_thread::yield();
                }
                // std::unique_lock l(q);

                //     auto status = m_queues_exit_condition.wait_until(l, __atime, [q = &q]() -> bool {
                //         return q->empty() || q->is_exit_force();
                //     });
                //     if (!status) {
                //         return small::EnumLock::kTimeout;
                //     }
            }

            return small::EnumLock::kExit;
        }

    private:
        // get job type queue from the group queues
        inline JobTypeQueue *get_job_type_group_queue(const JobTypeT job_type)
        {
            // optimization to get the queue from the type
            // instead of getting the group from type from m_jobs_types_groups
            // and then getting the queue from the m_jobs_group_queues
            auto it_q = m_jobs_types_queues.find(job_type);
            if (it_q == m_jobs_types_queues.end()) {
                return nullptr;
            }

            return it_q->second;
        }

    private:
        //
        // members
        //
        mutable small::base_lock                     m_lock;              // global locker
        std::atomic<std::size_t>                     m_total_count{};     // count of all jobs items
        std::unordered_map<JobTypeT, JobGroupT>      m_jobs_types_groups; // map to get the group for a job type
        small::config_prio_queue<PrioT>              m_prio_config;       // config for the priority queue
        std::unordered_map<JobGroupT, JobTypeQueue>  m_jobs_group_queues; // map of queues grouped by group
        std::unordered_map<JobTypeT, JobTypeQueue *> m_jobs_types_queues; // optimize from group to type map of queues
    };
} // namespace small
