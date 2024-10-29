#include <gtest/gtest.h>

#include "../include/hash.h"

namespace {
    class HashTest : public testing::Test
    {
    protected:
        HashTest() = default;

        void SetUp() override
        {
            // setup before test
        }
        void TearDown() override
        {
            // clean after test
        }

        const std::string m_text = "some text";
        const unsigned long long m_hash = 10048017132714414241ULL;
    };

    TEST_F(HashTest, Hash_Null_Terminating_String)
    {
        unsigned long long hash = small::qhashz(m_text.c_str());
        ASSERT_EQ(hash, m_hash);
    }

    TEST_F(HashTest, Hash_buffer)
    {
        unsigned long long hash = small::qhash(m_text.c_str(), m_text.size());
        ASSERT_EQ(hash, m_hash);
    }

    TEST_F(HashTest, Hash_String)
    {
        unsigned long long hash = small::qhash(m_text);
        ASSERT_EQ(hash, m_hash);
    }

    TEST_F(HashTest, Hash_Multiple)
    {
        unsigned long long hash1 = small::qhash(m_text.c_str(), 5, 0 /*start hash*/);
        unsigned long long hash2 = small::qhashz(m_text.c_str() + 5, hash1);
        ASSERT_EQ(hash2, m_hash);
    }

} // namespace