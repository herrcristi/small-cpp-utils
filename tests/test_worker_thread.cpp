#include <gtest/gtest.h>

#include <latch>

#include "../include/util.h"
#include "../include/worker_thread.h"

namespace {
    class WorkerThreadTest : public testing::Test
    {
    protected:
        WorkerThreadTest() = default;

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
    TEST_F(WorkerThreadTest, Lock)
    {
        small::event_queue<int> q;

        std::latch sync_thread{1};
        std::latch sync_main{1};

        // create thread
        auto thread = std::jthread([](small::event_queue<int> &_q, std::latch &sync_thread, std::latch &sync_main) {
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
    // workers
    //
    TEST_F(WorkerThreadTest, Worker_Operations)
    {
        auto timeStart = small::timeNow();

        // create workers
        small::worker_thread<int> workers(0 /*no threads*/, [](auto &w /*this*/, auto &item, auto b /*extra param b*/) {
            small::sleep(300);
            // process item using the workers lock (not recommended)
        },
                                          5 /*param b*/);

        // push
        workers.push_back(5);
        ASSERT_GE(workers.size(), 1); // because the thread is not started

        workers.start_threads(1); // start thread

        // wait_until
        auto ret = workers.wait_for(std::chrono::milliseconds(0));
        ASSERT_EQ(ret, small::EnumEventQueue::kQueue_Timeout);

        // wait to finish
        ret = workers.wait();
        ASSERT_EQ(ret, small::EnumEventQueue::kQueue_Exit);

        // check size
        ASSERT_EQ(workers.size(), 0);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion

        // push after wait is not allowed
        workers.push_back(1);
        ASSERT_EQ(workers.size(), 0);
    }

    TEST_F(WorkerThreadTest, Worker_Operations_Force_Exit)
    {
        auto timeStart = small::timeNow();

        // create workers
        small::worker_thread<int> workers(1 /*threads*/, [](auto &w /*this*/, auto &item, auto b /*extra param b*/) {
            small::sleep(300);
            if (w.is_exit()) {
                return;
            }
            small::sleep(300);
            // process item using the workers lock (not recommended)
        },
                                          5 /*param b*/);

        // push
        workers.push_back(5);
        workers.push_back(6);
        small::sleep(100); // wait for the thread to start and execute first sleep

        workers.signal_exit_force();
        ASSERT_EQ(workers.size(), 1);

        // push after exit will not work
        workers.push_back(5);
        ASSERT_EQ(workers.size(), 1);

        // wait to finish
        auto ret = workers.wait();
        ASSERT_EQ(ret, small::EnumEventQueue::kQueue_Exit);

        // check size
        ASSERT_EQ(workers.size(), 1);

        // elapsed only 300 and not 600
        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_LT(elapsed, 600 - 1); // due conversion
    }

} // namespace