#pragma once

#include <atomic>
#include <deque>
#include <optional>
#include <unordered_map>
#include <utility>

#include "base_wait_pop.h"

// a queue with prirorites elements having antistarvation mechanism
//
// small::prio_queue<int> q;
// ...
// q.push_back( small::EnumPriorities::kNormal, 1 );
// ...
//
// // on some thread
// int e = 0;
// auto ret = q.wait_pop_front( &e ); // ret can be small::EnumLock::kExit, small::EnumLock::kTimeout or ret == small::EnumLock::kElement
// //auto ret = q.wait_pop_front_for( std::chrono::minutes( 1 ), &e );
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
    // priorities
    //
    enum class EnumPriorities : unsigned int
    {
        kHighest = 0,
        kHigh,
        kNormal,
        kLow,
        kLowest
    };

    template <typename PrioT = EnumPriorities>
    struct config_prio_queue
    {
        std::vector<std::pair<PrioT, unsigned int /*ratio*/>> priorities; // list of used priorities ordered from high to low
                                                                          // ex: { { kHigh, 3 }, { kNormal, 3 }, { kLow, 0 } }
                                                                          // ratio 3:1, means after 3 High priorities execute 1 Normal, after 3 Normal execute 1 Low, etc
    };

    // setup default for EnumPriorities
    template <>
    struct config_prio_queue<EnumPriorities>
    {
        std::vector<std::pair<EnumPriorities, unsigned int /*ratio*/>> priorities{
            {small::EnumPriorities::kHighest, 3},
            {small::EnumPriorities::kHigh, 3},
            {small::EnumPriorities::kNormal, 3},
            {small::EnumPriorities::kLow, 3},
            {small::EnumPriorities::kLowest, 0}};
    };

    //
    // queue for priorities
    //
    template <typename T, typename PrioT = EnumPriorities>
    class prio_queue
    {
    public:
        //
        // prio_queue
        //
        explicit prio_queue(config_prio_queue<PrioT> config = {})
            : m_config(config)
        {
            // create queues
            for (auto &[prio, ratio] : m_config.priorities) {
                m_prio_queues[prio];
            }
        }

        prio_queue(const prio_queue &o) : prio_queue() { operator=(o); };
        prio_queue(prio_queue &&o) noexcept : prio_queue() { operator=(std::move(o)); };

        prio_queue &operator=(const prio_queue &o)
        {
            std::scoped_lock l(m_wait, o.m_wait);
            m_config      = o.m_config;
            m_prio_queues = o.m_prio_queues;
            return *this;
        }
        prio_queue &operator=(prio_queue &&o) noexcept
        {
            std::scoped_lock l(m_wait, o.m_wait);
            m_config      = std::move(o.m_config);
            m_prio_queues = std::move(o.m_prio_queues);
            return *this;
        }

        //
        // size
        //
        inline size_t size()
        {
            std::unique_lock l(m_wait);

            std::size_t total = 0;
            for (auto &[prio, q] : m_prio_queues) {
                total += q.size();
            }
            return total;
        }

        inline bool empty() { return size() == 0; }

        inline size_t size(const PrioT priority)
        {
            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(priority);
            return it != m_prio_queues.end() ? it->second.size() : 0;
        }

        inline bool empty(const PrioT priority) { return size(priority) == 0; }

        //
        // removes elements from queue
        //
        inline void clear()
        {
            std::unique_lock l(m_wait);
            for (auto &[prio, q] : m_prio_queues) {
                q.clear();
            }
        }

        inline void clear(const PrioT priority)
        {
            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(priority);
            if (it != m_prio_queues.end()) {
                it->second.clear();
            }
        }

        // clang-format off
        // use it as locker 
        inline void lock        () { m_wait.lock(); }
        inline void unlock      () { m_wait.unlock(); }
        inline bool try_lock    () { return m_wait.try_lock(); }
        // clang-format on

        //
        // push_back
        //
        inline void push_back(const PrioT priority, const T &elem)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(priority);
            if (it == m_prio_queues.end()) {
                return;
            }

            it->second.push_back(elem);
            m_wait.notify_one();
        }

        inline void push_back(const std::pair<PrioT, T> &pair_elem)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(pair_elem.first);
            if (it == m_prio_queues.end()) {
                return;
            }

            it->second.push_back(pair_elem.second);
            m_wait.notify_one();
        }

        inline void push_back(const PrioT priority, const std::vector<T> &elems)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(priority);
            if (it == m_prio_queues.end()) {
                return;
            }

            for (auto &elem : elems) {
                it->second.push_back(elem);
            }
            m_wait.notify_all();
        }

        // push_back move semantics
        inline void push_back(const PrioT priority, T &&elem)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(priority);
            if (it == m_prio_queues.end()) {
                return;
            }

            it->second.push_back(std::forward<T>(elem));
            m_wait.notify_one();
        }

        inline void push_back(std::pair<PrioT, T> &&pair_elem)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(pair_elem.first);
            if (it == m_prio_queues.end()) {
                return;
            }

            it->second.push_back(std::forward<T>(pair_elem.second));
            m_wait.notify_one();
        }

        inline void push_back(const PrioT priority, std::vector<T> &&elems)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(priority);
            if (it == m_prio_queues.end()) {
                return;
            }

            for (auto &elem : elems) {
                it->second.push_back(std::forward<T>(elem));
            }
            m_wait.notify_all();
        }

        // emplace_back
        template <typename... _Args>
        inline void emplace_back(const PrioT priority, _Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            std::unique_lock l(m_wait);

            auto it = m_prio_queues.find(priority);
            if (it == m_prio_queues.end()) {
                return;
            }

            it->second.emplace_back(std::forward<_Args>(__args)...);
            m_wait.notify_one();
        }

        // clang-format off
        //
        // exit
        //
        inline void signal_exit_force   ()  { m_wait.signal_exit_force(); }
        inline bool is_exit_force       ()  { return m_wait.is_exit_force(); }

        inline void signal_exit_when_done() { m_wait.signal_exit_when_done(); }
        inline bool is_exit_when_done   ()  { return m_wait.is_exit_when_done(); }
        
        inline bool is_exit             ()  { return is_exit_force() || is_exit_when_done(); }
        // clang-format on

        //
        // wait pop_front and return that element
        //
        inline EnumLock wait_pop_front(T *elem)
        {
            return m_wait.wait_pop(elem);
        }

        inline EnumLock wait_pop_front(std::vector<T> &vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop(vec_elems, max_count);
        }

        //
        // wait pop_front_for and return that element
        //
        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, T *elem)
        {
            return m_wait.wait_pop_for(__rtime, elem);
        }

        template <typename _Rep, typename _Period>
        inline EnumLock wait_pop_front_for(const std::chrono::duration<_Rep, _Period> &__rtime, std::vector<T> &vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop_for(__rtime, vec_elems, max_count);
        }

        //
        // wait until
        //
        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, T *elem)
        {
            return m_wait.wait_pop_until(__atime, elem);
        }

        template <typename _Clock, typename _Duration>
        inline EnumLock wait_pop_front_until(const std::chrono::time_point<_Clock, _Duration> &__atime, std::vector<T> &vec_elems, int max_count = 1)
        {
            return m_wait.wait_pop_until(__atime, vec_elems, max_count);
        }

    private:
        using BaseWaitPop = small::base_wait_pop<T, small::prio_queue<T>>;
        friend BaseWaitPop;

        struct Stats
        {
            unsigned int m_count_executed{}; // how many times was executed
        };

        // reset higher priority credits
        inline void reset_higher_stats(const PrioT priority)
        {
            for (auto &[prio, ratio] : m_config.priorities) {
                if (prio == priority) {
                    break;
                }
                auto &stats            = m_prio_stats[prio];
                stats.m_count_executed = 0;
            }
        }

        inline void reset_all_stats()
        {
            for (auto &[prio, ratio] : m_config.priorities) {
                auto &stats            = m_prio_stats[prio];
                stats.m_count_executed = 0;
            }
        }

        inline small::WaitFlags pop_front(std::deque<T> &queue, T *elem)
        {
            // get elem
            if (elem) {
                *elem = std::move(queue.front());
            }
            queue.pop_front();

            return small::WaitFlags::kElement;
        }

        // extract from queue
        inline small::WaitFlags test_and_get(T *elem, typename BaseWaitPop::TimePoint * /* time_wait_until */)
        {
            if (is_exit_force()) {
                return small::WaitFlags::kExit_Force;
            }

            std::optional<PrioT> prio_with_non_empty_queue;

            // iterate priorities from high to low
            for (auto &[prio, ratio] : m_config.priorities) {
                auto &queue = m_prio_queues[prio];
                auto &stats = m_prio_stats[prio];

                // save the first priority for which the queue is not empty
                if (!queue.empty() && !prio_with_non_empty_queue) {
                    prio_with_non_empty_queue = prio;
                }

                // check if it has credits
                if (stats.m_count_executed >= ratio) {
                    // no more credit here, go to next priority
                    continue;
                }

                // increase counter for current prio (see above that all previous (with higher priorities) are reseted)
                // do this even if nothing is in the queue (to avoid the kLowest to be executed too quickly with kVeryHigh)
                ++stats.m_count_executed;
                reset_higher_stats(prio);

                if (queue.empty()) {
                    // choose one from previous
                    if (prio_with_non_empty_queue) {
                        queue            = m_prio_queues[prio_with_non_empty_queue.value()];
                        auto &prev_stats = m_prio_stats[prio_with_non_empty_queue.value()];
                        ++prev_stats.m_count_executed;
                    }

                    // all queues are empty so far so go to a lower prio
                    if (queue.empty()) {
                        continue;
                    }
                }

                // get elem
                return pop_front(queue, elem);
            }

            // reset all stats
            reset_all_stats();

            if (prio_with_non_empty_queue) {
                auto &queue      = m_prio_queues[prio_with_non_empty_queue.value()];
                auto &prev_stats = m_prio_stats[prio_with_non_empty_queue.value()];
                ++prev_stats.m_count_executed;

                // get elem
                return pop_front(queue, elem);
            }

            // here all queues are empty
            if (is_exit_when_done()) {
                // exit
                return small::WaitFlags::kExit_When_Done;
            }

            return small::WaitFlags::kWait;
        }

    private:
        //
        // members
        //
        mutable BaseWaitPop                      m_wait{*this}; // implements locks & wait
        config_prio_queue<PrioT>                 m_config;      // config for priorities and ratio of executions
        std::unordered_map<PrioT, Stats>         m_prio_stats;  // keep credits per priority to implement the ratio (ex: 3:1)
        std::unordered_map<PrioT, std::deque<T>> m_prio_queues; // map of queues based on priorities
    };
} // namespace small
