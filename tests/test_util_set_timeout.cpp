#include <gtest/gtest.h>

#include <latch>

#include "../include/util.h"
#include "../include/util_timeout.h"

namespace {
    class UtilSetTimeoutTest : public testing::Test
    {
    protected:
        UtilSetTimeoutTest() = default;

        void SetUp() override
        {
            // initial setup
            intervalID = 0;
        }
        void TearDown() override
        {
            // cleanup after test
            if (intervalID) {
                auto ret = small::clear_interval(intervalID);
                ASSERT_EQ(ret, true);
            }
        }

        unsigned long long intervalID{};
    };

    //
    // set timeout operations
    //
    TEST_F(UtilSetTimeoutTest, SetTimeout)
    {
        int processing_count = 0;

        // set timeout
        auto timeoutID = small::set_timeout(std::chrono::milliseconds(300), [&processing_count]() {
            ++processing_count;
        });

        ASSERT_EQ(processing_count, 0);

        small::sleep(400);

        ASSERT_EQ(processing_count, 1);

        // already executed, nothing to clear
        auto ret = small::clear_timeout(timeoutID);
        ASSERT_EQ(ret, false);
    }

    TEST_F(UtilSetTimeoutTest, ClearTimeout)
    {
        int processing_count = 0;

        // set timeout
        auto timeoutID = small::set_timeout(std::chrono::milliseconds(300), [&processing_count]() {
            ++processing_count;
        });

        ASSERT_EQ(processing_count, 0);

        // this will cancel the execution
        auto ret = small::clear_timeout(timeoutID);
        ASSERT_TRUE(ret);

        small::sleep(400);

        ASSERT_EQ(processing_count, 0);
    }

    //
    // set interval operations
    //
    TEST_F(UtilSetTimeoutTest, SetIntervalOneExecution)
    {
        int processing_count = 0;

        // set interval
        intervalID = small::set_interval(std::chrono::milliseconds(300), [&processing_count]() {
            ++processing_count;
        });

        ASSERT_EQ(processing_count, 0);

        small::sleep(400);

        ASSERT_EQ(processing_count, 1);

        // this will cancel the execution
        auto ret = small::clear_interval(intervalID);
        ASSERT_TRUE(ret);
        intervalID = 0;

        small::sleep(400);
        ASSERT_EQ(processing_count, 1);
    }

    TEST_F(UtilSetTimeoutTest, SetIntervalManyExecutions)
    {
        int processing_count = 0;

        // set interval
        intervalID = small::set_interval(std::chrono::milliseconds(100), [&processing_count]() {
            ++processing_count;
        });

        ASSERT_EQ(processing_count, 0);

        small::sleep(350);

        ASSERT_EQ(processing_count, 3);

        // this will cancel the execution
        auto ret = small::clear_interval(intervalID);
        ASSERT_TRUE(ret);
        intervalID = 0;

        small::sleep(200);
        ASSERT_EQ(processing_count, 3);
    }

    TEST_F(UtilSetTimeoutTest, ClearIntervalBeforeNoExecution)
    {
        int processing_count = 0;

        // set timeout
        intervalID = small::set_interval(std::chrono::milliseconds(300), [&processing_count]() {
            ++processing_count;
        });

        ASSERT_EQ(processing_count, 0);

        // this will cancel the execution
        auto ret = small::clear_interval(intervalID);
        ASSERT_TRUE(ret);
        intervalID = 0;

        small::sleep(400);

        ASSERT_EQ(processing_count, 0);
    }

    TEST_F(UtilSetTimeoutTest, ClearIntervalAfterOneExecution)
    {
        int processing_count = 0;

        // set timeout
        intervalID = small::set_interval(std::chrono::milliseconds(300), [&processing_count]() {
            ++processing_count;
        });

        ASSERT_EQ(processing_count, 0);

        small::sleep(400);
        ASSERT_EQ(processing_count, 1);

        // this will cancel the execution
        auto ret = small::clear_interval(intervalID);
        ASSERT_TRUE(ret);
        intervalID = 0;

        small::sleep(400);
        ASSERT_EQ(processing_count, 1);
    }

    TEST_F(UtilSetTimeoutTest, ClearIntervalWhileIsInExecution)
    {
        int processing_count1 = 0;
        int processing_count2 = 0;

        // set timeout
        intervalID = small::set_interval(std::chrono::milliseconds(300), [&processing_count1, &processing_count2]() {
            ++processing_count1;
            small::sleep(300);
            ++processing_count2;
        });

        ASSERT_EQ(processing_count1, 0);
        ASSERT_EQ(processing_count2, 0);

        small::sleep(400);
        ASSERT_EQ(processing_count1, 1); // inside execution
        ASSERT_EQ(processing_count2, 0); // inside execution

        // this will cancel the execution
        auto ret = small::clear_interval(intervalID);
        ASSERT_TRUE(ret);
        intervalID = 0;

        small::sleep(400);
        ASSERT_EQ(processing_count1, 1);
        ASSERT_EQ(processing_count2, 1);
    }

} // namespace