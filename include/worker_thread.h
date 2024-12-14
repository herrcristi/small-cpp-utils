#pragma once

#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "lock_queue_thread.h"
#include "time_queue_thread.h"

// using qc = std::pair<int, std::string>;
// ...
// // with a lambda for processing working function
// small::worker_thread<qc> workers( {.threads_count = 0, .bulk_count = 2}, []( auto& w/*this*/, const auto& items, auto b/*extra param*/ ) -> void
// {
//     ...
//     {
//         std::unique_lock mlock( w ); // use worker_thread to lock
//         ...
//         for (auto &[i, s] : items) {
//              std::cout << "thread " << std::this_thread::get_id() << " processing " << i << " " << s << " b=" << b << "\n";
//         }
//     }
//     ...
// }, 5 /*extra param*/ );
//
// workers.start_threads(2); // manual start threads
// ...
// // or like this
// small::worker_thread<qc> workers2( {/*default 1 thread*/}, WorkerThreadFunction() );
// ...
// // where WorkerThreadFunction can be
// struct WorkerThreadFunction
// {
//     using qc = std::pair<int, std::string>;
//     void operator()( small::worker_thread<qc>& w /*worker_thread*/, const std::vector<qc>& items )
//     {
//         ...
//         // add extra in queue
//         // w.push_back(...)
//
//         small::sleep( 3000 );
//     }
// };
// ..
// ...
// workers.push_back( { 1, "a" } );
// workers.push_back( std::make_pair( 2, "b" ) );
// workers.emplace_back( 3, "e" );
// workers.push_back_delay_for( std::chrono::milliseconds(300), { 4, "f" } );
// ...
// // no more work, wait to be finished
// auto ret = workers.wait_for( std::chrono::seconds(30) ); // auto ret = workers.wait();
// if  ( ret ==  small::EnumLock:: kTimeout ) {
//     // when finishing after signal_exit_force the work is aborted
//     workers.signal_exit_force();
// }
// //

namespace small {
    //
    // small class for worker threads
    //
    struct config_worker_thread
    {
        int threads_count{1}; // how many threads for processing
        int bulk_count{1};    // how many objects are processed at once
    };

    template <typename T>
    class worker_thread
    {
    public:
        //
        // worker_thread
        //
        template <typename _Callable, typename... Args>
        worker_thread(const config_worker_thread config, _Callable function, Args... extra_parameters)
            : m_config(config),
              m_processing_function(std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*item*/, std::forward<Args>(extra_parameters)...))
        {
            // auto start threads if count > 0 otherwise threads should be manually started
            if (config.threads_count) {
                start_threads(config.threads_count);
            }
        }

        ~worker_thread()
        {
            wait();
        }

        // clang-format off
        // size of active items
        inline size_t   size        () { return m_queue_items.queue().size();  }
        // empty
        inline bool     empty       () { return size() == 0; }
        // clear
        inline void     clear       () { m_queue_items.queue().clear(); }
        
        // size of delayed items
        inline size_t   size_delayed() { return m_delayed_items.queue().size();  }
        // empty
        inline bool     empty_delayed() { return size_delayed() == 0; }
        // clear
        inline void     clear_delayed() { m_delayed_items.queue().clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:worker_thread<T>> m...)
        inline void     lock        () { m_queue_items.queue().lock(); }
        inline void     unlock      () { m_queue_items.queue().unlock(); }
        inline bool     try_lock    () { return m_queue_items.queue().try_lock(); }
        // clang-format on

        //
        // create threads
        //
        inline void start_threads(const int threads_count /* = 1 */)
        {
            // lock
            std::unique_lock mlock(m_queue_items.queue());

            // save setting only if it is increasing
            if (threads_count > m_config.threads_count) {
                m_config.threads_count = threads_count;
            }

            // create threads and save their future results
            m_queue_items.start_threads(m_config.threads_count);

            // create thread for time queue
            m_delayed_items.start_threads();
        }

        //
        // add items to be processed
        // push_back
        //
        inline void push_back(const T &t)
        {
            m_queue_items.queue().push_back(t);
        }

        inline void push_back(const std::vector<T> &items)
        {
            m_queue_items.queue().push_back(items);
        }

        // push back with move semantics
        inline void push_back(T &&t)
        {
            m_queue_items.queue().push_back(std::forward<T>(t));
        }

        inline void push_back(std::vector<T> &&items)
        {
            m_queue_items.queue().push_back(std::forward<std::vector<T>>(items));
        }

