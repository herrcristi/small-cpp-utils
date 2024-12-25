#include <gtest/gtest.h>

#include <latch>
#include <thread>

#include "../include/group_queue.h"
#include "../include/util.h"

namespace {

    // job type
    enum class JobType
    {
        kJob1,
        kJob2,
        kJob3,
    };

    class JobsQueueTest : public testing::Test
    {
    protected:
        JobsQueueTest() = default;

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
    TEST_F(JobsQueueTest, Lock)
    {
        small::group_queue<JobType, int> q;

        std::latch sync_thread{1};
        std::latch sync_main{1};

        // create thread
        auto thread = std::jthread([](small::group_queue<JobType, int> &_q, std::latch &sync_thread, std::latch &sync_main) {
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
    TEST_F(JobsQueueTest, Queue_Operations)
    {
        small::group_queue<JobType, int, JobType /*as group*/, small::EnumPriorities> q(
            {.priorities{{
                {small::EnumPriorities::kHigh, 3},
                {small::EnumPriorities::kNormal, 3},
                {small::EnumPriorities::kLow, 3},
            }}});
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);
        ASSERT_EQ(q.size(), 0);

        // push
        auto r_push = q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, 5);
        ASSERT_EQ(r_push, 1);

        r_push = q.push_back(small::EnumPriorities::kHigh, {JobType::kJob2, 6}); // as a pair
        ASSERT_EQ(r_push, 1);

        r_push = q.push_back(small::EnumPriorities::kHighest, {JobType::kJob2, 7}); // is ignored, priority not setup
        ASSERT_EQ(r_push, 0);

        r_push = q.push_back(small::EnumPriorities::kNormal, JobType::kJob3, 8);
        ASSERT_EQ(r_push, 1);

        ASSERT_EQ(q.size(), 3);

        // pop
        std::pair<JobType, int> value{};

        auto ret = q.wait_pop_front(JobType::kJob1 /*as group*/, &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value.first, JobType::kJob2); // higher priority
        ASSERT_EQ(value.second, 6);

        value = {};
        ret   = q.wait_pop_front(JobType::kJob1 /*as group*/, &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value.first, JobType::kJob1);
        ASSERT_EQ(value.second, 5); // low priority

        value = {};
        ret   = q.wait_pop_front(JobType::kJob3 /*as group*/, &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value.first, JobType::kJob3);
        ASSERT_EQ(value.second, 8);

        // check size
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(JobsQueueTest, Queue_Operations_Vec)
    {
        small::group_queue<JobType, int> q;
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);

        ASSERT_EQ(q.size(), 0);

        // push
        auto r_push = q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, {1, 2, 3, 4});
        ASSERT_EQ(r_push, 4);

        std::vector<int> v{5, 6, 7, 8};
        r_push = q.push_back(small::EnumPriorities::kHigh, JobType::kJob2, v);
        ASSERT_EQ(r_push, 4);

        r_push = q.push_back(small::EnumPriorities::kLow, JobType::kJob1, {9, 10, 11, 12});
        ASSERT_EQ(r_push, 4);
        ASSERT_EQ(q.size(), 12);

        // pop
        std::vector<std::pair<JobType, int>> values;

        auto ret = q.wait_pop_front(JobType::kJob1 /*as group*/, values, 12);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        std::vector<int> expected_order = {5, 6, 7, 1, 8, 2, 3, 9, 4, 10, 11, 12};
        ASSERT_EQ(values.size(), expected_order.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            bool is_job2 = expected_order[i] == 5 || expected_order[i] == 6 || expected_order[i] == 7 || expected_order[i] == 8;
            ASSERT_EQ(values[i].first, is_job2 ? JobType::kJob2 : JobType::kJob1);
            ASSERT_EQ(values[i].second, expected_order[i]);
        }

        // check size
        ASSERT_EQ(q.size(), 0);

        q.clear();
    }

    TEST_F(JobsQueueTest, Queue_Operations_Clear)
    {
        small::group_queue<JobType, int> q;
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);

        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, 1);
        ASSERT_EQ(q.size(), 1);

        // clear
        q.clear();

        // check size
        ASSERT_EQ(q.size(), 0);

        // push again
        q.push_back(small::EnumPriorities::kNormal, JobType::kJob2, 1);
        ASSERT_EQ(q.size(), 1);

        // clear
        q.clear(JobType::kJob3 /*as group*/); // no op
        ASSERT_EQ(q.size(), 1);

        q.clear(JobType::kJob1 /*as group*/);
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(JobsQueueTest, Queue_Operations_Timeout)
    {
        small::group_queue<JobType, int> q;
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);

        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::timeNow();

