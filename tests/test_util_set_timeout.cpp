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
        }
        void TearDown() override
        {
            // cleanup after test
        }
    };

    //
    // operations
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

} // namespace