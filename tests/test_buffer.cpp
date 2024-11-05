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
            char **m_e{};
            explicit AutoDelete(char *&e) : m_e(&e) {}
            ~AutoDelete() { small::buffer::free(*m_e); };
        };

        small::buffer b;
        b = m_test;
        ASSERT_EQ(b, m_test);

        auto e = b.extract();
        {
            AutoDelete ad(e);

            ASSERT_EQ(e, m_test);
            ASSERT_EQ(b, "");
        }
        ASSERT_EQ(e, nullptr);

        small::buffer b1;
        ASSERT_EQ(b1, "");

        auto e1 = b1.extract();
        {
            AutoDelete ad1(e1);
            b1 += 'a';

            ASSERT_EQ(b1, "a");
            ASSERT_EQ(e1, std::string());
        }
        ASSERT_EQ(e1, nullptr);
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

    TEST_F(BufferTest, buffer_swap)
    {
        small::buffer b;
        b = m_test;
        ASSERT_EQ(b, m_test);

        small::buffer b1 = "a";
        ASSERT_EQ(b1, "a");

        b.swap(b1);
        ASSERT_EQ(b, "a");
        ASSERT_EQ(b1, m_test);

        small::buffer b2; // default empty
        b.swap(b2);
        ASSERT_EQ(b, "");
        ASSERT_EQ(b2, "a");

        b2.clear();
        ASSERT_EQ(b2, "");
    }

    TEST_F(BufferTest, buffer_size)
    {
        small::buffer b{m_test};
        ASSERT_EQ(b, m_test);
        ASSERT_EQ(b.size(), m_test.size());
        ASSERT_EQ(b.length(), m_test.size());
        ASSERT_EQ(b.empty(), false);

        b.clear();
        ASSERT_EQ(b.size(), 0);
        ASSERT_EQ(b.empty(), true);
    }

    TEST_F(BufferTest, buffer_resize)
    {
        small::buffer b(1UL, m_test);
        ASSERT_EQ(b, m_test);
        ASSERT_EQ(b.get_chunk_size(), 1);

        b += "abcd";
        ASSERT_EQ(b, m_test + "abcd");

        b.resize(20);
        ASSERT_EQ(b, std::string_view("some textabcd\0\0\0\0\0\0\0", 20));

        b.resize(m_test.size() + 1);
        ASSERT_EQ(b, m_test + "a");
    }

    TEST_F(BufferTest, buffer_data)
    {
        small::buffer b(m_test);
        ASSERT_EQ(b.data(), m_test);
        ASSERT_EQ(b.get_buffer(), m_test);

        ASSERT_EQ(b.begin(), m_test);
        ASSERT_EQ(b.end(), std::string());

        ASSERT_EQ(b.rend(), m_test);
        ASSERT_EQ(b.rbegin(), std::string());
    }

    TEST_F(BufferTest, buffer_views)
    {
        small::buffer b(m_test);
        ASSERT_EQ(b.c_string(), m_test);
        ASSERT_EQ(b.c_view(), m_test);

        auto v = b.c_vector();
        ASSERT_EQ(v.size(), m_test.size());
        ASSERT_EQ(v.data(), m_test);
    }

    TEST_F(BufferTest, buffer_assign)
    {
        small::buffer b;
        b.assign(m_test);
        ASSERT_EQ(b, m_test);

        b.assign('a');
        ASSERT_EQ(b, "a");

        small::buffer b2;
        b2.assign("abc");
        ASSERT_EQ(b2, "abc");

        b2 = b;
        ASSERT_EQ(b2, "a");
        ASSERT_EQ(b, "a");
        b2 += 'b';
        b += 'c';
        ASSERT_EQ(b2, "ab");
        ASSERT_EQ(b, "ac");

        b.assign({"abc", 2});
        ASSERT_EQ(b, "ab");

        constexpr std::string_view sv = "abc";
        b.assign(sv);
        ASSERT_EQ(b, "abc");
    }

    TEST_F(BufferTest, buffer_append)
    {
        small::buffer b;
        b.append(m_test);
        ASSERT_EQ(b, m_test);

        b.append('a');
        ASSERT_EQ(b, m_test + "a");

        b.append("b");
        ASSERT_EQ(b, m_test + "ab");

        b.append({"c", 1});
        ASSERT_EQ(b, m_test + "abc");

        b.append("de", 1);
        ASSERT_EQ(b, m_test + "abcd");

        constexpr std::string_view sv = "f";
        b.append(sv);
        ASSERT_EQ(b, m_test + "abcdf");
    }

    TEST_F(BufferTest, buffer_insert)
    {
        small::buffer b;
        b.insert(0, m_test);
        ASSERT_EQ(b, m_test);

        b.insert(0, 'a');
        ASSERT_EQ(b, "a" + m_test);

        b.insert(0, "b");
        ASSERT_EQ(b, "ba" + m_test);

        b.insert(1, {"c", 1});
        ASSERT_EQ(b, "bca" + m_test);

        b.insert(1, "de", 1);
        ASSERT_EQ(b, "bdca" + m_test);

        constexpr std::string_view sv = "f";
        b.insert(0, sv);
        ASSERT_EQ(b, "fbdca" + m_test);
    }

    TEST_F(BufferTest, buffer_set)
    {
        small::buffer b;
        b.set(0, m_test);
        ASSERT_EQ(b, m_test);

        b.set(0, 'a');
        ASSERT_EQ(b, "a");

        b.set(1, "b");
        ASSERT_EQ(b, "ab");

        b.set(1, {"c", 1});
        ASSERT_EQ(b, "ac");

        b.set(2, "de", 1);
        ASSERT_EQ(b, "acd");

        constexpr std::string_view sv = "f";
        b.set(0, sv);
        ASSERT_EQ(b, "f");

        b.set(2, "g");
        ASSERT_EQ(b, std::string_view("f\0g", 3));
    }

    TEST_F(BufferTest, buffer_overwrite_same_as_set)
    {
        small::buffer b;
        b.overwrite(0, m_test);
        ASSERT_EQ(b, m_test);

        b.overwrite(0, 'a');
        ASSERT_EQ(b, "a");

        b.overwrite(1, "b");
        ASSERT_EQ(b, "ab");

        b.overwrite(1, {"c", 1});
        ASSERT_EQ(b, "ac");

        b.overwrite(2, "de", 1);
        ASSERT_EQ(b, "acd");

        constexpr std::string_view sv = "f";
        b.overwrite(0, sv);
        ASSERT_EQ(b, "f");

        b.overwrite(2, "g");
        ASSERT_EQ(b, std::string_view("f\0g", 3));
    }

    TEST_F(BufferTest, buffer_erase)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b, "abcd");

        b.erase(2);
        ASSERT_EQ(b, "ab");

        b.erase(0);
        ASSERT_EQ(b, "");
    }

    TEST_F(BufferTest, buffer_erase_with_length)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b, "abcd");

        b.erase(2, 1);
        ASSERT_EQ(b, "abd");

        b.erase(0, 2);
        ASSERT_EQ(b, "d");
    }

    TEST_F(BufferTest, buffer_is_eq)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b, "abcd");
        ASSERT_EQ(b.is_equal("abcd", 4), true);
        ASSERT_EQ(b.compare("abcd", 4), 0);
        ASSERT_EQ(b.compare("abc", 3), 1);
        ASSERT_EQ(b.compare("abd", 3), -1);
    }

    TEST_F(BufferTest, buffer_at)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b, "abcd");
        ASSERT_EQ(b[0], 'a');
        ASSERT_EQ(b[1], 'b');

        ASSERT_EQ(b.at(2), 'c');
        ASSERT_EQ(b.at(3), 'd');

        ASSERT_EQ(b.front(), 'a');
        ASSERT_EQ(b.back(), 'd');

        b.erase(0);
        ASSERT_EQ(b, "");
        ASSERT_EQ(b.back(), '\0');
    }

    TEST_F(BufferTest, buffer_push_pop)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b, "abcd");

        b.push_back('e');
        ASSERT_EQ(b, "abcde");

        b.pop_back();
        ASSERT_EQ(b, "abcd");

        b.pop_back();
        ASSERT_EQ(b, "abc");

        b.pop_back();
        ASSERT_EQ(b, "ab");

        b.pop_back();
        ASSERT_EQ(b, "a");

        b.pop_back();
        ASSERT_EQ(b, "");

        b.pop_back();
        ASSERT_EQ(b, "");
    }

    TEST_F(BufferTest, buffer_substr)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.substr(1), "bcd");
        ASSERT_EQ(b.substr(1, 2), "bc");
        ASSERT_EQ(b.substr(0, 5), "abcd");
    }

    TEST_F(BufferTest, buffer_starts_with)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.starts_with('a'), true);
        ASSERT_EQ(b.starts_with({"b", 1}), false);
        ASSERT_EQ(b.starts_with("abc"), true);
        ASSERT_EQ(b.starts_with("abcd"), true);
        ASSERT_EQ(b.starts_with("abcde"), false);
    }

    TEST_F(BufferTest, buffer_ends_with)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.ends_with('d'), true);
        ASSERT_EQ(b.ends_with({"a", 1}), false);
        ASSERT_EQ(b.ends_with("bcd"), true);
        ASSERT_EQ(b.ends_with("abcd"), true);
        ASSERT_EQ(b.ends_with("abcde"), false);
    }

    TEST_F(BufferTest, buffer_contains)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.contains('d'), true);
        ASSERT_EQ(b.contains({"a", 1}), true);
        ASSERT_EQ(b.contains({"e", 1}), false);
        ASSERT_EQ(b.contains("bcd"), true);
        ASSERT_EQ(b.contains("abcd"), true);
        ASSERT_EQ(b.contains("abcde"), false);
    }

    TEST_F(BufferTest, buffer_find)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.find('d'), 3);
        ASSERT_EQ(b.find({"a", 1}), 0);
        ASSERT_EQ(b.find({"a", 1}, 2), std::string::npos);
        ASSERT_EQ(b.find({"e", 1}), std::string::npos);

        ASSERT_EQ(b.find("bcd"), 1);
        ASSERT_EQ(b.find("bcd", 1, 3), 1);
        ASSERT_EQ(b.find("bcd", 2), std::string::npos);
        ASSERT_EQ(b.find("abcd"), 0);
        ASSERT_EQ(b.find("abcde"), std::string::npos);
    }

    TEST_F(BufferTest, buffer_rfind)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.rfind('d'), 3);
        ASSERT_EQ(b.rfind({"c", 1}), 2);
        ASSERT_EQ(b.rfind({"a", 1}), 0);
        ASSERT_EQ(b.rfind({"e", 1}), std::string::npos);

        ASSERT_EQ(b.rfind("bcd"), 1);
        ASSERT_EQ(b.rfind("bcd", 1, 3), 1);
        ASSERT_EQ(b.rfind("bcd", 2, 3), 1);
        ASSERT_EQ(b.rfind("bc", 2), 1);
        ASSERT_EQ(b.rfind("bcd", 4), 1);
        ASSERT_EQ(b.rfind("abcd"), 0);
        ASSERT_EQ(b.rfind("abcde"), std::string::npos);
    }

    TEST_F(BufferTest, buffer_find_first_of)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.find_first_of('d'), 3);
        ASSERT_EQ(b.find_first_of({"c", 1}), 2);
        ASSERT_EQ(b.find_first_of({"a", 1}), 0);
        ASSERT_EQ(b.find_first_of({"e", 1}), std::string::npos);

        ASSERT_EQ(b.find_first_of("bcd"), 1);
        ASSERT_EQ(b.find_first_of("bcd", 1, 3), 1);
        ASSERT_EQ(b.find_first_of("bcd", 2), 2);
        ASSERT_EQ(b.find_first_of("bcd", 4), std::string::npos);
        ASSERT_EQ(b.find_first_of("abcd"), 0);
        ASSERT_EQ(b.find_first_of("abcde"), 0);
    }

    TEST_F(BufferTest, buffer_find_last_of)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.find_last_of('d'), 3);
        ASSERT_EQ(b.find_last_of({"c", 1}), 2);
        ASSERT_EQ(b.find_last_of({"a", 1}), 0);
        ASSERT_EQ(b.find_last_of({"e", 1}), std::string::npos);

        ASSERT_EQ(b.find_last_of("bcd"), 3);
        ASSERT_EQ(b.find_last_of("bcd", 1, 3), 1);
        ASSERT_EQ(b.find_last_of("bcd", 2), 2);
        ASSERT_EQ(b.find_last_of("bcd", 0), std::string::npos);
        ASSERT_EQ(b.find_last_of("abcd"), 3);
        ASSERT_EQ(b.find_last_of("abcde"), 3);
    }

    TEST_F(BufferTest, buffer_find_first_not_of)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.find_first_not_of('d'), 0);
        ASSERT_EQ(b.find_first_not_of({"c", 1}), 0);
        ASSERT_EQ(b.find_first_not_of({"a", 1}), 1);
        ASSERT_EQ(b.find_first_not_of({"e", 1}), 0);

        ASSERT_EQ(b.find_first_not_of("bcd"), 0);
        ASSERT_EQ(b.find_first_not_of("bcd", 1, 3), std::string::npos);
        ASSERT_EQ(b.find_first_not_of("bcd", 2), std::string::npos);
        ASSERT_EQ(b.find_first_not_of("bcd", 4), std::string::npos);
        ASSERT_EQ(b.find_first_not_of("abcd"), std::string::npos);
        ASSERT_EQ(b.find_first_not_of("e"), 0);
    }

    TEST_F(BufferTest, buffer_find_last_not_of)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b.find_last_not_of('d'), 2);
        ASSERT_EQ(b.find_last_not_of({"c", 1}), 3);
        ASSERT_EQ(b.find_last_not_of({"a", 1}), 3);
        ASSERT_EQ(b.find_last_not_of({"e", 1}), 3);

        ASSERT_EQ(b.find_last_not_of("bcd"), 0);
        ASSERT_EQ(b.find_last_not_of("cd", 1), 1);
        ASSERT_EQ(b.find_last_not_of("abcd"), std::string::npos);
        ASSERT_EQ(b.find_last_not_of("abcde"), std::string::npos);
    }

    TEST_F(BufferTest, buffer_comparison)
    {
        small::buffer b = "abcd";
        ASSERT_EQ(b == "abcd", true);
        ASSERT_EQ(b == "abcde", false);
        ASSERT_EQ(b == m_test, false);

        ASSERT_EQ(b != "abcd", false);
        ASSERT_EQ(b != "abcde", true);
        ASSERT_EQ(b != m_test, true);

        ASSERT_EQ(b < "abcd", false);
        ASSERT_EQ(b < "abcde", true);
        ASSERT_EQ(b <= "abcd", true);
        ASSERT_EQ(b <= "abcde", true);

        ASSERT_EQ(b > "abcd", false);
        ASSERT_EQ(b > "abcde", false);
        ASSERT_EQ(b >= "abcd", true);
        ASSERT_EQ(b >= "abcde", false);
    }

    TEST_F(BufferTest, buffer_hash)
    {
        small::buffer b = m_test;
        auto h = std::hash<std::string_view>{}(b);
        ASSERT_EQ(h, 3766111187626408651);
        ASSERT_EQ(h, std::hash<std::string>{}(m_test));
    }

} // namespace