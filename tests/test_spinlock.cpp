#pragma warning(disable : 4464) // relative include path contains '..'
#pragma warning(disable : 4577) // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed

#include <gtest/gtest.h>

#include "../include/spinlock.h"

namespace {
    class SpinLockTest : public testing::Test
    {
    protected:
        SpinLockTest() = default;

        void SetUp() override
        {
            // setup before test
        }
        void TearDown() override
        {
            // clean after test
        }

        // test data
        small::spinlock m_lock;
    };

    TEST_F(SpinLockTest, Lock)
    {
        m_lock.lock();

        // try to lock and it wont succeed
        auto locked = m_lock.try_lock();
        ASSERT_FALSE(locked);

        // unlock
        m_lock.unlock();

        // locking again will succeed
        locked = m_lock.try_lock();
        ASSERT_TRUE(locked);

        m_lock.unlock();
    }
} // namespace