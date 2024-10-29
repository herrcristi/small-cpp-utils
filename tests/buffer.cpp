#include <gtest/gtest.h>

#include "../include/buffer.h"

namespace {
    class BufferTest : public testing::Test
    {
    protected:
        BufferTest() = default;

        void SetUp() override
        {
            // setup before test
        }
        void TearDown() override
        {
            // clean after test
        }

        std::string m_test = "some text";
    };

    //
    // buffer
    //
    TEST_F(BufferTest, buffer)
    {
        small::buffer b = m_test;
        ASSERT_EQ(b, m_test);
        ASSERT_EQ(b.get_chunk_size(), small::DEFAULT_BUFFER_CHUNK_SIZE);

        small::buffer b1{'a'};
        ASSERT_EQ(b1, "a");

        small::buffer b2{"abc"};
        ASSERT_EQ(b2, "abc");

        small::buffer b3{"abc", 2};
        ASSERT_EQ(b3, "ab");

        constexpr std::string_view sv = "abc";
        small::buffer b4{sv};
        ASSERT_EQ(b4, "abc");

        small::buffer b5{0 /*alloc size*/, sv};
        ASSERT_EQ(b5, "abc");
        ASSERT_EQ(b5.get_chunk_size(), 1);
    }

    TEST_F(BufferTest, buffer_operator_eq)
    {
        small::buffer b;
        b = m_test;
        ASSERT_EQ(b, m_test);
        ASSERT_EQ(b.get_chunk_size(), small::DEFAULT_BUFFER_CHUNK_SIZE);

        small::buffer b1;
        b1 = 'a';
        ASSERT_EQ(b1, "a");

        small::buffer b2;
        b2 = {"abc"};
        ASSERT_EQ(b2, "abc");

        b2 = b1;
        ASSERT_EQ(b2, "a");

        small::buffer b3;
        b3 = {"abc", 2};
        ASSERT_EQ(b3, "ab");

        constexpr std::string_view sv = "abc";
        small::buffer b4;
        b4 = {sv};
        ASSERT_EQ(b4, "abc");

        small::buffer b5(1UL);
        b5 = sv;
        ASSERT_EQ(b5.get_chunk_size(), small::DEFAULT_BUFFER_CHUNK_SIZE);
        ASSERT_EQ(b5, "abc");
        b5.set_chunk_size(1UL);
        ASSERT_EQ(b5.get_chunk_size(), 1);
    }

    TEST_F(BufferTest, buffer_extract)
    {
        struct AutoDelete
        {
            char *m_e{};
            explicit AutoDelete(char *e) : m_e(e) {}
            ~AutoDelete() { small::buffer::free(m_e); };
        };

        small::buffer b;
        b = m_test;
        ASSERT_EQ(b, m_test);

        auto e = b.extract();
        AutoDelete ad(e);

        ASSERT_EQ(e, m_test);
        ASSERT_EQ(b, "");

        small::buffer b1;
        ASSERT_EQ(b1, "");

        auto e1 = b1.extract();
        AutoDelete ad1(e1);
        b1 += 'a';

        ASSERT_EQ(b1, "a");
        ASSERT_EQ(e1, std::string());
    }

    TEST_F(BufferTest, buffer_plus)
    {
        small::buffer b;
        b = m_test;
        ASSERT_EQ(b, m_test);
        ASSERT_EQ(b.get_chunk_size(), small::DEFAULT_BUFFER_CHUNK_SIZE);

        b += 'a';
        ASSERT_EQ(b, m_test + "a");

        b += "b";
        ASSERT_EQ(b, m_test + "ab");

        b += {"c", 1};
        ASSERT_EQ(b, m_test + "abc");

        constexpr std::string_view sv = "d";
        b += {sv};
        ASSERT_EQ(b, m_test + "abcd");
    }

} // namespace