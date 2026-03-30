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
        ASSERT_TRUE(decoded.has_value());
        ASSERT_EQ(*decoded, m_text);
    }

    TEST_F(Base64Test, FromBase64_string)
    {
        // as string_view
        auto decoded = small::frombase64(m_base64);
        ASSERT_TRUE(decoded.has_value());
        ASSERT_EQ(*decoded, m_text);

        auto dv = small::frombase64<std::vector<char>>(m_base64.data(), m_base64.size());
        ASSERT_TRUE(dv.has_value());
        ASSERT_EQ(std::string_view(dv->data(), dv->size()), m_text);

        auto dvu = small::frombase64<std::vector<unsigned char>>(m_base64);
        ASSERT_TRUE(dvu.has_value());
        ASSERT_EQ(std::string_view(reinterpret_cast<const char*>(dvu->data()), dvu->size()), m_text);

        auto db = small::frombase64<small::buffer>(m_base64);
        ASSERT_TRUE(db.has_value());
        ASSERT_EQ(*db, m_text);
    }

    TEST_F(Base64Test, FromBase64_vector)
    {
        // as vector
        std::vector<char> v;
        for (auto ch : m_base64) {
            v.push_back(ch);
        }
        auto decoded = small::frombase64(v);
        ASSERT_TRUE(decoded.has_value());
        ASSERT_EQ(*decoded, m_text);
    }

    TEST_F(Base64Test, FromBase64_buffer)
    {
        // as buffer
        small::buffer b       = m_base64;
        auto          decoded = small::frombase64(b);
        ASSERT_TRUE(decoded.has_value());
        ASSERT_EQ(*decoded, m_text);
    }

    //
    // Base64 RFC 4648 Validation Tests
    //
    TEST_F(Base64Test, Base64_RFC4648_ValidPadding)
    {
        // Valid base64 strings with proper padding
        auto result = small::frombase64("YWJjZA==");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "abcd");

        result = small::frombase64("YWJj");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "abc");

        result = small::frombase64("YWI=");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "ab");

        // Mixed content
        result = small::frombase64("SGVsbG8gV29ybGQ=");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "Hello World");

        result = small::frombase64("VGhpcyBpcyBhIHRlc3Q=");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "This is a test");
    }

    TEST_F(Base64Test, Base64_RFC4648_InvalidPadding)
    {
        // Invalid padding patterns - should return nullopt
        // 3 padding characters (invalid)
        ASSERT_FALSE(small::frombase64("YQ===").has_value());

        // 4 padding characters (invalid)
        ASSERT_FALSE(small::frombase64("====").has_value());

        // Padding in the middle (invalid)
        ASSERT_FALSE(small::frombase64("YQ==YQ==").has_value());

        // Padding not at multiple of 4 (invalid length)
        ASSERT_FALSE(small::frombase64("YQ=").has_value());     // 3 chars
        ASSERT_FALSE(small::frombase64("YQ==YQ=").has_value()); // 7 chars
    }

    TEST_F(Base64Test, Base64_RFC4648_InvalidCharacters)
    {
        // Invalid characters - should return nullopt
        // Space characters
        ASSERT_FALSE(small::frombase64("YQ == ").has_value());

        // Special characters not in base64 alphabet
        ASSERT_FALSE(small::frombase64("YQ!@#=").has_value());

        // Newlines or whitespace
        ASSERT_FALSE(small::frombase64("YQ\n==").has_value());
        ASSERT_FALSE(small::frombase64("\tYQ==").has_value());
    }

    TEST_F(Base64Test, Base64_RFC4648_LengthValidation)
    {
        // Length must be multiple of 4 (RFC 4648 requirement)
        // Empty is valid
        auto result = small::frombase64("");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "");

        // 4 chars is valid
        result = small::frombase64("YQ==");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "a");

        // 8 chars is valid
        result = small::frombase64("YWJjZA==");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, "abcd");

        // Invalid lengths (not multiple of 4)
        ASSERT_FALSE(small::frombase64("YQ").has_value());      // 2 chars
        ASSERT_FALSE(small::frombase64("YQ=").has_value());     // 3 chars
        ASSERT_FALSE(small::frombase64("YQ==Y").has_value());   // 5 chars
        ASSERT_FALSE(small::frombase64("YQ==YLB").has_value()); // 7 chars
    }

    TEST_F(Base64Test, Base64_RFC4648_RoundTrip)
    {
        // Test round-trip encoding/decoding with validation
        std::vector<std::string> test_strings = {
            "",
            "a",
            "ab",
            "abc",
            "abcd",
            "hello",
            "hello world",
            "This is a test with spaces",
            "Special!@#$%^&*()_+-=[]{}|;:',.<>?/",
            "\x00\x01\x02\x03\x04\x05", // binary data
        };

        for (const auto& original : test_strings) {
            // Encode
            auto encoded = small::tobase64(original);

            // Decode and verify round-trip
            auto decoded = small::frombase64(encoded);
            ASSERT_TRUE(decoded.has_value());
            ASSERT_EQ(*decoded, original);
        }
    }

    TEST_F(Base64Test, Base64_ErrorHandling)
    {
        // Test that invalid base64 returns nullopt and sets null terminator
        char buffer[32] = {0};

        // Invalid input should return nullopt and set buffer[0] = '\0'
        auto result = small::base64impl::frombase64(buffer, "!!!!", 4);
        ASSERT_FALSE(result.has_value());
        ASSERT_EQ(buffer[0], '\0');

        // Non-multiple of 4 length
        result = small::base64impl::frombase64(buffer, "YQ", 2);
        ASSERT_FALSE(result.has_value());
        ASSERT_EQ(buffer[0], '\0');

        // Valid input should return decoded length
        result = small::base64impl::frombase64(buffer, "YQ==", 4);
        ASSERT_TRUE(result.has_value());
        ASSERT_GT(*result, 0);
        buffer[*result] = '\0'; // null terminate for comparison
        ASSERT_EQ(std::string(buffer, *result), "a");
    }
} // namespace