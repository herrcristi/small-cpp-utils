#pragma once

#include <atomic>
#include <chrono>
#include <thread>

// spin lock - just like a mutex but it uses atomic lockless to do locking
// use with care because order of execution is not guaranteed
//
// small::spinlock lock; // small::critical_section lock;
// ...
// {
//     std::unique_lock<small::spinlock> mlock( lock );
//
//    // do your work
//    ...
// }
//
namespace small {
    // spinlock
    class spinlock
    {
    public:
        //
        // spinlock
        //
        spinlock(const int &spin_count = 4000, const int &wait_in_micro_seconds = 1000 /*1 millisecond*/)
            : m_spin_count(spin_count),
              m_wait_in_microseconds(wait_in_micro_seconds)
        {
        }

        //
        // lock functions
        //
        inline void lock()
        {
            // Atomically changes the state of a std::atomic_flag to set (true) and returns the value it held before.
            for (int count = 0; m_lock.test_and_set(std::memory_order_acquire); ++count) {
                // lock is true but is not set in this lock
                if (count >= m_spin_count) {
                    // wait
                    std::this_thread::sleep_for(std::chrono::microseconds(m_wait_in_microseconds)); // std::this_thread::yield();
                }
            }
        }

        //
        // unlock
        //
        inline void unlock()
        {
            m_lock.clear(std::memory_order_release);
        }

        //
        // try lock
        //
        inline bool try_lock()
        {
            return m_lock.test_and_set(std::memory_order_acquire) ? /*before was true so no lock*/ false : /*lock*/ true;
        }

    private:
        // members
        std::atomic_flag m_lock{};
        int m_spin_count{};
        int m_wait_in_microseconds{};
    };
} // namespace small