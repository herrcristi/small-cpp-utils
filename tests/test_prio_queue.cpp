#include <gtest/gtest.h>

#include <latch>
#include <thread>

#include "../include/prio_queue.h"
#include "../include/util.h"

namespace {
    class PrioQueueTest : public testing::Test
    {
    protected:
        PrioQueueTest() = default;

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
    TEST_F(PrioQueueTest, Lock)
    {
        small::prio_queue<int> q;

        std::latch sync_thread{1};
        std::latch sync_main{1};

        // create thread
        auto thread = std::jthread([](small::prio_queue<int>& _q, std::latch& sync_thread, std::latch& sync_main) {
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
    TEST_F(PrioQueueTest, Queue_Operations)
    {
        small::prio_queue<int, small::EnumPriorities> q{
            {.priorities{{
                {small::EnumPriorities::kHigh, 3},
                {small::EnumPriorities::kNormal, 3},
                {small::EnumPriorities::kLow, 3},
            }}}};
        ASSERT_EQ(q.size(), 0);

        // push
        auto r_push = q.push_back(small::EnumPriorities::kNormal, 5);
        ASSERT_EQ(r_push, 1);

        r_push = q.push_back(small::EnumPriorities::kHighest, 5); // is ignored, priority not setup
        ASSERT_EQ(r_push, 0);

        r_push = q.push_back({small::EnumPriorities::kNormal, 6}); // as a pair
        ASSERT_EQ(r_push, 1);
        ASSERT_EQ(q.size(), 2);

        // wait to be empty
        auto ret_wait = q.wait_for(std::chrono::milliseconds(100));
        ASSERT_EQ(ret_wait, small::EnumLock::kTimeout);

        // pop
        int  value{};
        auto ret = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        value = {};
        ret   = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 6);

        // check size
        ASSERT_EQ(q.size(), 0);

        ret_wait = q.wait();
        ASSERT_EQ(ret_wait, small::EnumLock::kExit);

        // other q
        small::prio_queue<int, int /*priorities*/> q1{
            {.priorities{{{1 /*prio*/, 3}}}}};
        ASSERT_EQ(q1.size(), 0);

        r_push = q1.push_back(1 /*prio*/, 5);
        ASSERT_EQ(r_push, 1);
        r_push = q1.push_back(2 /*prio*/, 5); // ignored
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q1.size(), 1);

        q1.clear();
        ASSERT_EQ(q1.size(), 0);
    }

    //
    // queue ignored priorities
    //
    TEST_F(PrioQueueTest, Queue_Operations_Ignore_Priorities)
    {
        small::prio_queue<int, small::EnumIgnorePriorities> q;
        ASSERT_EQ(q.size(), 0);

        // push
        auto r_push = q.push_back(small::EnumIgnorePriorities::kNoPriority, 5);
        ASSERT_EQ(r_push, 1);

        r_push = q.push_back(small::EnumIgnorePriorities::kNoPriority, 6);
        ASSERT_EQ(r_push, 1);

        r_push = q.push_back({small::EnumIgnorePriorities::kNoPriority, 7}); // as a pair
        ASSERT_EQ(r_push, 1);
        ASSERT_EQ(q.size(), 3);

        // pop
        int  value{};
        auto ret = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        value = {};
        ret   = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 6);

        value = {};
        ret   = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 7);

        // check size
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(PrioQueueTest, Queue_Operations_Vec)
    {
        small::prio_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        auto r_push = q.push_back(small::EnumPriorities::kNormal, {1, 2, 3, 4});
        ASSERT_EQ(r_push, 4);

        std::vector<int> v{5, 6, 7, 8};
        r_push = q.push_back(small::EnumPriorities::kHigh, v);
        ASSERT_EQ(r_push, 4);

        r_push = q.push_back(small::EnumPriorities::kLow, {9, 10, 11, 12});
        ASSERT_EQ(r_push, 4);
        ASSERT_EQ(q.size(), 12);

        // pop
        std::vector<int> values;
        auto             ret = q.wait_pop_front(values, 12);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        std::vector<int> expected_order = {5, 6, 7, 1, 8, 2, 3, 9, 4, 10, 11, 12};
        ASSERT_EQ(values.size(), expected_order.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            ASSERT_EQ(values[i], expected_order[i]);
        }

        // check size
        ASSERT_EQ(q.size(), 0);

        q.clear();
    }

    TEST_F(PrioQueueTest, Queue_Operations_Clear)
    {
        small::prio_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(small::EnumPriorities::kNormal, 1);
        ASSERT_EQ(q.size(), 1);

        // clear
        q.clear();

        // check size
        ASSERT_EQ(q.size(), 0);

        // push again
        q.push_back(small::EnumPriorities::kNormal, 1);
        ASSERT_EQ(q.size(), 1);

        // clear
        q.clear(small::EnumPriorities::kHigh); // no op
        ASSERT_EQ(q.size(), 1);

        q.clear(small::EnumPriorities::kNormal);
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(PrioQueueTest, Queue_Operations_Timeout)
    {
        small::prio_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::timeNow();
        int  value{};
        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(300), &value);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        q.emplace_back(small::EnumPriorities::kNormal, 5);
        ASSERT_EQ(q.size(), 1);

        // pop
        value = {};
        ret   = q.wait_pop_front_for(std::chrono::milliseconds(300), &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        ASSERT_EQ(q.size(), 0);

        // pop again
        value     = {};
        timeStart = small::timeNow();
        ret       = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), &value);
        elapsed   = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(PrioQueueTest, Queue_Operations_Timeout_Vec)
    {
        small::prio_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto             timeStart = small::timeNow();
        std::vector<int> values;
        auto             ret = q.wait_pop_front_for(std::chrono::milliseconds(300), values, 10);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        q.emplace_back(small::EnumPriorities::kNormal, 5);
        q.emplace_back(small::EnumPriorities::kNormal, 15);
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
        ret       = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), values, 10);
        elapsed   = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(PrioQueueTest, Queue_Operations_Thread)
    {
        small::prio_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push inside thread
        auto timeStart = small::timeNow();
        auto thread    = std::jthread([](small::prio_queue<int>& _q) {
            small::sleep(300);

            int value{5};
            _q.push_back(small::EnumPriorities::kNormal, value);
        },
                                   std::ref(q));

        // wait and pop
        int  value   = {};
        auto ret     = q.wait_pop_front(&value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
    }

    TEST_F(PrioQueueTest, Queue_Operations_Signal_Exit_Force)
    {
        small::prio_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(small::EnumPriorities::kNormal, 5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        int  value = {};
        auto ret   = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::timeNow();
        auto thread    = std::jthread([](small::prio_queue<int>& _q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_force();
        },
                                   std::ref(q));

        // check
        value        = {};
        ret          = q.wait_pop_front(&value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_back(small::EnumPriorities::kNormal, 5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(PrioQueueTest, Queue_Operations_Signal_Exit_When_Done)
    {
        small::prio_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(small::EnumPriorities::kNormal, 5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        int  value = {};
        auto ret   = q.wait_pop_front(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::timeNow();
        auto thread    = std::jthread([](small::prio_queue<int>& _q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_when_done();
        },
                                   std::ref(q));

        // check that exit happened because there is nothing in queue
        value        = {};
        ret          = q.wait_pop_front(&value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_back(small::EnumPriorities::kNormal, 5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

} // namespace