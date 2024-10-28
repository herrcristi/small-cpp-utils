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

        small::buffer b3("abc", 2);
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

} // namespace