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
        auto thread = std::jthread([](small::lock_queue<int> &_q, std::latch &sync_thread, std::latch &sync_main) {
            std::unique_lock lock(_q);
            sync_thread.count_down(); // signal that thread is started (and also locked is acquired)
            sync_main.wait();         // wait that the main finished executing test to proceed further
            _q.lock();                // locked again on same thread
            small::sleep(300);        // sleep inside lock
            _q.unlock();
        },
                                   std::ref(q), std::ref(sync_thread), std::ref(sync_main));

        // wait for the thread to start
        sync_thread.wait();

        // try to lock and it wont succeed
        auto locked = q.try_lock();
        ASSERT_FALSE(locked);

        // signal thread to proceed further
        auto timeStart = small::timeNow();
        sync_main.count_down();

        // wait for the thread to stop
        while (!locked) {
            small::sleep(1);
            locked = q.try_lock();
        }
        ASSERT_TRUE(locked);

        // unlock
        q.unlock();

        auto elapsed = small::timeDiffMs(timeStart);
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
        q.push_back(5);
        ASSERT_EQ(q.size(), 1);

        // pop
        int value{};
        auto ret = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        // check size
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(LockQueueTest, Queue_Operations_Vec)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(5);
        ASSERT_EQ(q.size(), 1);

        // pop
        std::vector<int> values;
        auto ret = q.wait_pop_front(values, 10);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(values.size(), 1);
        ASSERT_EQ(values[0], 5);

        // check size
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(LockQueueTest, Queue_Operations_Timeout)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::timeNow();
        int value{};
        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(300), &value);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        q.emplace_back(5);
        ASSERT_EQ(q.size(), 1);

        // pop
        value = {};
        ret = q.wait_pop_front_for(std::chrono::milliseconds(300), &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        ASSERT_EQ(q.size(), 0);

        // pop again
        value = {};
        timeStart = small::timeNow();
        ret = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), &value);
        elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(LockQueueTest, Queue_Operations_Timeout_Vec)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::timeNow();
        std::vector<int> values;
        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(300), values, 10);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::timeDiffMs(timeStart);
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
        timeStart = small::timeNow();
        ret = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), values, 10);
        elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(LockQueueTest, Queue_Operations_Thread)
    {
        small::lock_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push inside thread
        auto timeStart = small::timeNow();
        auto thread = std::jthread([](small::lock_queue<int> &_q) {
            small::sleep(300);

            int value{5};
            _q.push_back(value);
        },
                                   std::ref(q));

        // wait and pop
        int value = {};
        auto ret = q.wait_pop_front(&value);
        auto elapsed = small::timeDiffMs(timeStart);

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
        int value = {};
        auto ret = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::timeNow();
        auto thread = std::jthread([](small::lock_queue<int> &_q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_force();
        },
                                   std::ref(q));

        // check
        value = {};
        ret = q.wait_pop_front(&value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        q.push_back(5);
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
        int value = {};
        auto ret = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::timeNow();
        auto thread = std::jthread([](small::lock_queue<int> &_q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_when_done();
        },
                                   std::ref(q));

        // check that exit happened because there is nothing in queue
        value = {};
        ret = q.wait_pop_front(&value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        q.push_back(5);
        ASSERT_EQ(q.size(), 0);
    }

} // namespace