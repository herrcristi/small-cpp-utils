#include "test_common.h"

#include "../include/lru_cache.h"

namespace {
    class LRUCacheTest : public testing::Test
    {
    protected:
        LRUCacheTest() = default;

        void SetUp() override
        {
            // setup before test
        }
        void TearDown() override
        {
            // clean after test
        }
    };

    //
    // operations
    //
    TEST_F(LRUCacheTest, operations)
    {
        small::lru_cache<int, std::string> cache({.capacity = 2});
        ASSERT_EQ(cache.size(), 0);

        // A
        cache.set(1, "A");
        ASSERT_EQ(cache.size(), 1);

        // B A
        cache.set(2, "B");
        ASSERT_EQ(cache.size(), 2);

        // A B
        ASSERT_EQ(*cache.get(1), "A"); // will be promoted first

        // C A
        cache.set(3, "C");
        ASSERT_EQ(cache.size(), 2); // oldest element will be removed
        ASSERT_EQ(cache.get(2), nullptr);

        // D C
        cache.set(4, "D");
        ASSERT_EQ(cache.size(), 2); // oldest element will be removed
        ASSERT_EQ(cache.get(1), nullptr);

        // C
        cache.erase(4);
        ASSERT_EQ(cache.size(), 1);
        ASSERT_EQ(cache.get(4), nullptr);
    }

    TEST_F(LRUCacheTest, no_capacity)
    {
        small::lru_cache<int, std::string> cache({.capacity = 0});
        ASSERT_EQ(cache.size(), 0);

        cache.set(1, "A");
        ASSERT_EQ(cache.size(), 0); // not added
        ASSERT_EQ(cache.get(1), nullptr);
    }

    TEST_F(LRUCacheTest, update_existing_key)
    {
        small::lru_cache<int, std::string> cache({.capacity = 2});
        ASSERT_EQ(cache.size(), 0);

        // A
        cache.set(1, "A");
        ASSERT_EQ(cache.size(), 1);

        // B A
        cache.set(2, "B");
        ASSERT_EQ(cache.size(), 2);

        // Update A to C -> C B
        cache.set(1, "C");
        ASSERT_EQ(cache.size(), 2);
        ASSERT_EQ(*cache.get(1), "C");
    }

    TEST_F(LRUCacheTest, clear_cache)
    {
        small::lru_cache<int, std::string> cache({.capacity = 2});
        ASSERT_EQ(cache.size(), 0);

        // A
        cache.set(1, "A");
        ASSERT_EQ(cache.size(), 1);

        // B A
        cache.set(2, "B");
        ASSERT_EQ(cache.size(), 2);

        // Clear cache
        cache.clear();
        ASSERT_EQ(cache.size(), 0);
        ASSERT_EQ(cache.get(1), nullptr);
        ASSERT_EQ(cache.get(2), nullptr);
    }

    TEST_F(LRUCacheTest, get_nonexistent_key)
    {
        small::lru_cache<int, std::string> cache({.capacity = 2});
        ASSERT_EQ(cache.size(), 0);

        // A
        cache.set(1, "A");
        ASSERT_EQ(cache.size(), 1);

        // B A
        cache.set(2, "B");
        ASSERT_EQ(cache.size(), 2);

        // Get non-existent key
        ASSERT_EQ(cache.get(3), nullptr);
        ASSERT_EQ(cache.size(), 2);
    }

    TEST_F(LRUCacheTest, erase_nonexistent_key)
    {
        small::lru_cache<int, std::string> cache({.capacity = 2});
        ASSERT_EQ(cache.size(), 0);

        // A
        cache.set(1, "A");
        ASSERT_EQ(cache.size(), 1);

        // B A
        cache.set(2, "B");
        ASSERT_EQ(cache.size(), 2);

        // Erase non-existent key
        cache.erase(3);
        ASSERT_EQ(cache.size(), 2);
    }

    TEST_F(LRUCacheTest, copy_constructor)
    {
        small::lru_cache<int, std::string> cache1({.capacity = 2});
        cache1.set(1, "A");
        cache1.set(2, "B");

        small::lru_cache<int, std::string> cache2 = cache1;
        ASSERT_EQ(cache2.size(), 2);
        ASSERT_EQ(*cache2.get(1), "A");
        ASSERT_EQ(*cache2.get(2), "B");
        cache2.set(3, "C");
        ASSERT_EQ(cache2.size(), 2); // capacity is 2 like cache1

        ASSERT_EQ(cache1.size(), 2); // cache1 should not be affected
    }

    TEST_F(LRUCacheTest, copy_assignment)
    {
        small::lru_cache<int, std::string> cache1({.capacity = 2});
        cache1.set(1, "A");
        cache1.set(2, "B");

        small::lru_cache<int, std::string> cache2({.capacity = 3});
        cache2 = cache1;
        ASSERT_EQ(cache2.size(), 2);
        ASSERT_EQ(*cache2.get(1), "A");
        ASSERT_EQ(*cache2.get(2), "B");
        cache2.set(3, "C");
        ASSERT_EQ(cache2.size(), 2); // capacity is 2 like cache1

        ASSERT_EQ(cache1.size(), 2); // cache1 should not be affected
    }

    TEST_F(LRUCacheTest, move_constructor)
    {
        small::lru_cache<int, std::string> cache1({.capacity = 2});
        cache1.set(1, "A");
        cache1.set(2, "B");

        small::lru_cache<int, std::string> cache2 = std::move(cache1);
        ASSERT_EQ(cache2.size(), 2);
        ASSERT_EQ(*cache2.get(1), "A");
        ASSERT_EQ(*cache2.get(2), "B");
        cache2.set(3, "C");
        ASSERT_EQ(cache2.size(), 2); // capacity is 2 like cache1

        ASSERT_EQ(cache1.size(), 0); // cache1 should be empty after move
    }

    TEST_F(LRUCacheTest, move_assignment)
    {
        small::lru_cache<int, std::string> cache1({.capacity = 2});
        cache1.set(1, "A");
        cache1.set(2, "B");

        small::lru_cache<int, std::string> cache2({.capacity = 3});
        cache2 = std::move(cache1);
        ASSERT_EQ(cache2.size(), 2);
        ASSERT_EQ(*cache2.get(1), "A");
        ASSERT_EQ(*cache2.get(2), "B");
        cache2.set(3, "C");
        ASSERT_EQ(cache2.size(), 2); // capacity is 2 like cache1

        ASSERT_EQ(cache1.size(), 0); // cache1 should be empty after move
    }

} // namespace