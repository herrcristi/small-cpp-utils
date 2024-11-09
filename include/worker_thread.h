#pragma once

#include <functional>
#include <thread>
#include <vector>

#include "event_queue.h"

// using qc = std::pair<int, std::string>;
// ...
// // with a lambda for processing working function
// small::worker_thread<qc> workers( 0, []( auto& w/*this*/, auto& item, auto b/*extra param*/ ) -> void
// {
//     ...
//     {
//         std::unique_lock mlock( w ); // use worker_thread to lock
//         ...
//         //std::cout << "thread " << std::this_thread::get_id() << "processing " << item.first << " " << item.second << " b=" << b << "\n";
//     }
//     ...
// }, 5 /*extra param*/ );
//
// workers.start_threads(2); // manual start threads
// ...
// // or like this
// small::worker_thread<qc> workers2( 1, WorkerThreadFunction() );
// ...
// // where WorkerThreadFunction can be
// struct WorkerThreadFunction
// {
//     using qc = std::pair<int, std::string>;
//     void operator()( small::worker_thread<qc>& w /*worker_thread*/, qc& item )
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
// ...
// // no more work, wait to be finished
// auto ret = workers.wait_for( std::chrono::seconds(30) ); // auto ret = workers.wait();
// if  ( ret ==  small::EnumEventQueue:: kQueue_Timeout ) {
//     // when finishing after signal_exit the work is aborted
//     workers.signal_exit();
// }
// //

namespace small {
    //
    // small class for worker threads
    //
    template <class T>
    class worker_thread
    {
    public:
        //
        // worker_thread
        //
        template <typename _Callable, typename... Args>
        worker_thread(const int threads_count /*= 1*/, _Callable function, Args... extra_parameters)
            : m_processing_function(std::bind(std::forward<_Callable>(function), std::ref(*this), std::placeholders::_1 /*item*/, std::forward<Args>(extra_parameters)...))
        {
            // auto start threads if count > 0 otherwise threads should be manually started
            if (threads_count) {
                start_threads(threads_count);
            }
        }

        ~worker_thread()
        {
            signal_exit();
            stop_threads();
        }

        // clang-format off
        // size
        inline size_t   size        () const { return m_queue_items.size();  }
        // empty
        inline bool     empty       () const { return size() == 0; }
        // clear
        inline void     clear       () { m_queue_items.clear(); }
        // clang-format on

        // clang-format off
        // use it as locker (std::unique_lock<small:worker_thread<T>> m...)
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

            // check again
            if (m_threads_flag_created.load() == true) {
                return;
            }

            // create threads
            m_threads.resize(threads_count);
            for (auto &th : m_threads) {
                if (!th.joinable()) {
                    th = std::thread(/*[&]() { thread_function(); }*/ &worker_thread::thread_function, this);
                }
            }

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

            m_queue_items.push_back(std::forward<T>(t));
        }

        // push back with move semantics
        inline void push_back(T &&t)
        {
            if (is_exit()) {
                return;
            }

            m_queue_items.push_back(std::forward<T>(t));
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

        // clang-format off
        //
        // signal exit
        //
        inline void signal_exit ()  { m_queue_items.signal_exit(); }
        inline bool is_exit     ()  { return m_queue_items.is_exit(); }
        // clang-format on

    private:
        // some prevention
        worker_thread(const worker_thread &) = delete;
        worker_thread(worker_thread &&) = delete;

        worker_thread &operator=(const worker_thread &) = delete;
        worker_thread &operator=(worker_thread &&__t) = delete;

    private:
        //
        // inner thread function
        //
        inline void thread_function()
        {
            T elem{};
            while (1) {
                // wait
                small::EnumEventQueue ret = m_queue_items.wait_pop_front /*_for*/ (/*std::chrono::minutes( 10 ),*/ &elem);

                if (ret == small::EnumEventQueue::kQueue_Exit) {
                    // force stop
                    break;
                } else if (ret == small::EnumEventQueue::kQueue_Timeout) {
                    // nothing to do
                } else if (ret == small::EnumEventQueue::kQueue_Element) {
                    // process
                    m_processing_function(std::ref(elem)); // bind the std::placeholders::_1
                }
            }
        }

        //
        // stop threads
        //
        inline void stop_threads()
        {
            std::vector<std::thread> threads;
            {
                std::unique_lock mlock(m_queue_items);
                threads = std::move(m_threads);
                m_threads_flag_created.store(false);
            }

            // wait for threads to stop
            for (auto &th : threads) {
                if (th.joinable()) {
                    th.join();
                }
            }
        }

    private:
        //
        // members
        //
        std::vector<std::thread> m_threads;               // threads
        std::atomic<bool> m_threads_flag_created{};       // threads flag
        small::event_queue<T> m_queue_items;              // queue of items
        std::function<void(T &)> m_processing_function{}; // processing Function
    };
} // namespace small
