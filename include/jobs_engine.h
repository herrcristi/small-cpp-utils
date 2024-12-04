#pragma once

#include "worker_thread.h"

// using qc = std::pair<int, std::string>;
// ...
// //

namespace small {
    //
    // small class for worker threads
    //
    struct config_jobs_engine
    {
        int threads_count{1}; // how many threads for processing
        int bulk_count{1};    // how many objects are processed at once
    };

    template <class T>
    class jobs_engine
    {
    public:
        //
        // jobs_engine
        //
        template <typename _Callable, typename... Args>
        jobs_engine(const config_jobs_engine config, _Callable function, Args... extra_parameters)
            : m_config(config),
              m_processing_function(std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*item*/, std::forward<Args>(extra_parameters)...))
        {
            // auto start threads if count > 0 otherwise threads should be manually started
            if (config.threads_count) {
                start_threads(config.threads_count);
            }
        }

        ~jobs_engine()
        {
            wait();
        }

        // clang-format off
        // size of active items
        inline size_t   size        () { return m_queue_items.size();  }
        // empty
        inline bool     empty       () { return size() == 0; }
        // clear
        inline void     clear       () { m_queue_items.clear(); }
        
        // size of delayed items
        inline size_t   size_delayed() { return m_delayed_items.size();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { m_delayed_items.clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_engine<T>> m...)
        inline void     lock        () { m_queue_items.lock(); }
        inline void     unlock      () { m_queue_items.unlock(); }
        inline bool     try_lock    () { return m_queue_items.try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            // check first if threads were already started
            if (m_threads_flag_created.load() == true) {
                return;
            }

            // lock
            std::unique_lock mlock(m_queue_items);

            // save setting
            m_config.threads_count = threads_count;

            // check again
            if (m_threads_flag_created.load() == true) {
                return;
            }

            // create threads and save their future results
            m_threads_futures.reserve(threads_count + 1);
            m_threads_futures.resize(threads_count);
            for (auto &tf : m_threads_futures) {
                tf = std::async(std::launch::async, &jobs_engine::thread_function, this);
            }
            m_threads_futures.push_back(std::async(std::launch::async, &jobs_engine::thread_function_delayed, this));

            // mark threads were created
            m_threads_flag_created.store(true);
        }

        //
        // add items to be processed
        // push_back
        //
        inline void push_back(const T &t)
        {
            if (is_exit()) {
                return;
            }

            m_queue_items.push_back(t);
        }

        // push back with move semantics
        inline void push_back(T &&t)
        {
            if (is_exit()) {
                return;
            }

            m_queue_items.push_back(std::forward<T>(t));
        }

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const T &elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_for(__rtime, elem);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const T &elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_until(__atime, elem);
        }

        // push_back move semantics
        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, T &&elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_for(__rtime, std::forward<T>(elem));
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, T &&elem)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.push_delay_until(__atime, std::forward<T>(elem));
        }

        // emplace_back
        template <typename... _Args>
        inline void emplace_back(_Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            m_queue_items.emplace_back(std::forward<_Args>(__args)...);
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline void emplace_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, _Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.emplace_delay_for(__rtime, std::forward<T>(__args)...);
        }

        template </* typename _Clock, typename _Duration, */ typename... _Args> // avoid time_casting from one clock to another
        inline void emplace_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, _Args &&...__args)
        {
            if (is_exit()) {
                return;
            }

            m_delayed_items.emplace_delay_until(__atime, std::forward<T>(__args)...);
        }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { m_queue_items.signal_exit_force();     m_delayed_items.signal_exit_force(); }
        inline void signal_exit_when_done   ()  { m_delayed_items.signal_exit_when_done(); /*when the delayed will be finished will signal the queue items to exit when done*/ }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_queue_items.is_exit_force() || m_delayed_items.is_exit_force(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();
            for (auto &th : m_threads_futures) {
                th.wait();
            }
            return EnumLock::kExit;
        }

        // wait some time then signal exit
        template <typename _Rep, typename _Period>
        inline EnumLock wait_for(const std::chrono::duration<_Rep, _Period> &__rtime)
        {
            using __dur = typename std::chrono::system_clock::duration;
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
            for (auto &th : m_threads_futures) {
                auto ret = th.wait_until(__atime);
                if (ret == std::future_status::timeout) {
                    return EnumLock::kTimeout;
                }
            }
            return EnumLock::kExit;
        }

    private:
        // some prevention
        jobs_engine(const jobs_engine &) = delete;
        jobs_engine(jobs_engine &&) = delete;

        jobs_engine &operator=(const jobs_engine &) = delete;
        jobs_engine &operator=(jobs_engine &&__t) = delete;

    private:
        //
        // inner thread function for active items
        //
        inline void thread_function()
        {
            std::vector<T> vec_elems;
            const int bulk_count = std::max(m_config.bulk_count, 1);
            while (true) {
                // wait
                small::EnumLock ret = m_queue_items.wait_pop_front(vec_elems, bulk_count);

                if (ret == small::EnumLock::kExit) {
                    // force stop
                    break;
                } else if (ret == small::EnumLock::kTimeout) {
                    // nothing to do
                } else if (ret == small::EnumLock::kElement) {
                    // process
                    m_processing_function(vec_elems); // bind the std::placeholders::_1
                }
                small::sleepMicro(1);
            }
        }

        //
        // inner thread function for delayed items
        //
        inline void thread_function_delayed()
        {
            std::vector<T> vec_elems;
            const int bulk_count = std::max(m_config.bulk_count, 1);
            while (true) {
                // wait
                small::EnumLock ret = m_delayed_items.wait_pop(vec_elems, bulk_count);

                if (ret == small::EnumLock::kExit) {
                    m_queue_items.signal_exit_when_done();
                    // force stop
                    break;
                } else if (ret == small::EnumLock::kTimeout) {
                    // nothing to do
                } else if (ret == small::EnumLock::kElement) {
                    // put them to active items queue // TODO add support for push vec
                    for (auto &elem : vec_elems) {
                        push_back(std::move(elem));
                    }
                }
                small::sleepMicro(1);
            }
        }

    private:
        //
        // members
        //
        config_jobs_engine m_config;                                         // config
        std::vector<std::future<void>> m_threads_futures;                    // threads futures (needed to wait for)
        std::atomic<bool> m_threads_flag_created{};                          // threads flag
        small::lock_queue<T> m_queue_items;                                  // queue of items
        small::time_queue<T> m_delayed_items;                                // queue of delayed items
        std::function<void(const std::vector<T> &)> m_processing_function{}; // processing Function
    };
} // namespace small
