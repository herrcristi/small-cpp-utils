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

    TEST_F(HashTest, Hash_Null_Terminating_string)
    {
        unsigned long long hash = small::quick_hash_z(m_text.c_str(), 0);
        ASSERT_EQ(hash, m_hash);
    }
} // namespace