#pragma once

#include <atomic>
#include <deque>
#include <unordered_map>

#include "base_lock.h"
#include "prio_queue.h"

// a queue for grouping items which have a type, info and a priority
//
// enum Type {
//      kType1
// };
// enum GroupType {
//      kGroup1
// };
//
// small::group_queue<Type, int, GroupType> q;
// q.add_type_group( Type::kType1, GroupType::kGroup1 );
// ...
// q.push_back( small::EnumPriorities::kNormal, Type::kType1, 1 );
// ...
//
// // on some thread
// std::pair<Type, int> e{};
// auto ret = q.wait_pop_front( GroupType::kGroup1, &e );
// // or wait_pop_front_for( std::chrono::minutes( 1 ), GroupType::kGroup1, &e );
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
    // queue for grouping items that have a type, priority and info elem
    // the types are grouped by group
    //
    template <typename TypeT, typename ElemT, typename GroupT = TypeT, typename PrioT = EnumPriorities>
    class group_queue
    {
        using TypeQueue = small::prio_queue<std::pair<TypeT, ElemT>, PrioT>;

    public:
        //
        // group_queue
        //
        explicit group_queue(const small::config_prio_queue<PrioT> &config = {})
            : m_prio_config{config}
        {
        }
        group_queue(const group_queue &o) : group_queue() { operator=(o); };
        group_queue(group_queue &&o) noexcept : group_queue() { operator=(std::move(o)); };

        inline group_queue &operator=(const group_queue &o)
        {
            std::scoped_lock l(m_lock, o.m_lock);
            m_total_count  = o.m_total_count;
            m_types_groups = o.m_types_groups;
            m_group_queues = o.m__group_queues;
            // rebuild m_types_queues
            m_types_queues.clear();
            for (auto &[type, group] : m_types_groups) {
                m_types_queues[type] = &m_group_queues[group];
            }
            return *this;
        }
        inline group_queue &operator=(group_queue &&o) noexcept
        {
            std::scoped_lock l(m_lock, o.m_lock);
            m_total_count  = o.m_total_count;
            m_types_groups = std::move(o.m_types_groups);
            m_group_queues = std::move(o.m_group_queues);
            m_types_queues = std::move(o.m_types_queues);
            return *this;
        }

        //
        // config, map the type to a group
        //
        inline void add_type_group(const TypeT type, const GroupT group)
        {
            // m_group_queues will be initialized only here so later it can be accessed even without locking
            m_types_groups[type] = group;

            auto it_group = m_group_queues.find(group);
            if (it_group == m_group_queues.end()) {
                m_group_queues[group] = TypeQueue{m_prio_config};
            }
            m_types_queues[type] = &m_group_queues[group];
        }

        //
        // size
        //
        inline size_t size()
        {
            return m_total_count.load();
        }

        inline bool empty() { return size() == 0; }

        inline size_t size(const GroupT group)
        {
            auto it = m_group_queues.find(group);
            return it != m_group_queues.end() ? it->second.size() : 0;
        }

        inline bool empty(const GroupT group) { return size(group) == 0; }

        //
        // removes elements
        //
        inline void clear()
        {
            for (auto &[group, q] : m_group_queues) {
                std::unique_lock l(q);
                m_total_count.fetch_sub(q.size());
                q.clear();
            }
        }

        inline void clear(const GroupT group)
        {
            auto it = m_group_queues.find(group);
            if (it != m_group_queues.end()) {
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
        inline std::size_t push_back(const PrioT priority, const TypeT type, const ElemT &elem)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_type_group_queue(type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->push_back(priority, {type, elem});
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        inline std::size_t push_back(const PrioT priority, const std::pair<TypeT, ElemT> &pair_elem)
        {
            if (is_exit()) {
                return 0;
            }

            auto type = pair_elem.first;

            // get queue
            auto q = get_type_group_queue(type);
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

        inline std::size_t push_back(const PrioT priority, const TypeT type, const std::vector<ElemT> &elems)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_type_group_queue(type);
            if (!q) {
                return 0;
            }

            std::size_t count = 0;
            for (auto &elem : elems) {
                ++m_total_count; // increase before adding
                auto ret = q->push_back(priority, {type, elem});
                if (!ret) {
                    --m_total_count;
                }
                count += ret;
            }
            return count;
        }

        // push_back move semantics
        inline std::size_t push_back(const PrioT priority, const TypeT type, ElemT &&elem)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_type_group_queue(type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->push_back(priority, {type, std::forward<ElemT>(elem)});
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        inline std::size_t push_back(const PrioT priority, std::pair<TypeT, ElemT> &&pair_elem)
        {
            if (is_exit()) {
                return 0;
            }

            auto type = pair_elem.first;

            // get queue
            auto q = get_type_group_queue(type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->push_back(priority, std::forward<std::pair<TypeT, ElemT>>(pair_elem));
            if (!ret) {
                --m_total_count;
            }
            return ret;
        }

        inline std::size_t push_back(const PrioT priority, const TypeT type, std::vector<ElemT> &&elems)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_type_group_queue(type);
            if (!q) {
                return 0;
            }

            std::size_t count = 0;
            for (auto &elem : elems) {
                ++m_total_count; // increase before adding
                auto ret = q->push_back(priority, {type, std::forward<ElemT>(elem)});
                if (!ret) {
                    --m_total_count;
                }
                count += ret;
            }
            return count;
        }

        // emplace_back
        template <typename... _Args>
        inline std::size_t emplace_back(const PrioT priority, const TypeT type, _Args &&...__args)
        {
            if (is_exit()) {
                return 0;
            }

            // get queue
            auto q = get_type_group_queue(type);
            if (!q) {
                return 0;
            }

            ++m_total_count; // increase before adding
            auto ret = q->emplace_back(priority, type, ElemT{std::forward<_Args>(__args)...});
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
            for (auto &[group, q] : m_group_queues) {
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
            for (auto &[group, q] : m_group_queues) {
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
        inline EnumLock wait_pop_front(const GroupT             group,
                                       std::pair<TypeT, ElemT> *elem)
        {
            // get queue
            auto it_q = m_group_queues.find(group);
            if (it_q == m_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto &q   = it_q->second;
            auto  ret = q.wait_pop_front(elem);
            if (ret == small::EnumLock::kElement) {
                --m_total_count; // decrease total count
            }

            return ret;
        }

        inline EnumLock wait_pop_front(const GroupT                          group,
                                       std::vector<std::pair<TypeT, ElemT>> &vec_elems,
                                       int                                   max_count = 1)
        {
            // get queue
            auto it_q = m_group_queues.find(group);
            if (it_q == m_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto &q   = it_q->second;
            auto  ret = q.wait_pop_front(vec_elems, max_count);
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
                                           const GroupT                                group,
                                           std::pair<TypeT, ElemT>                    *elem)
        {
            // get queue
            auto it_q = m_group_queues.find(group);
            if (it_q == m_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto &q   = it_q->second;
            auto  ret = q.wait_pop_front_for(__rtime, elem);
            if (ret == small::EnumLock::kElement) {
                --m_total_count; // decrease total count
            }

            return ret;
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime,
                                           const GroupT                                group,
                                           std::vector<std::pair<TypeT, ElemT>>       &vec_elems,
                                           int                                         max_count = 1)
        {
            // get queue
            auto it_q = m_group_queues.find(group);
            if (it_q == m_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto &q   = it_q->second;
            auto  ret = q.wait_pop_front_for(__rtime, vec_elems, max_count);
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
                                             const GroupT                                      group,
                                             std::pair<TypeT, ElemT>                          *elem)
        {
            // get queue
            auto it_q = m_group_queues.find(group);
            if (it_q == m_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto &q   = it_q->second;
            auto  ret = q.wait_pop_front_until(__atime, elem);
            if (ret == small::EnumLock::kElement) {
                --m_total_count; // decrease total count
            }

            return ret;
        }

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime,
                                             const GroupT                                      group,
                                             std::vector<std::pair<TypeT, ElemT>>             &vec_elems,
                                             int                                               max_count = 1)
        {
            // get queue
            auto it_q = m_group_queues.find(group);
            if (it_q == m_group_queues.end()) {
                return small::EnumLock::kTimeout;
            }

            // wait
            auto &q   = it_q->second;
            auto  ret = q.wait_pop_front_until(__atime, vec_elems, max_count);
            if (ret == small::EnumLock::kElement) {
                for (std::size_t i = 0; i < vec_elems.size(); ++i) {
                    --m_total_count; // decrease total count
                }
            }

            return ret;
        }

        //
        // wait for queues to become empty
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();
            for (auto &[group, q] : m_group_queues) {
                q.wait();
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

            for (auto &[group, q] : m_group_queues) {
                auto status = q.wait_until(__atime);
                if (status == small::EnumLock::kTimeout) {
                    return small::EnumLock::kTimeout;
                }
            }

            return small::EnumLock::kExit;
        }

    private:
        //
        // get type queue from the group queues
        //
        inline TypeQueue *get_type_group_queue(const TypeT type)
        {
            // optimization to get the queue from the type
            // instead of getting the group from type from m_types_groups
            // and then getting the queue from the m_group_queues
            auto it_q = m_types_queues.find(type);
            if (it_q == m_types_queues.end()) {
                return nullptr;
            }

            return it_q->second;
        }

    private:
        //
        // members
        //
        mutable small::base_lock               m_lock;          // global locker
        std::atomic<std::size_t>               m_total_count{}; // count of all items
        std::unordered_map<TypeT, GroupT>      m_types_groups;  // map to get the group for a type
        small::config_prio_queue<PrioT>        m_prio_config;   // config for the priority queue
        std::unordered_map<GroupT, TypeQueue>  m_group_queues;  // map of queues grouped by group
        std::unordered_map<TypeT, TypeQueue *> m_types_queues;  // optimize from group to type map of queues
    };
} // namespace small
