#include "test_common.h"

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