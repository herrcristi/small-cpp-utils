#include <gtest/gtest.h>

#include <latch>

#include "../include/event.h"
#include "../include/util.h"

namespace {
    class EventTest : public testing::Test
    {
    protected:
        EventTest() = default;

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
    TEST_F(EventTest, Lock)
    {
        small::event event;

        std::latch sync_thread{1};
        std::latch sync_main{1};

        // create thread
        auto thread = std::jthread([](small::event &e, std::latch &sync_thread, std::latch &sync_main) {
            std::unique_lock lock(e);
            sync_thread.count_down(); // signal that thread is started (and also locked is acquired)
            sync_main.wait();         // wait that the main finished executing test to proceed further
            e.lock();                 // locked again on same thread
            small::sleep(300);        // sleep inside lock
            e.unlock();
        },
                                   std::ref(event), std::ref(sync_thread), std::ref(sync_main));

        // wait for the thread to start
        sync_thread.wait();

        // try to lock and it wont succeed
        auto locked = event.try_lock();
        ASSERT_FALSE(locked);

        // signal thread to proceed further
        auto timeStart = small::timeNow();
        sync_main.count_down();

        // wait for the thread to stop
        while (!locked) {
            small::sleep(1);
            locked = event.try_lock();
        }
        ASSERT_TRUE(locked);

        // unlock
        event.unlock();

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1);
    }

    //
    // wait
    //
    TEST_F(EventTest, Wait_Set_NoDelay)
    {
        // create automatic event
        small::event event;
        event.set_event();

        auto timeStart = small::timeNow();
        event.wait();
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_LE(elapsed, 100);
    }

    TEST_F(EventTest, Wait_Set_Delay)
    {
        // create automatic event
        small::event event;

        // create thread
        auto timeStart = small::timeNow();
        auto thread = std::jthread([](small::event &e) {
            small::sleep(300);
            e.set_event();
        },
                                   std::ref(event));

        // wait to be signaled in the thread
        event.wait();
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
    }

    TEST_F(EventTest, Wait_Manual_Multiple_Threads)
    {
        // create manual event
        small::event event(small::EventType::kEvent_Manual);

        // create listening threads
        auto thread0 = std::jthread([](small::event &e) {
            e.wait();
        },
                                    std::ref(event));
        auto thread1 = std::jthread([](small::event &e) {
            e.wait();
        },
                                    std::ref(event));

        auto timeStart = small::timeNow();

        // signal event
        small::sleep(300);
        event.set_event();

        thread0.join();
        thread1.join();

        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
    }

