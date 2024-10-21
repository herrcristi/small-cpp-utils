#include <gtest/gtest.h>

#include "../include/spinlock.h"

namespace {
    class SpinLockTest : public testing::Test
    {
    protected:
        SpinLockTest() = default;

        // static bool Is3(int n) { return n == 3; }
    };

    TEST_F(SpinLockTest, Lock)
    {
        small::spinlock lock1;

        lock1.lock();

        // try to lock and it wont succeed
        auto locked = lock1.try_lock();
        std::cout << "Test returned " << locked << "\n";
        EXPECT_FALSE(locked);

        // unlock first one
        lock1.unlock();

        // locking again will succeed
        locked = lock1.try_lock();
        EXPECT_TRUE(locked);

        lock1.unlock();
    }
} // namespace