        std::pair<JobType, int> value{};

        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(300), JobType::kJob1 /*as group*/, &value);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        q.emplace_back(small::EnumPriorities::kNormal, JobType::kJob1, 5);
        ASSERT_EQ(q.size(), 1);

        // pop
        value = {};
        ret   = q.wait_pop_front_for(std::chrono::milliseconds(300), JobType::kJob1 /*as group*/, &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value.second, 5);

        ASSERT_EQ(q.size(), 0);

        // pop again
        value     = {};
        timeStart = small::timeNow();
        ret       = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), JobType::kJob1 /*as group*/, &value);
        elapsed   = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(JobsQueueTest, Queue_Operations_Timeout_Vec)
    {
        small::group_queue<JobType, int> q;
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);

        ASSERT_EQ(q.size(), 0);

        // wait with timeout (since no elements)
        auto timeStart = small::timeNow();

        std::vector<std::pair<JobType, int>> values;

        auto ret = q.wait_pop_front_for(std::chrono::milliseconds(300), JobType::kJob1 /*as group*/, values, 10);
        ASSERT_EQ(ret, small::EnumLock::kTimeout);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push
        q.emplace_back(small::EnumPriorities::kNormal, JobType::kJob1, 5);
        q.emplace_back(small::EnumPriorities::kNormal, JobType::kJob1, 15);
        ASSERT_EQ(q.size(), 2);

        // pop
        ret = q.wait_pop_front_for(std::chrono::milliseconds(300), JobType::kJob1 /*as group*/, values, 10);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(values.size(), 2);
        ASSERT_EQ(values[0].second, 5);
        ASSERT_EQ(values[1].second, 15);

        ASSERT_EQ(q.size(), 0);

        // pop again
        timeStart = small::timeNow();
        ret       = q.wait_pop_front_until(timeStart + std::chrono::milliseconds(300), JobType::kJob1 /*as group*/, values, 10);
        elapsed   = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, small::EnumLock::kTimeout);
    }

    TEST_F(JobsQueueTest, Queue_Operations_Thread)
    {
        small::group_queue<JobType, int> q;
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);

        ASSERT_EQ(q.size(), 0);

        // push inside thread
        auto timeStart = small::timeNow();

        auto thread = std::jthread([](small::group_queue<JobType, int> &_q) {
            small::sleep(300);

            int value{5};
            _q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, value);
        },
                                   std::ref(q));

        // wait and pop
        std::pair<JobType, int> value = {};

        auto ret     = q.wait_pop_front(JobType::kJob1 /*as group*/, &value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value.second, 5);
        ASSERT_EQ(q.size(), 0);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
    }

    TEST_F(JobsQueueTest, Queue_Operations_Signal_Exit_Force)
    {
        small::group_queue<JobType, int> q;
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);

        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, 5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        std::pair<JobType, int> value = {};

        auto ret = q.wait_pop_front(JobType::kJob1 /*as group*/, &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value.second, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::timeNow();
        auto thread    = std::jthread([](small::group_queue<JobType, int> &_q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_force();
        },
                                   std::ref(q));

        // check
        value        = {};
        ret          = q.wait_pop_front(JobType::kJob1 /*as group*/, &value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, 5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

    TEST_F(JobsQueueTest, Queue_Operations_Signal_Exit_When_Done)
    {
        small::group_queue<JobType, int> q;
        q.add_type_group(JobType::kJob1, JobType::kJob1 /*as group*/);
        q.add_type_group(JobType::kJob2, JobType::kJob1 /*as group*/); // same group
        q.add_type_group(JobType::kJob3, JobType::kJob3 /*as group*/);

        ASSERT_EQ(q.size(), 0);

        // push
        q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, 5);
        ASSERT_EQ(q.size(), 1);

        // wait and pop
        std::pair<JobType, int> value = {};

        auto ret = q.wait_pop_front(JobType::kJob1 /*as group*/, &value);
        ASSERT_EQ(ret, small::EnumLock::kElement);
        ASSERT_EQ(value.second, 5);
        ASSERT_EQ(q.size(), 0);

        // create thread
        auto timeStart = small::timeNow();
        auto thread    = std::jthread([](small::group_queue<JobType, int> &_q) {
            // signal after some time
            small::sleep(300);
            _q.signal_exit_when_done();
        },
                                   std::ref(q));

        // check that exit happened because there is nothing in queue
        value        = {};
        ret          = q.wait_pop_front(JobType::kJob1 /*as group*/, &value);
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_EQ(ret, small::EnumLock::kExit);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push is no longer accepted
        auto r_push = q.push_back(small::EnumPriorities::kNormal, JobType::kJob1, 5);
        ASSERT_EQ(r_push, 0);
        ASSERT_EQ(q.size(), 0);
    }

} // namespace