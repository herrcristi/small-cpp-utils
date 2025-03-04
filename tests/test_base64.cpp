#include "test_common.h"

#include "../include/base64.h"

namespace {
    class Base64Test : public testing::Test
    {
    protected:
        Base64Test() = default;

        void SetUp() override
        {
            // setup before test
        }
        void TearDown() override
        {
            // clean after test
        }

        const std::string m_text   = "hello world";
        const std::string m_base64 = "aGVsbG8gd29ybGQ=";
    };

    //
    // to base64
    //
    TEST_F(Base64Test, ToBase64_char)
    {
        // as char + length
        auto b64 = small::tobase64(m_text.c_str(), m_text.size());
        ASSERT_EQ(b64, m_base64);
    }

    TEST_F(Base64Test, ToBase64_string)
    {
        // as string_view
        auto b64 = small::tobase64(m_text);
        ASSERT_EQ(b64, m_base64);

        auto b64v = small::tobase64<std::vector<char>>(m_text);
        ASSERT_EQ(std::string_view(b64v.data(), b64v.size()), m_base64);

        auto b64vu = small::tobase64<std::vector<unsigned char>>(m_text);
        ASSERT_EQ(std::string_view(reinterpret_cast<const char*>(b64v.data()), b64vu.size()), m_base64);

        auto b64b = small::tobase64<small::buffer>(m_text);
        ASSERT_EQ(b64b, m_base64);
    }

    TEST_F(Base64Test, ToBase64_vector)
    {
        // as vector
        std::vector<char> v;
        for (auto ch : m_text) {
            v.push_back(ch);
        }
        auto b64 = small::tobase64(v);
        ASSERT_EQ(b64, m_base64);
    }

    TEST_F(Base64Test, ToBase64_buffer)
    {
        // as buffer
        small::buffer b   = m_text;
        auto          b64 = small::tobase64(b);
        ASSERT_EQ(b64, m_base64);
    }

    //
    // from base64
    //
    TEST_F(Base64Test, FromBase64_char)
    {
        // as char + length
        auto decoded = small::frombase64(m_base64.c_str(), m_base64.size());
        ASSERT_EQ(decoded, m_text);
    }

    TEST_F(Base64Test, FromBase64_string)
    {
        // as string_view
        auto decoded = small::frombase64(m_base64);
        ASSERT_EQ(decoded, m_text);

        auto dv = small::frombase64<std::vector<char>>(m_base64.data(), m_base64.size());
        ASSERT_EQ(std::string_view(dv.data(), dv.size()), m_text);

        auto dvu = small::frombase64<std::vector<unsigned char>>(m_base64);
        ASSERT_EQ(std::string_view(reinterpret_cast<const char*>(dvu.data()), dvu.size()), m_text);

        auto db = small::frombase64<small::buffer>(m_base64);
        ASSERT_EQ(db, m_text);
    }

    TEST_F(Base64Test, FromBase64_vector)
    {
        // as vector
        std::vector<char> v;
        for (auto ch : m_base64) {
            v.push_back(ch);
        }
        auto decoded = small::frombase64(v);
        ASSERT_EQ(decoded, m_text);
    }

    TEST_F(Base64Test, FromBase64_buffer)
    {
        // as buffer
        small::buffer b       = m_base64;
        auto          decoded = small::frombase64(b);
        ASSERT_EQ(decoded, m_text);
    }
} // namespace