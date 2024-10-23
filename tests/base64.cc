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
        const std::string m_base64 = "c29tZSB0ZXh0";
    };

    TEST_F(Base64Test, Base64_Null_Terminating_String)
    {
        auto b64 = small::tobase64_s(m_text.c_str(), m_text.size());
        ASSERT_EQ(b64, m_base64);
    }
} // namespace