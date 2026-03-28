#include "test_common.h"

#include "../include/time_queue.h"
#include "../include/util.h"

namespace {
    class TimeQueueTest : public testing::Test
    {
    protected:
        TimeQueueTest() = default;

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
    TEST_F(TimeQueueTest, Lock)
    {
        small::time_queue<int> q;

        std::latch sync_thread{1};
        std::latch sync_main{1};

        // create thread
        auto thread = std::jthread([](small::time_queue<int>& _q, std::latch& _sync_thread, const std::latch& _sync_main) {
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
    TEST_F(TimeQueueTest, Queue_Operations_Now)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        auto timeStart = small::time_now();

        // push
        q.push_delay_for(std::chrono::milliseconds(-1), 5);
        ASSERT_EQ(q.size(), 1);

        // wait to be empty
        auto ret_wait = q.wait_for(std::chrono::milliseconds(100));
        ASSERT_EQ(ret_wait, small::EnumLock::kTimeout);

        // pop
        int  value{};
        auto ret     = q.wait_pop(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        // check size
        ASSERT_EQ(q.size(), 0);
        ASSERT_LE(elapsed, 300);

        ret_wait = q.wait();
        ASSERT_EQ(ret_wait, small::EnumLock::kExit);
    }

    TEST_F(TimeQueueTest, Queue_Operations_Vec)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        auto timeStart = small::time_now();

        // push
        auto r_push = q.push_delay_for(std::chrono::milliseconds(0), 5);
        ASSERT_EQ(r_push, 1);
        r_push = q.push_delay_for(std::chrono::milliseconds(0), 6);
        ASSERT_EQ(r_push, 1);
        r_push = q.push_delay_for(std::chrono::milliseconds(0), {7, 8});
        ASSERT_EQ(r_push, 2);

        std::vector<int> v{9};
        q.push_delay_for(std::chrono::milliseconds(0), v);
        ASSERT_EQ(q.size(), 5);

        // pop
        std::vector<int> values;
        auto             ret = q.wait_pop(values, 10);

        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(values.size(), 5);
        ASSERT_EQ(values[0], 5);
        ASSERT_EQ(values[1], 6);
        ASSERT_EQ(values[2], 7);
        ASSERT_EQ(values[3], 8);
        ASSERT_EQ(values[4], 9);

        // check size
        ASSERT_EQ(q.size(), 0);
        ASSERT_LE(elapsed, 300);
    }

    TEST_F(TimeQueueTest, Queue_Operations_Delay)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        auto timeStart = small::time_now();

        // push
        q.push_delay_for(std::chrono::milliseconds(300), 5);
        ASSERT_EQ(q.size(), 1);

        // pop
        int  value{};
        auto ret     = q.wait_pop(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        // check size
        ASSERT_EQ(q.size(), 0);
        ASSERT_GE(elapsed, 300 - 1);
    }

    TEST_F(TimeQueueTest, Queue_Operations_Timeout)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::time_now();
        int  value{};
        auto ret = q.wait_pop_for(std::chrono::milliseconds(300), &value);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::time_diff_ms(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        timeStart = small::time_now();

        q.emplace_delay_for(std::chrono::milliseconds(0), 5);
        ASSERT_EQ(q.size(), 1);

        // pop
        value   = {};
        ret     = q.wait_pop_for(std::chrono::milliseconds(300), &value);
        elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);

        ASSERT_EQ(q.size(), 0);
        ASSERT_LE(elapsed, 100);

        // pop again
        timeStart = small::time_now();

        value     = {};
        timeStart = small::time_now();
        ret       = q.wait_pop_until(timeStart + std::chrono::milliseconds(300), &value);
        elapsed   = small::time_diff_ms(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(TimeQueueTest, Queue_Operations_Timeout_Vec)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        auto timeStart = small::time_now();

        // wait with timeout (since no elements)
        std::vector<int> values;
        auto             ret = q.wait_pop_for(std::chrono::milliseconds(300), values, 10);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::time_diff_ms(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        timeStart = small::time_now();
        q.emplace_delay_for(std::chrono::milliseconds(0), 5);
        q.emplace_delay_for(std::chrono::milliseconds(300), 15);
        ASSERT_EQ(q.size(), 2);

        // pop
        ret     = q.wait_pop_for(std::chrono::milliseconds(600), values, 10);
        elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(values.size(), 1);
        ASSERT_EQ(values[0], 5);
        ASSERT_LE(elapsed, 100);

        ret     = q.wait_pop_for(std::chrono::milliseconds(600), values, 10);
        elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(values.size(), 1);
        ASSERT_EQ(values[0], 15);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        ASSERT_EQ(q.size(), 0);

        // pop again
        timeStart = small::time_now();
        ret       = q.wait_pop_until(timeStart + std::chrono::milliseconds(300), values, 10);
        elapsed   = small::time_diff_ms(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(TimeQueueTest, Queue_Operations_Thread)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push inside thread
        auto timeStart = small::time_now();
        auto thread    = std::jthread([](small::time_queue<int>& _q) {
            int value{5};
            _q.push_delay_for(std::chrono::milliseconds(300), value);
        },
                                      std::ref(q));

        // wait and pop
        int  value   = {};
        auto ret     = q.wait_pop(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
    }

    TEST_F(TimeQueueTest, Queue_Operations_Signal_Exit_Force)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_delay_for(std::chrono::milliseconds(0), 5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        int  value = {};
        auto ret   = q.wait_pop(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::time_now();
        auto thread    = std::jthread([](small::time_queue<int>& _q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_force();
        },
                                      std::ref(q));

        // check
        value        = {};
        ret          = q.wait_pop(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_delay_for(std::chrono::milliseconds(0), 5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(TimeQueueTest, Queue_Operations_Signal_Exit_When_Done)
    {
        small::time_queue<int> q;
        ASSERT_EQ(q.size(), 0);

        // push
        q.push_delay_for(std::chrono::milliseconds(0), 5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        int  value = {};
        auto ret   = q.wait_pop(&value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::time_now();
        auto thread    = std::jthread([](small::time_queue<int>& _q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_when_done();
        },
                                      std::ref(q));

        // check that exit happened because there is nothing in queue
        value        = {};
        ret          = q.wait_pop(&value);
        auto elapsed = small::time_diff_ms(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_delay_for(std::chrono::milliseconds(0), 5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

    //
    // capacity checking tests
    //
    TEST_F(TimeQueueTest, Capacity_Check_Single_Element)
    {
        small::time_queue_config config;
        config.max_size = 3;
        small::time_queue<int> q(config);

        // Push 3 elements successfully
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), 1), 1);
        ASSERT_EQ(q.size(), 1);

        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), 2), 1);
        ASSERT_EQ(q.size(), 2);

        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), 3), 1);
        ASSERT_EQ(q.size(), 3);

        // Push should fail due to capacity limit
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), 4), 0);
        ASSERT_EQ(q.size(), 3);
    }

    TEST_F(TimeQueueTest, Capacity_Check_Move_Semantics)
    {
        small::time_queue_config config;
        config.max_size = 2;
        small::time_queue<int> q(config);

        // Push 2 elements with move semantics
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), std::make_unique<int>(1) ? 1 : 0), 1);
        ASSERT_EQ(q.size(), 1);

        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), std::make_unique<int>(2) ? 1 : 0), 1);
        ASSERT_EQ(q.size(), 2);

        // Push should fail due to capacity limit
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), std::make_unique<int>(3) ? 1 : 0), 0);
        ASSERT_EQ(q.size(), 2);
    }

    TEST_F(TimeQueueTest, Capacity_Check_Vector_Elements)
    {
        small::time_queue_config config;
        config.max_size = 5;
        small::time_queue<int> q(config);

        // Push vector of 3 elements successfully
        std::vector<int> vec1{1, 2, 3};
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), vec1), 3);
        ASSERT_EQ(q.size(), 3);

        // Push vector of 2 more elements successfully
        std::vector<int> vec2{4, 5};
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), vec2), 2);
        ASSERT_EQ(q.size(), 5);

        // Push vector when at capacity - should return 0
        std::vector<int> vec3{6, 7};
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), vec3), 0);
        ASSERT_EQ(q.size(), 5);
    }

    TEST_F(TimeQueueTest, Capacity_Check_Vector_Partial_Insert)
    {
        small::time_queue_config config;
        config.max_size = 4;
        small::time_queue<int> q(config);

        // Push vector of 2 elements
        std::vector<int> vec1{1, 2};
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), vec1), 2);
        ASSERT_EQ(q.size(), 2);

        // Push vector of 3 elements - should insert only 2 due to capacity
        std::vector<int> vec2{3, 4, 5};
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), vec2), 2);
        ASSERT_EQ(q.size(), 4); // 2 + 2, not 2 + 3
    }

    TEST_F(TimeQueueTest, Capacity_Check_With_Push_Until)
    {
        small::time_queue_config config;
        config.max_size = 3;
        small::time_queue<int> q(config);

        auto timePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(100);

        // Push 3 elements with push_delay_until
        ASSERT_EQ(q.push_delay_until(timePoint, 1), 1);
        ASSERT_EQ(q.push_delay_until(timePoint, 2), 1);
        ASSERT_EQ(q.push_delay_until(timePoint, 3), 1);
        ASSERT_EQ(q.size(), 3);

        // Push should fail due to capacity
        ASSERT_EQ(q.push_delay_until(timePoint, 4), 0);
        ASSERT_EQ(q.size(), 3);
    }

    TEST_F(TimeQueueTest, Capacity_Check_Vector_Push_Until)
    {
        small::time_queue_config config;
        config.max_size = 5;
        small::time_queue<int> q(config);

        auto timePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(100);

        // Push vector of 3 elements
        std::vector<int> vec1{1, 2, 3};
        ASSERT_EQ(q.push_delay_until(timePoint, vec1), 3);
        ASSERT_EQ(q.size(), 3);

        // Push vector of 3 elements - should insert only 2 due to capacity
        std::vector<int> vec2{4, 5, 6};
        ASSERT_EQ(q.push_delay_until(timePoint, vec2), 2);
        ASSERT_EQ(q.size(), 5);
    }

    TEST_F(TimeQueueTest, Capacity_Zero_Rejects_All)
    {
        small::time_queue_config config;
        config.max_size = 0;
        small::time_queue<int> q(config);

        // All pushes should fail when max_size is 0
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), 1), 0);
        ASSERT_EQ(q.size(), 0);

        std::vector<int> vec{1, 2, 3};
        ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), vec), 0);
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(TimeQueueTest, Capacity_Unlimited_Default)
    {
        // Default config has unlimited capacity
        small::time_queue<int> q;

        // Should be able to push many elements
        for (int i = 0; i < 1000; ++i) {
            ASSERT_EQ(q.push_delay_for(std::chrono::milliseconds(0), i), 1);
        }
        ASSERT_EQ(q.size(), 1000);
    }

} // namespace