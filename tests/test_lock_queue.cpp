#include <gtest/gtest.h>

#include <latch>

#include "../include/lock_queue.h"
#include "../include/util.h"

namespace {
    class LockQueueTest : public testing::Test
    {
    protected:
        LockQueueTest() = default;

        void SetUp() override
        {
            // setup before test
        }
        void TearDown() override
        {
            // cleanup after test
        }
    };

    //
    // lock
    //
    TEST_F(LockQueueTest, Lock)
    {
        small::lock_queue<int> q;

        std::latch sync_thread{1};
        std::latch sync_main{1};

        // create thread
        auto thread = std::jthread([](small::lock_queue<int>& _q, std::latch& _sync_thread, const std::latch& _sync_main) {
            std::unique_lock lock(_q);
            _sync_thread.count_down(); // signal that thread is started (and also locked is acquired)
            _sync_main.wait();         // wait that the main finished executing test to proceed further
            _q.lock();                 // locked again on same thread
            small::sleep(300);         // sleep inside lock
            _q.unlock();
        },
                                   std::ref(q), std::ref(sync_thread), std::ref(sync_main));

        // wait for the thread to start
        sync_thread.wait();

        // try to lock and it wont succeed
        auto locked = q.try_lock();
        ASSERT_FALSE(locked);

        // signal thread to proceed further
        auto timeStart = small::time_now();
        sync_main.count_down();

        // wait for the thread to stop
        while (!locked) {
            small::sleep(1);
            locked = q.try_lock();
        }
        ASSERT_TRUE(locked);

        // unlock
        q.unlock();

        auto elapsed = small::time_diff_ms(timeStart);
        ASSERT_GE(elapsed, 300 - 1);
    }

    //
    // queue
    //
    TEST_F(LockQueueTest, Queue_Operations)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        auto r_push = q.push_back(5);
        ASSERT_EQ(r_push, 1);
        ASSERT_EQ(q.size(), 1);

        // wait to be empty
        auto ret_wait = q.wait_for(std::chrono::milliseconds(100));
        ASSERT_EQ(ret_wait, small::EnumLock::kTimeout);

        // pop
        int  value{};
        auto ret = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        // check size
        ASSERT_EQ(q.size(), 0);

        ret_wait = q.wait();
        ASSERT_EQ(ret_wait, small::EnumLock::kExit);
    }

    TEST_F(LockQueueTest, Queue_Operations_Vec)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        auto r_push = q.push_back({5, 6});
        ASSERT_EQ(r_push, 2);

        std::vector<int> v{7, 8};
        r_push = q.push_back(v);
        ASSERT_EQ(r_push, 2);
        ASSERT_EQ(q.size(), 4);

        // pop
        std::vector<int> values;

        auto ret = q.wait_pop_front(values, 10);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(values.size(), 4);
        ASSERT_EQ(values[0], 5);
        ASSERT_EQ(values[1], 6);
        ASSERT_EQ(values[2], 7);
        ASSERT_EQ(values[3], 8);

        // check size
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(LockQueueTest, Queue_Operations_Timeout)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::time_now();
        int  value{};
        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(300), &value);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::time_diff_ms(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        q.emplace_back(5);
        ASSERT_EQ(q.size(), 1);

        // pop
        value = {};
        ret   = q.wait_pop_front_for(std::chrono::milliseconds(300), &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        ASSERT_EQ(q.size(), 0);

        // pop again
        value     = {};
        timeStart = small::time_now();
        ret       = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), &value);
        elapsed   = small::time_diff_ms(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(LockQueueTest, Queue_Operations_Timeout_Vec)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::time_now();

        std::vector<int> values;

        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(300), values, 10);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::time_diff_ms(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        q.emplace_back(5);
        q.emplace_back(15);
        ASSERT_EQ(q.size(), 2);

        // pop
        ret = q.wait_pop_front_for(std::chrono::milliseconds(300), values, 10);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(values.size(), 2);
        ASSERT_EQ(values[0], 5);
        ASSERT_EQ(values[1], 15);

        ASSERT_EQ(q.size(), 0);

        // pop again
        timeStart = small::time_now();
        ret       = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), values, 10);
        elapsed   = small::time_diff_ms(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(LockQueueTest, Queue_Operations_Thread)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push inside thread
        auto timeStart = small::time_now();
        auto thread    = std::jthread([](small::lock_queue<int>& _q) {
            small::sleep(300);

            int value{5};
            _q.push_back(value);
        },
                                   std::ref(q));

        // wait and pop
        int  value   = {};
        auto ret     = q.wait_pop_front(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
    }

    TEST_F(LockQueueTest, Queue_Operations_Signal_Exit_Force)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        int  value = {};
        auto ret   = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::time_now();
        auto thread    = std::jthread([](small::lock_queue<int>& _q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_force();
        },
                                   std::ref(q));

        // check
        value        = {};
        ret          = q.wait_pop_front(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_back(5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(LockQueueTest, Queue_Operations_Signal_Exit_When_Done)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        int  value = {};
        auto ret   = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::time_now();
        auto thread    = std::jthread([](small::lock_queue<int>& _q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_when_done();
        },
                                   std::ref(q));

        // check that exit happened because there is nothing in queue
        value        = {};
        ret          = q.wait_pop_front(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_back(5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

} // namespace