    //
    // wait with condition
    //
    TEST_F(EventTest, Wait_Condition_Signal_By_Thread)
    {
        // create automatic event
        small::event event;

        // create thread
        auto timeStart = small::timeNow();
        auto thread = std::jthread([](small::event &e) {
            small::sleep(300);
            e.set_event(); // signal
        },
                                   std::ref(event));

        int conditionEvaluatedInc = 0;

        // wait to be signaled in the thread
        event.wait([&]() {
            conditionEvaluatedInc++;
            return true; // condition always true
        });
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1);         // due conversion
        ASSERT_EQ(conditionEvaluatedInc, 1); // condition is evaluated after event is signaled
    }

    TEST_F(EventTest, Wait_Condition_Evaluate)
    {
        // create automatic event
        small::event event;

        // create thread
        auto timeStart = small::timeNow();
        auto thread = std::jthread([](small::event &e) {
            e.set_event(); // signal
        },
                                   std::ref(event));

        const int waitTimeCondition = 300;
        int conditionEvaluatedInc = 0;

        // wait to be signaled in the thread
        event.wait([&]() {
            // event is signaled by condition will return after some time
            conditionEvaluatedInc++;
            auto diff = small::timeDiffMs(timeStart);
            return diff >= waitTimeCondition ? true : false;
        });
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, waitTimeCondition - 1); // due conversion
        ASSERT_GE(conditionEvaluatedInc, 3);       // condition is evaluated many times
        ASSERT_LE(conditionEvaluatedInc, waitTimeCondition + 1);
    }

    TEST_F(EventTest, Wait_Condition_Evaluate_Manual_Event)
    {
        // create manual event
        small::event event(small::EventType::kEvent_Manual);
        event.set_event();

        auto timeStart = small::timeNow();

        const int waitTimeCondition = 300;
        int conditionEvaluatedInc = 0;

        // wait to be signaled in the thread
        event.wait([&]() {
            // event is signaled by condition will return after some time
            conditionEvaluatedInc++;
            auto diff = small::timeDiffMs(timeStart);
            return diff >= waitTimeCondition ? true : false;
        });
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, waitTimeCondition - 1); // due conversion
        ASSERT_GE(conditionEvaluatedInc, 3);       // condition is evaluated many times
        ASSERT_LE(conditionEvaluatedInc, waitTimeCondition + 1);
    }

    //
    // wait for
    //
    TEST_F(EventTest, Wait_For_Timeout)
    {
        // create event
        small::event event;

        auto timeStart = small::timeNow();

        auto ret = event.wait_for(std::chrono::milliseconds(300));
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, std::cv_status::timeout);
    }

    TEST_F(EventTest, Wait_For_NoTimeout)
    {
        // create event
        small::event event;
        event.set_event();

        auto timeStart = small::timeNow();

        auto ret = event.wait_for(std::chrono::milliseconds(300));
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_LE(elapsed, 100); // check some time even there is no delay
        ASSERT_EQ(ret, std::cv_status::no_timeout);
    }

    //
    // wait for condition
    //
    TEST_F(EventTest, Wait_For_Condition_Timeout)
    {
        // create manual event
        small::event event(small::EventType::kEvent_Manual);

        auto timeStart = small::timeNow();
        int conditionEvaluatedInc = 0;

        // wait to be signaled in the thread
        auto ret = event.wait_for(std::chrono::milliseconds(300), [&]() {
            return false;
        });
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(conditionEvaluatedInc, 0);
        ASSERT_EQ(ret, std::cv_status::timeout);
    }

    TEST_F(EventTest, Wait_For_Condition_NoTimeout)
    {
        // create manual event
        small::event event;
        event.set_event();

        auto timeStart = small::timeNow();
        int conditionEvaluatedInc = 0;

        // wait to be signaled in the thread
        auto ret = event.wait_for(std::chrono::milliseconds(300), [&]() {
            conditionEvaluatedInc++;
            return true;
        });
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_LE(elapsed, 100); // check some time even there is no delay
        ASSERT_EQ(conditionEvaluatedInc, 1);
        ASSERT_EQ(ret, std::cv_status::no_timeout);
    }

    //
    // wait until
    //
    TEST_F(EventTest, Wait_Until_Timeout)
    {
        // create event
        small::event event;

        auto timeStart = small::timeNow();

        auto ret = event.wait_until(timeStart + std::chrono::milliseconds(300));
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(ret, std::cv_status::timeout);
    }

    TEST_F(EventTest, Wait_Until_NoTimeout)
    {
        // create event
        small::event event;
        event.set_event();

        auto timeStart = small::timeNow();

        auto ret = event.wait_until(timeStart + std::chrono::milliseconds(300));
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_LE(elapsed, 100); // check some time even there is no delay
        ASSERT_EQ(ret, std::cv_status::no_timeout);
    }

    //
    // wait until condition
    //
    TEST_F(EventTest, Wait_Until_Condition_Timeout)
    {
        // create manual event
        small::event event(small::EventType::kEvent_Manual);

        auto timeStart = small::timeNow();
        int conditionEvaluatedInc = 0;

        // wait to be signaled in the thread
        auto ret = event.wait_until(timeStart + std::chrono::milliseconds(300), [&]() {
            return false;
        });
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(conditionEvaluatedInc, 0);
        ASSERT_EQ(ret, std::cv_status::timeout);
    }

    TEST_F(EventTest, Wait_Until_Condition_NoTimeout)
    {
        // create manual event
        small::event event;
        event.set_event();

        auto timeStart = small::timeNow();
        int conditionEvaluatedInc = 0;

        // wait to be signaled in the thread
        auto ret = event.wait_until(timeStart + std::chrono::milliseconds(300), [&]() {
            conditionEvaluatedInc++;
            return true;
        });
        auto elapsed = small::timeDiffMs(timeStart);

        ASSERT_LE(elapsed, 100); // check some time even there is no delay
        ASSERT_EQ(conditionEvaluatedInc, 1);
        ASSERT_EQ(ret, std::cv_status::no_timeout);
    }

} // namespace