        //
        // push_back with specific timeings
        //
        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const T &elem)
        {
            m_delayed_items.queue().push_delay_for(__rtime, elem);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const T &elem)
        {
            m_delayed_items.queue().push_delay_until(__atime, elem);
        }

        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, const std::vector<T> &elems)
        {
            m_delayed_items.queue().push_delay_for(__rtime, elems);
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, const std::vector<T> &elems)
        {
            m_delayed_items.queue().push_delay_until(__atime, elems);
        }

        // push_back move semantics
        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, T &&elem)
        {
            m_delayed_items.queue().push_delay_for(__rtime, std::forward<T>(elem));
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, T &&elem)
        {
            m_delayed_items.queue().push_delay_until(__atime, std::forward<T>(elem));
        }

        template <typename _Rep, typename _Period>
        inline void push_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, std::vector<T> &&elems)
        {
            m_delayed_items.queue().push_delay_for(__rtime, std::forward<std::vector<T>>(elems));
        }

        // avoid time_casting from one clock to another // template <typename _Clock, typename _Duration> //
        inline void push_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, std::vector<T> &&elems)
        {
            m_delayed_items.queue().push_delay_until(__atime, std::forward<std::vector<T>>(elems));
        }

        // emplace_back
        template <typename... _Args>
        inline void emplace_back(_Args &&...__args)
        {
            m_queue_items.queue().emplace_back(std::forward<_Args>(__args)...);
        }

        // emplace_back
        template <typename _Rep, typename _Period, typename... _Args>
        inline void emplace_back_delay_for(const std::chrono::duration<_Rep, _Period> &__rtime, _Args &&...__args)
        {
            m_delayed_items.queue().emplace_delay_for(__rtime, std::forward<T>(__args)...);
        }

        template </* typename _Clock, typename _Duration, */ typename... _Args> // avoid time_casting from one clock to another
        inline void emplace_back_delay_until(const std::chrono::time_point<typename small::time_queue<T>::TimeClock, typename small::time_queue<T>::TimeDuration> &__atime, _Args &&...__args)
        {
            m_delayed_items.queue().emplace_delay_until(__atime, std::forward<T>(__args)...);
        }

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit_force       ()  { m_queue_items.queue().signal_exit_force();     m_delayed_items.queue().signal_exit_force(); }
        inline void signal_exit_when_done   ()  { m_delayed_items.queue().signal_exit_when_done(); /*when the delayed will be finished will signal the queue items to exit when done*/ }
        
        // to be used in processing function
        inline bool is_exit                 ()  { return m_queue_items.queue().is_exit_force() || m_delayed_items.queue().is_exit_force(); }
        // clang-format on

        //
        // wait for threads to finish processing
        //
        inline EnumLock wait()
        {
            signal_exit_when_done();

            m_delayed_items.wait();

            // only now can signal exit when done for queue items (when no more delayed items can be pushed)
            m_queue_items.wait();

            return EnumLock::kExit;
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

            auto status = m_delayed_items.wait_until(__atime);
            if (status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            // only now can signal exit when done for queue items (when no more delayed items can be pushed)
            status = m_queue_items.wait_until(__atime);
            if (status == small::EnumLock::kTimeout) {
                return small::EnumLock::kTimeout;
            }

            return EnumLock::kExit;
        }

    private:
        // some prevention
        worker_thread(const worker_thread &)            = delete;
        worker_thread(worker_thread &&)                 = delete;
        worker_thread &operator=(const worker_thread &) = delete;
        worker_thread &operator=(worker_thread &&__t)   = delete;

    private:
        //
        // inner thread function for active items
        //
        friend small::lock_queue_thread<T, small::worker_thread<T>>;
        friend small::time_queue_thread<T, small::worker_thread<T>>;

        inline config_worker_thread &config()
        {
            return m_config;
        }

        // callback for queue_items
        inline void process_items(std::vector<T> &&items)
        {
            m_processing_function(std::forward<std::vector<T>>(items)); // bind the std::placeholders::_1
        }

    private:
        //
        // members
        //
        config_worker_thread                                 m_config;                // config
        small::lock_queue_thread<T, small::worker_thread<T>> m_queue_items{*this};    // queue of items
        small::time_queue_thread<T, small::worker_thread<T>> m_delayed_items{*this};  // queue of delayed items
        std::function<void(const std::vector<T> &)>          m_processing_function{}; // processing Function
    };
} // namespace small
