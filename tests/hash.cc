#include <gtest/gtest.h>

#include "../include/hash.h"

namespace {
    class HashTest : public testing::Test
    {
    protected:
        HashTest() = default;

        void SetUp() override {}
        void TearDown() override {}

        const std::string m_text = "some text";
        const unsigned long long m_hash = 10048017132714414241ULL;
    };

    TEST_F(HashTest, Hash_Null_Terminating_String)
    {
        unsigned long long hash = small::quick_hash_z(m_text.c_str(), 0);
        ASSERT_EQ(hash, m_hash);
    }

    TEST_F(HashTest, Hash_buffer)
    {
        unsigned long long hash = small::quick_hash_b(m_text.c_str(), m_text.size(), 0);
        ASSERT_EQ(hash, m_hash);
    }

    TEST_F(HashTest, Hash_Multiple)
    {
        unsigned long long hash1 = small::quick_hash_b(m_text.c_str(), 5, 0);
        unsigned long long hash2 = small::quick_hash_z(m_text.c_str() + 5, hash1);
        ASSERT_EQ(hash2, m_hash);
    }
} // namespace