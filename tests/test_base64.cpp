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

    //
    // Base64 RFC 4648 Validation Tests
    //
    TEST_F(Base64Test, Base64_RFC4648_ValidPadding)
    {
        // Valid base64 strings with proper padding
        ASSERT_EQ(small::frombase64("YWJjZA=="), "abcd");

        ASSERT_EQ(small::frombase64("YWJj"), "abc");
        ASSERT_EQ(small::frombase64("YWI="), "ab");

        // Mixed content
        ASSERT_EQ(small::frombase64("SGVsbG8gV29ybGQ="), "Hello World");
        ASSERT_EQ(small::frombase64("VGhpcyBpcyBhIHRlc3Q="), "This is a test");
    }

    TEST_F(Base64Test, Base64_RFC4648_InvalidPadding)
    {
        // Invalid padding patterns - should return empty string
        // 3 padding characters (invalid)
        ASSERT_EQ(small::frombase64("YQ==="), "");

        // 4 padding characters (invalid)
        ASSERT_EQ(small::frombase64("===="), "");

        // Padding in the middle (invalid)
        ASSERT_EQ(small::frombase64("YQ==YQ=="), "");

        // Padding not at multiple of 4 (invalid length)
        ASSERT_EQ(small::frombase64("YQ="), "");     // 3 chars
        ASSERT_EQ(small::frombase64("YQ==YQ="), ""); // 7 chars
    }

    TEST_F(Base64Test, Base64_RFC4648_InvalidCharacters)
    {
        // Invalid characters - should return empty string
        // Space characters
        ASSERT_EQ(small::frombase64("YQ == "), "");

        // Special characters not in base64 alphabet
        ASSERT_EQ(small::frombase64("YQ!@#="), "");

        // Newlines or whitespace
        ASSERT_EQ(small::frombase64("YQ\n=="), "");
        ASSERT_EQ(small::frombase64("\tYQ=="), "");
    }

    TEST_F(Base64Test, Base64_RFC4648_LengthValidation)
    {
        // Length must be multiple of 4 (RFC 4648 requirement)
        ASSERT_EQ(small::frombase64(""), "");             // empty is valid
        ASSERT_EQ(small::frombase64("YQ=="), "a");        // 4 chars
        ASSERT_EQ(small::frombase64("YWJjZA=="), "abcd"); // 8 chars

        // Invalid lengths (not multiple of 4)
        ASSERT_EQ(small::frombase64("YQ"), "");      // 2 chars
        ASSERT_EQ(small::frombase64("YQ="), "");     // 3 chars
        ASSERT_EQ(small::frombase64("YQ==Y"), "");   // 5 chars
        ASSERT_EQ(small::frombase64("YQ==YLB"), ""); // 7 chars
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
            ASSERT_EQ(decoded, original);
        }
    }

    TEST_F(Base64Test, Base64_ErrorHandling)
    {
        // Test that invalid base64 returns empty string and sets null terminator
        char buffer[32] = {0};

        // Invalid input should set first byte to '\0' and return 0
        std::size_t len = small::base64impl::frombase64(buffer, "!!!!", 4);
        ASSERT_EQ(len, 0);
        ASSERT_EQ(buffer[0], '\0');

        // Non-multiple of 4 length
        len = small::base64impl::frombase64(buffer, "YQ", 2);
        ASSERT_EQ(len, 0);
        ASSERT_EQ(buffer[0], '\0');

        // Valid input should work
        len = small::base64impl::frombase64(buffer, "YQ==", 4);
        ASSERT_GT(len, 0);
        buffer[len] = '\0'; // null terminate for comparison
        ASSERT_EQ(std::string(buffer, len), "a");
    }
} // namespace