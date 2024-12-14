#pragma once

#include <atomic>
#include <deque>
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
        kCritical = 0,
        kVeryHigh,
        kHigh,
        kNormal,
        kLow,
        kVeryLow
    };

    template <typename PrioT = EnumPriorities>
    struct config_prio_queue
    {
        std::vector<std::pair<PrioT, int /*ratio*/>> priorities; // list of used priorities ordered from high to low
                                                                 // ex: { { kHigh, 3 }, { kNormal, 3 }, { kLow, 0 } }
                                                                 // ratio 3:1, means after 3 High priorities execute 1 Normal, after 3 Normal execute 1 Low, etc
    };

    // setup default for EnumPriorities
    template <>
    struct config_prio_queue<EnumPriorities>
    {
        std::vector<EnumPriorities> priorities{{small::EnumPriorities::kCritical, 3},
                                               {small::EnumPriorities::kVeryHigh, 3},
                                               {small::EnumPriorities::kHigh, 3},
                                               {small::EnumPriorities::kNormal, 3},
                                               {small::EnumPriorities::kLow, 3},
                                               {small::EnumPriorities::kVeryLow, 0}};
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
            for (auto prio : m_config.priotities) {
                m_prio_queues[prio];
            }
        }

        prio_queue(const prio_queue &o) : prio_queue() { operator=(o); };
        prio_queue(prio_queue &&o) noexcept : prio_queue() { operator=(std::move(o)); };

        prio_queue &operator=(const prio_queue &o)
        {
            std::scoped_lock l(m_lock, o.m_lock);
            m_config      = o.m_config;
            m_prio_queues = o.m_prio_queues;
            return *this;
        }
        prio_queue &operator=(prio_queue &&o) noexcept
        {
            std::scoped_lock l(m_lock, o.m_lock);
            m_config      = std::move(o.m_config);
            m_prio_queues = std::move(o.m_prio_queues);
            return *this;
        }

        //
        // size
        //
        inline size_t size()
        {
            std::unique_lock l(m_lock);

            std::size_t total = 0;
            for (auto &[prio, q] : m_prio_queues) {
                total += q.size();
            }
            return total;
        }

        inline bool empty() { return size() == 0; }

        inline size_t size(const PrioT priority)
        {
            std::unique_lock l(m_lock);

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
            m_queue.clear();
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

        inline small::WaitFlags test_and_get(T *elem, typename BaseWaitPop::TimePoint * /* time_wait_until */)
        {
            // TODO
            if (is_exit_force()) {
                return small::WaitFlags::kExit_Force;
            }

            // check queue size
            if (m_queue.empty()) {
                if (is_exit_when_done()) {
                    // exit but dont reset event to allow other threads to exit
                    return small::WaitFlags::kExit_When_Done;
                }

                return small::WaitFlags::kNone;
            }

            // get elem
            if (elem) {
                *elem = std::move(m_queue.front());
            }
            m_queue.pop_front();

            return small::WaitFlags::kElement;
        }

    private:
        //
        // members
        //
        mutable BaseWaitPop                      m_wait{*this}; // implements locks & wait
        config_prio_queue<PrioT>                 m_config;      // config for priorities and ratio of executions
        std::unordered_map<PrioT, std::deque<T>> m_prio_queues; // map of queues based on priorities
    };
} // namespace small
