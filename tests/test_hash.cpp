
#include "test_common.h"

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

        const std::string        m_text    = "some text";
        const unsigned long long m_hash131 = 10048017132714414241ULL;
        const unsigned long long m_hash1a  = 1565534772893300484ULL;
    };

    TEST_F(HashTest, Hash_FNV1a_Null_Terminating_String)
    {
        // Using FNV-1a algorithm (FNV_1a is now the default qhashz)
        unsigned long long hash = small::qhash1az(m_text.c_str());
        // Hash value will be different from old 131x algorithm
        // Just verify it's non-zero and consistent
        ASSERT_EQ(hash, m_hash1a);
    }

    TEST_F(HashTest, Hash_FNV1a_buffer)
    {
        // Using FNV-1a algorithm
        unsigned long long hash = small::qhash1a(m_text.c_str(), m_text.size());
        ASSERT_EQ(hash, m_hash1a);
    }

    TEST_F(HashTest, Hash_FNV1a_String)
    {
        // Using FNV-1a algorithm
        unsigned long long hash = small::qhash1a(m_text);
        ASSERT_EQ(hash, m_hash1a);
    }

    TEST_F(HashTest, Hash_FNV1a_Multiple)
    {
        // Using FNV-1a algorithm
        unsigned long long hash1 = small::qhash1a(m_text.c_str(), 5, 0 /*start hash*/);
        unsigned long long hash2 = small::qhash1az(m_text.c_str() + 5, hash1);
        ASSERT_EQ(hash2, m_hash1a);
    }

    // Test that different inputs produce different hashes (no collisions)
    TEST_F(HashTest, Hash_FNV1a_No_Collisions)
    {
        unsigned long long hash1 = small::qhash1a("test1");
        unsigned long long hash2 = small::qhash1a("test2");
        unsigned long long hash3 = small::qhash1a("different");

        ASSERT_NE(hash1, hash2);
        ASSERT_NE(hash2, hash3);
        ASSERT_NE(hash1, hash3);
    }

    TEST_F(HashTest, Hash_131_Null_Terminating_String)
    {
        // Using 131x algorithm
        unsigned long long hash = small::qhash131z(m_text.c_str());
        // Hash value will be different from old 131x algorithm
        // Just verify it's non-zero and consistent
        ASSERT_EQ(hash, m_hash131);
    }

    TEST_F(HashTest, Hash_131_buffer)
    {
        // Using FNV-1a algorithm
        unsigned long long hash = small::qhash131(m_text.c_str(), m_text.size());
        ASSERT_EQ(hash, m_hash131);
    }

    TEST_F(HashTest, Hash_131_String)
    {
        // Using 131x algorithm
        unsigned long long hash = small::qhash131(m_text);
        ASSERT_EQ(hash, m_hash131);
    }

    TEST_F(HashTest, Hash_131_Multiple)
    {
        // Using 131x algorithm
        unsigned long long hash1 = small::qhash131(m_text.c_str(), 5, 0 /*start hash*/);
        unsigned long long hash2 = small::qhash131z(m_text.c_str() + 5, hash1);
        ASSERT_EQ(hash2, m_hash131);
    }

    // Test that different inputs produce different hashes (no collisions)
    TEST_F(HashTest, Hash_131_No_Collisions)
    {
        unsigned long long hash1 = small::qhash131("test1");
        unsigned long long hash2 = small::qhash131("test2");
        unsigned long long hash3 = small::qhash131("different");

        ASSERT_NE(hash1, hash2);
        ASSERT_NE(hash2, hash3);
        ASSERT_NE(hash1, hash3);
    }

} // namespace