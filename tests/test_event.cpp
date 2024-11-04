#include <gtest/gtest.h>

#include "../include/event.h"

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
            // clean after test
        }
    };

    TEST_F(EventTest, Lock)
    {
        small::event event;
        event.lock();

        // try to lock and it wont succeed
        auto locked = event.try_lock();
        ASSERT_FALSE(locked);

        // unlock
        event.unlock();

        // locking again will succeed
        locked = event.try_lock();
        ASSERT_TRUE(locked);

        event.unlock();
    }
} // namespace