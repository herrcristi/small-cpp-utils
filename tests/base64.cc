#include <gtest/gtest.h>

#include "../include/base64.h"

namespace {
    class Base64Test : public testing::Test
    {
    protected:
        Base64Test() = default;

        void SetUp() override {}
        void TearDown() override {}

        const std::string m_text = "some text";
        const unsigned long long m_hash = 10048017132714414241ULL;
    };

    TEST_F(Base64Test, Base64_Null_Terminating_String)
    {
        // unsigned long long hash = small::quick_hash_z(m_text.c_str(), 0);
        // ASSERT_EQ(hash, m_hash);
    }
} // namespace