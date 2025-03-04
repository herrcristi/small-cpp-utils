#pragma warning(disable : 4464) // relative include path contains '..'
#pragma warning(disable : 4577) // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed

#include <gtest/gtest.h>

#include "../include/buffer.h"
#include "../include/stack_string.h"

namespace {
    using namespace std::literals;

    class StackStringTest : public testing::Test
    {
    protected:
        StackStringTest() = default;

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
    // stack_string
    //
    TEST_F(StackStringTest, stack_string)
    {
        small::stack_string s = m_test;
        ASSERT_EQ(s, m_test);

        small::buffer       b0 = m_test;
        small::stack_string s0 = b0.c_view();
        s0                     = b0;
        ASSERT_EQ(s0, m_test);

        small::stack_string s1{'a'};
        ASSERT_EQ(s1, "a");

        small::stack_string s2{"abc", 3};
        ASSERT_EQ(s2, "abc");

        small::stack_string s3{"abc", 2};
        ASSERT_EQ(s3, "ab");

        constexpr std::string_view sv = "abc";
        small::stack_string        s4{sv};
        ASSERT_EQ(s4, "abc");

        small::stack_string s5{sv};
        ASSERT_EQ(s5, "abc");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = m_test;
        ASSERT_EQ(s_1, m_test);

        small::stack_string<1> s0_1 = b0.c_view();
        s0_1                        = b0;
        ASSERT_EQ(s0_1, m_test);

        small::stack_string<1> s1_1{'a'};
        ASSERT_EQ(s1_1, "a");

        small::stack_string<1> s2_1{"abc", 3};
        ASSERT_EQ(s2_1, "abc");

        small::stack_string<1> s3_1{"abc", 2};
        ASSERT_EQ(s3_1, "ab");

        small::stack_string<1> s4_1{sv};
        ASSERT_EQ(s4_1, "abc");

        small::stack_string<1> s5_1{sv};
        ASSERT_EQ(s5_1, "abc");
    }

    TEST_F(StackStringTest, stack_string_operator_eq)
    {
        small::stack_string s;
        s = m_test;
        ASSERT_EQ(s, m_test);

        small::stack_string s1;
        s1 = 'a';
        ASSERT_EQ(s1, "a");

        small::stack_string s2;
        s2 = "abc"sv;
        ASSERT_EQ(s2, "abc");

        s2 = s1;
        ASSERT_EQ(s2, "a");

        small::stack_string s3;
        s3 = std::string_view{"abc", 2}; // reduced size
        ASSERT_EQ(s3, "ab");

        constexpr std::string_view sv = "abc";
        small::stack_string        s4;
        s4 = {sv};
        ASSERT_EQ(s4, "abc");

        small::stack_string s5;
        s5 = sv;
        ASSERT_EQ(s5, "abc");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1 = m_test;
        ASSERT_EQ(s_1, m_test);

        small::stack_string<1> s1_1;
        s1_1 = 'a';
        ASSERT_EQ(s1_1, "a");

        small::stack_string<1> s2_1;
        s2_1 = "abc"sv;
        ASSERT_EQ(s2_1, "abc");

        s2_1 = s1_1;
        ASSERT_EQ(s2_1, "a");

        s2_1 = s1; // conversion from different sizes
        ASSERT_EQ(s2_1, "a");

        small::stack_string<1> s3_1;
        s3_1 = std::string_view{"abc", 2}; // reduced size
        ASSERT_EQ(s3_1, "ab");

        small::stack_string<1> s4_1;
        s4_1 = {sv};
        ASSERT_EQ(s4_1, "abc");

        small::stack_string<1> s5_1;
        s5_1 = sv;
        ASSERT_EQ(s5_1, "abc");
    }

    TEST_F(StackStringTest, stack_string_clear)
    {
        small::stack_string s;
        s = m_test;
        ASSERT_EQ(s, m_test);

        s.clear();
        ASSERT_EQ(s, "");

        small::stack_string s1;
        ASSERT_EQ(s1, "");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1 = m_test;
        ASSERT_EQ(s_1, m_test);

        s_1.clear();
        ASSERT_EQ(s_1, "");

        small::stack_string<1> s1_1;
        ASSERT_EQ(s1_1, "");
    }

    TEST_F(StackStringTest, stack_string_plus)
    {
        small::stack_string s;
        s = m_test;
        ASSERT_EQ(s, m_test);

        s += 'a';
        ASSERT_EQ(s, m_test + "a");

        s += "b";
        ASSERT_EQ(s, m_test + "ab");

        s += {"c", 1};
        ASSERT_EQ(s, m_test + "abc");

        constexpr std::string_view sv  = "d"sv;
        s                             += {sv};
        ASSERT_EQ(s, m_test + "abcd");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1 = m_test;
        ASSERT_EQ(s_1, m_test);

        s_1 += 'a';
        ASSERT_EQ(s_1, m_test + "a");

        s_1 += "b";
        ASSERT_EQ(s_1, m_test + "ab");

        s_1 += {"c", 1};
        ASSERT_EQ(s_1, m_test + "abc");

        s_1 += {sv};
        ASSERT_EQ(s_1, m_test + "abcd");
    }

    TEST_F(StackStringTest, stack_string_swap)
    {
        small::stack_string s;
        s = m_test;
        ASSERT_EQ(s, m_test);

        small::stack_string s1 = "a"sv;
        ASSERT_EQ(s1, "a");

        s.swap(s1);
        ASSERT_EQ(s, "a");
        ASSERT_EQ(s1, m_test);

        small::stack_string s2; // default empty
        s.swap(s2);
        ASSERT_EQ(s, "");
        ASSERT_EQ(s2, "a");

        s2.clear();
        ASSERT_EQ(s2, "");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1 = m_test;
        ASSERT_EQ(s_1, m_test);

        small::stack_string<1> s1_1 = "a"sv;
        ASSERT_EQ(s1_1, "a");

        s_1.swap(s1_1);
        ASSERT_EQ(s_1, "a");
        ASSERT_EQ(s1_1, m_test);

        small::stack_string<1> s2_1; // default empty
        s_1.swap(s2_1);
        ASSERT_EQ(s_1, "");
        ASSERT_EQ(s2_1, "a");

        s2_1.clear();
        ASSERT_EQ(s2_1, "");
    }

    TEST_F(StackStringTest, stack_string_size)
    {
        small::stack_string s{m_test};
        ASSERT_EQ(s, m_test);
        ASSERT_EQ(s.size(), m_test.size());
        ASSERT_EQ(s.length(), m_test.size());
        ASSERT_EQ(s.empty(), false);

        s.clear();
        ASSERT_EQ(s.size(), 0);
        ASSERT_EQ(s.empty(), true);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1{m_test};
        ASSERT_EQ(s_1, m_test);
        ASSERT_EQ(s_1.size(), m_test.size());
        ASSERT_EQ(s_1.length(), m_test.size());
        ASSERT_EQ(s_1.empty(), false);

        s_1.clear();
        ASSERT_EQ(s_1.size(), 0);
        ASSERT_EQ(s_1.empty(), true);
    }

    TEST_F(StackStringTest, stack_string_resize)
    {
        small::stack_string s(m_test);
        ASSERT_EQ(s, m_test);

        s += "abcd";
        ASSERT_EQ(s, m_test + "abcd");

        s.resize(20);
        ASSERT_EQ(s, std::string_view("some textabcd\0\0\0\0\0\0\0", 20));

        s.resize(m_test.size() + 1);
        ASSERT_EQ(s, m_test + "a");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1(m_test);
        ASSERT_EQ(s_1, m_test);

        s_1 += "abcd";
        ASSERT_EQ(s_1, m_test + "abcd");

        s_1.resize(20);
        ASSERT_EQ(s_1, std::string_view("some textabcd\0\0\0\0\0\0\0", 20));

        s_1.resize(m_test.size() + 1);
        ASSERT_EQ(s_1, m_test + "a");
    }

    TEST_F(StackStringTest, stack_string_data)
    {
        small::stack_string s(m_test);
        ASSERT_EQ(s.data(), m_test);

        ASSERT_EQ(s.begin(), m_test);
        ASSERT_EQ(s.end(), std::string());

        ASSERT_EQ(s.rend(), m_test);
        ASSERT_EQ(s.rbegin(), std::string());

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1(m_test);
        ASSERT_EQ(s_1.data(), m_test);

        ASSERT_EQ(s_1.begin(), m_test);
        ASSERT_EQ(s_1.end(), std::string());

        ASSERT_EQ(s_1.rend(), m_test);
        ASSERT_EQ(s_1.rbegin(), std::string());
    }

    TEST_F(StackStringTest, stack_string_views)
    {
        small::stack_string s(m_test);
        ASSERT_EQ(s.c_string(), m_test);
        ASSERT_EQ(s.c_view(), m_test);

        auto v = s.c_vector();
        ASSERT_EQ(v.size(), m_test.size());
        ASSERT_EQ(std::string(v.data(), v.size()), m_test);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1(m_test);
        ASSERT_EQ(s_1.c_string(), m_test);
        ASSERT_EQ(s_1.c_view(), m_test);

        auto v_1 = s_1.c_vector();
        ASSERT_EQ(v_1.size(), m_test.size());
        ASSERT_EQ(std::string(v_1.data(), v_1.size()), m_test);
    }

    TEST_F(StackStringTest, stack_string_assign)
    {
        small::stack_string s;
        s.assign(m_test);
        ASSERT_EQ(s, m_test);

        s.assign('a');
        ASSERT_EQ(s, "a");

        small::stack_string s2;
        s2.assign("abc");
        ASSERT_EQ(s2, "abc");

        s2 = s;
        ASSERT_EQ(s2, "a");
        ASSERT_EQ(s, "a");
        s2 += 'b';
        s  += 'c';
        ASSERT_EQ(s2, "ab");
        ASSERT_EQ(s, "ac");

        s.assign({"abc", 2});
        ASSERT_EQ(s, "ab");

        constexpr std::string_view sv = "abc";
        s.assign(sv);
        ASSERT_EQ(s, "abc");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1.assign(m_test);
        ASSERT_EQ(s_1, m_test);

        s_1.assign('a');
        ASSERT_EQ(s_1, "a");

        small::stack_string<1> s2_1;
        s2_1.assign("abc");
        ASSERT_EQ(s2_1, "abc");

        s2_1 = s_1;
        ASSERT_EQ(s2_1, "a");
        ASSERT_EQ(s_1, "a");
        s2_1 += 'b';
        s_1  += 'c';
        ASSERT_EQ(s2_1, "ab");
        ASSERT_EQ(s_1, "ac");

        s_1.assign({"abc", 2});
        ASSERT_EQ(s_1, "ab");

        s_1.assign(sv);
        ASSERT_EQ(s_1, "abc");
    }

    TEST_F(StackStringTest, stack_string_append)
    {
        small::stack_string s;
        s.append(m_test);
        ASSERT_EQ(s, m_test);

        s.append('a');
        ASSERT_EQ(s, m_test + "a");

        s.append("b");
        ASSERT_EQ(s, m_test + "ab");

        s.append({"c", 1});
        ASSERT_EQ(s, m_test + "abc");

        s.append("de", 1);
        ASSERT_EQ(s, m_test + "abcd");

        constexpr std::string_view sv = "f";
        s.append(sv);
        ASSERT_EQ(s, m_test + "abcdf");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1.append(m_test);
        ASSERT_EQ(s_1, m_test);

        s_1.append('a');
        ASSERT_EQ(s_1, m_test + "a");

        s_1.append("b");
        ASSERT_EQ(s_1, m_test + "ab");

        s_1.append({"c", 1});
        ASSERT_EQ(s_1, m_test + "abc");

        s_1.append("de", 1);
        ASSERT_EQ(s_1, m_test + "abcd");

        s_1.append(sv);
        ASSERT_EQ(s_1, m_test + "abcdf");
    }

    TEST_F(StackStringTest, stack_string_insert)
    {
        small::stack_string s;
        s.insert(0, m_test);
        ASSERT_EQ(s, m_test);

        s.insert(0, 'a');
        ASSERT_EQ(s, "a" + m_test);

        s.insert(0, "b");
        ASSERT_EQ(s, "ba" + m_test);

        s.insert(1, {"c", 1});
        ASSERT_EQ(s, "bca" + m_test);

        s.insert(1, "de", 1);
        ASSERT_EQ(s, "bdca" + m_test);

        constexpr std::string_view sv = "f";
        s.insert(0, sv);
        ASSERT_EQ(s, "fbdca" + m_test);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1.insert(0, m_test);
        ASSERT_EQ(s_1, m_test);

        s_1.insert(0, 'a');
        ASSERT_EQ(s_1, "a" + m_test);

        s_1.insert(0, "b");
        ASSERT_EQ(s_1, "ba" + m_test);

        s_1.insert(1, {"c", 1});
        ASSERT_EQ(s_1, "bca" + m_test);

        s_1.insert(1, "de", 1);
        ASSERT_EQ(s_1, "bdca" + m_test);

        s_1.insert(0, sv);
        ASSERT_EQ(s_1, "fbdca" + m_test);
    }

    TEST_F(StackStringTest, stack_string_set)
    {
        small::stack_string s;
        s.set(0, m_test);
        ASSERT_EQ(s, m_test);

        s.set(0, 'a');
        ASSERT_EQ(s, "a");

        s.set(1, "b");
        ASSERT_EQ(s, "ab");

        s.set(1, {"c", 1});
        ASSERT_EQ(s, "ac");

        s.set(2, "de", 1);
        ASSERT_EQ(s, "acd");

        constexpr std::string_view sv = "f";
        s.set(0, sv);
        ASSERT_EQ(s, "f");

        s.set(2, "g");
        ASSERT_EQ(s, std::string_view("fg", 2)); // does not add zeros to be f\0g

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1.set(0, m_test);
        ASSERT_EQ(s_1, m_test);

        s_1.set(0, 'a');
        ASSERT_EQ(s_1, "a");

        s_1.set(1, "b");
        ASSERT_EQ(s_1, "ab");

        s_1.set(1, {"c", 1});
        ASSERT_EQ(s_1, "ac");

        s_1.set(2, "de", 1);
        ASSERT_EQ(s_1, "acd");

        s_1.set(0, sv);
        ASSERT_EQ(s_1, "f");

        s_1.set(2, "g");
        ASSERT_EQ(s_1, std::string_view("fg", 2)); // does not add zeros to be f\0g
    }

    TEST_F(StackStringTest, stack_string_overwrite_same_as_set)
    {
        small::stack_string s;
        s.overwrite(0, m_test);
        ASSERT_EQ(s, m_test);

        s.overwrite(0, 'a');
        ASSERT_EQ(s, "a");

        s.overwrite(1, "b");
        ASSERT_EQ(s, "ab");

        s.overwrite(1, {"c", 1});
        ASSERT_EQ(s, "ac");

        s.overwrite(2, "de", 1);
        ASSERT_EQ(s, "acd");

        constexpr std::string_view sv = "f";
        s.overwrite(0, sv);
        ASSERT_EQ(s, "f");

        s.overwrite(2, "g");
        ASSERT_EQ(s, std::string_view("fg", 2)); // does not add zeros to be f\0g

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1;
        s_1.overwrite(0, m_test);
        ASSERT_EQ(s_1, m_test);

        s_1.overwrite(0, 'a');
        ASSERT_EQ(s_1, "a");

        s_1.overwrite(1, "b");
        ASSERT_EQ(s_1, "ab");

        s_1.overwrite(1, {"c", 1});
        ASSERT_EQ(s_1, "ac");

        s_1.overwrite(2, "de", 1);
        ASSERT_EQ(s_1, "acd");

        s_1.overwrite(0, sv);
        ASSERT_EQ(s_1, "f");

        s_1.overwrite(2, "g");
        ASSERT_EQ(s_1, std::string_view("fg", 2)); // does not add zeros to be f\0g
    }

    TEST_F(StackStringTest, stack_string_erase)
    {
        small::stack_string s = {"abcd", 4};
        ASSERT_EQ(s, "abcd");

        s.erase(2);
        ASSERT_EQ(s, "ab");

        s.erase(0);
        ASSERT_EQ(s, "");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = {"abcd", 4};
        ASSERT_EQ(s_1, "abcd");

        s_1.erase(2);
        ASSERT_EQ(s_1, "ab");

        s_1.erase(0);
        ASSERT_EQ(s_1, "");
    }

    TEST_F(StackStringTest, stack_string_erase_with_length)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s, "abcd");

        s.erase(2, 1);
        ASSERT_EQ(s, "abd");

        s.erase(0, 2);
        ASSERT_EQ(s, "d");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1, "abcd");

        s_1.erase(2, 1);
        ASSERT_EQ(s_1, "abd");

        s_1.erase(0, 2);
        ASSERT_EQ(s_1, "d");
    }

    TEST_F(StackStringTest, stack_string_is_eq)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s, "abcd");
        ASSERT_EQ(s.is_equal("abcd", 4), true);
        ASSERT_EQ(s.compare("abcd", 4), 0);
        ASSERT_EQ(s.compare("abc", 3), 1);
        ASSERT_EQ(s.compare("abd", 3), -1);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1, "abcd");
        ASSERT_EQ(s_1.is_equal("abcd", 4), true);
        ASSERT_EQ(s_1.compare("abcd", 4), 0);
        ASSERT_EQ(s_1.compare("abc", 3), 1);
        ASSERT_EQ(s_1.compare("abd", 3), -1);
    }

    TEST_F(StackStringTest, stack_string_at)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s, "abcd");
        ASSERT_EQ(s[0], 'a');
        ASSERT_EQ(s[1], 'b');

        ASSERT_EQ(s.at(2), 'c');
        ASSERT_EQ(s.at(3), 'd');

        ASSERT_EQ(s.front(), 'a');
        ASSERT_EQ(s.back(), 'd');

        s.erase(0);
        ASSERT_EQ(s, "");
        ASSERT_EQ(s.back(), '\0');

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1, "abcd");
        ASSERT_EQ(s_1[0], 'a');
        ASSERT_EQ(s_1[1], 'b');

        ASSERT_EQ(s_1.at(2), 'c');
        ASSERT_EQ(s_1.at(3), 'd');

        ASSERT_EQ(s_1.front(), 'a');
        ASSERT_EQ(s_1.back(), 'd');

        s_1.erase(0);
        ASSERT_EQ(s_1, "");
        ASSERT_EQ(s_1.back(), '\0');
    }

    TEST_F(StackStringTest, stack_string_push_pop)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s, "abcd");

        s.push_back('e');
        ASSERT_EQ(s, "abcde");

        s.pop_back();
        ASSERT_EQ(s, "abcd");

        s.pop_back();
        ASSERT_EQ(s, "abc");

        s.pop_back();
        ASSERT_EQ(s, "ab");

        s.pop_back();
        ASSERT_EQ(s, "a");

        s.pop_back();
        ASSERT_EQ(s, "");

        s.pop_back();
        ASSERT_EQ(s, "");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1, "abcd");

        s_1.push_back('e');
        ASSERT_EQ(s_1, "abcde");

        s_1.pop_back();
        ASSERT_EQ(s_1, "abcd");

        s_1.pop_back();
        ASSERT_EQ(s_1, "abc");

        s_1.pop_back();
        ASSERT_EQ(s_1, "ab");

        s_1.pop_back();
        ASSERT_EQ(s_1, "a");

        s_1.pop_back();
        ASSERT_EQ(s_1, "");

        s_1.pop_back();
        ASSERT_EQ(s_1, "");
    }

    TEST_F(StackStringTest, stack_string_substr)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.substr(1), "bcd");
        ASSERT_EQ(s.substr(1, 2), "bc");
        ASSERT_EQ(s.substr(0, 5), "abcd");

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.substr(1), "bcd");
        ASSERT_EQ(s_1.substr(1, 2), "bc");
        ASSERT_EQ(s_1.substr(0, 5), "abcd");
    }

    TEST_F(StackStringTest, stack_string_starts_with)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.starts_with('a'), true);
        ASSERT_EQ(s.starts_with({"b", 1}), false);
        ASSERT_EQ(s.starts_with("abc"), true);
        ASSERT_EQ(s.starts_with("abcd"), true);
        ASSERT_EQ(s.starts_with("abcde"), false);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.starts_with('a'), true);
        ASSERT_EQ(s_1.starts_with({"b", 1}), false);
        ASSERT_EQ(s_1.starts_with("abc"), true);
        ASSERT_EQ(s_1.starts_with("abcd"), true);
        ASSERT_EQ(s_1.starts_with("abcde"), false);
    }

    TEST_F(StackStringTest, stack_string_ends_with)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.ends_with('d'), true);
        ASSERT_EQ(s.ends_with({"a", 1}), false);
        ASSERT_EQ(s.ends_with("bcd"), true);
        ASSERT_EQ(s.ends_with("abcd"), true);
        ASSERT_EQ(s.ends_with("abcde"), false);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.ends_with('d'), true);
        ASSERT_EQ(s_1.ends_with({"a", 1}), false);
        ASSERT_EQ(s_1.ends_with("bcd"), true);
        ASSERT_EQ(s_1.ends_with("abcd"), true);
        ASSERT_EQ(s_1.ends_with("abcde"), false);
    }

    TEST_F(StackStringTest, stack_string_contains)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.contains('d'), true);
        ASSERT_EQ(s.contains({"a", 1}), true);
        ASSERT_EQ(s.contains({"e", 1}), false);
        ASSERT_EQ(s.contains("bcd"), true);
        ASSERT_EQ(s.contains("abcd"), true);
        ASSERT_EQ(s.contains("abcde"), false);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.contains('d'), true);
        ASSERT_EQ(s_1.contains({"a", 1}), true);
        ASSERT_EQ(s_1.contains({"e", 1}), false);
        ASSERT_EQ(s_1.contains("bcd"), true);
        ASSERT_EQ(s_1.contains("abcd"), true);
        ASSERT_EQ(s_1.contains("abcde"), false);
    }

    TEST_F(StackStringTest, stack_string_find)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.find('d'), 3);
        ASSERT_EQ(s.find({"a", 1}), 0);
        ASSERT_EQ(s.find({"a", 1}, 2), std::string::npos);
        ASSERT_EQ(s.find({"e", 1}), std::string::npos);

        ASSERT_EQ(s.find("bcd"), 1);
        ASSERT_EQ(s.find("bcd", 1, 3), 1);
        ASSERT_EQ(s.find("bcd", 2), std::string::npos);
        ASSERT_EQ(s.find("abcd"), 0);
        ASSERT_EQ(s.find("abcde"), std::string::npos);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.find('d'), 3);
        ASSERT_EQ(s_1.find({"a", 1}), 0);
        ASSERT_EQ(s_1.find({"a", 1}, 2), std::string::npos);
        ASSERT_EQ(s_1.find({"e", 1}), std::string::npos);

        ASSERT_EQ(s_1.find("bcd"), 1);
        ASSERT_EQ(s_1.find("bcd", 1, 3), 1);
        ASSERT_EQ(s_1.find("bcd", 2), std::string::npos);
        ASSERT_EQ(s_1.find("abcd"), 0);
        ASSERT_EQ(s_1.find("abcde"), std::string::npos);
    }

    TEST_F(StackStringTest, stack_string_rfind)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.rfind('d'), 3);
        ASSERT_EQ(s.rfind({"c", 1}), 2);
        ASSERT_EQ(s.rfind({"a", 1}), 0);
        ASSERT_EQ(s.rfind({"e", 1}), std::string::npos);

        ASSERT_EQ(s.rfind("bcd"), 1);
        ASSERT_EQ(s.rfind("bcd", 1, 3), 1);
        ASSERT_EQ(s.rfind("bcd", 2, 3), 1);
        ASSERT_EQ(s.rfind("bc", 2), 1);
        ASSERT_EQ(s.rfind("bcd", 4), 1);
        ASSERT_EQ(s.rfind("abcd"), 0);
        ASSERT_EQ(s.rfind("abcde"), std::string::npos);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.rfind('d'), 3);
        ASSERT_EQ(s_1.rfind({"c", 1}), 2);
        ASSERT_EQ(s_1.rfind({"a", 1}), 0);
        ASSERT_EQ(s_1.rfind({"e", 1}), std::string::npos);

        ASSERT_EQ(s_1.rfind("bcd"), 1);
        ASSERT_EQ(s_1.rfind("bcd", 1, 3), 1);
        ASSERT_EQ(s_1.rfind("bcd", 2, 3), 1);
        ASSERT_EQ(s_1.rfind("bc", 2), 1);
        ASSERT_EQ(s_1.rfind("bcd", 4), 1);
        ASSERT_EQ(s_1.rfind("abcd"), 0);
        ASSERT_EQ(s_1.rfind("abcde"), std::string::npos);
    }

    TEST_F(StackStringTest, stack_string_find_first_of)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.find_first_of('d'), 3);
        ASSERT_EQ(s.find_first_of({"c", 1}), 2);
        ASSERT_EQ(s.find_first_of({"a", 1}), 0);
        ASSERT_EQ(s.find_first_of({"e", 1}), std::string::npos);

        ASSERT_EQ(s.find_first_of("bcd"), 1);
        ASSERT_EQ(s.find_first_of("bcd", 1, 3), 1);
        ASSERT_EQ(s.find_first_of("bcd", 2), 2);
        ASSERT_EQ(s.find_first_of("bcd", 4), std::string::npos);
        ASSERT_EQ(s.find_first_of("abcd"), 0);
        ASSERT_EQ(s.find_first_of("abcde"), 0);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.find_first_of('d'), 3);
        ASSERT_EQ(s_1.find_first_of({"c", 1}), 2);
        ASSERT_EQ(s_1.find_first_of({"a", 1}), 0);
        ASSERT_EQ(s_1.find_first_of({"e", 1}), std::string::npos);

        ASSERT_EQ(s_1.find_first_of("bcd"), 1);
        ASSERT_EQ(s_1.find_first_of("bcd", 1, 3), 1);
        ASSERT_EQ(s_1.find_first_of("bcd", 2), 2);
        ASSERT_EQ(s_1.find_first_of("bcd", 4), std::string::npos);
        ASSERT_EQ(s_1.find_first_of("abcd"), 0);
        ASSERT_EQ(s_1.find_first_of("abcde"), 0);
    }

    TEST_F(StackStringTest, stack_string_find_last_of)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.find_last_of('d'), 3);
        ASSERT_EQ(s.find_last_of({"c", 1}), 2);
        ASSERT_EQ(s.find_last_of({"a", 1}), 0);
        ASSERT_EQ(s.find_last_of({"e", 1}), std::string::npos);

        ASSERT_EQ(s.find_last_of("bcd"), 3);
        ASSERT_EQ(s.find_last_of("bcd", 1, 3), 1);
        ASSERT_EQ(s.find_last_of("bcd", 2), 2);
        ASSERT_EQ(s.find_last_of("bcd", 0), std::string::npos);
        ASSERT_EQ(s.find_last_of("abcd"), 3);
        ASSERT_EQ(s.find_last_of("abcde"), 3);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.find_last_of('d'), 3);
        ASSERT_EQ(s_1.find_last_of({"c", 1}), 2);
        ASSERT_EQ(s_1.find_last_of({"a", 1}), 0);
        ASSERT_EQ(s_1.find_last_of({"e", 1}), std::string::npos);

        ASSERT_EQ(s_1.find_last_of("bcd"), 3);
        ASSERT_EQ(s_1.find_last_of("bcd", 1, 3), 1);
        ASSERT_EQ(s_1.find_last_of("bcd", 2), 2);
        ASSERT_EQ(s_1.find_last_of("bcd", 0), std::string::npos);
        ASSERT_EQ(s_1.find_last_of("abcd"), 3);
        ASSERT_EQ(s_1.find_last_of("abcde"), 3);
    }

    TEST_F(StackStringTest, stack_string_find_first_not_of)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.find_first_not_of('d'), 0);
        ASSERT_EQ(s.find_first_not_of({"c", 1}), 0);
        ASSERT_EQ(s.find_first_not_of({"a", 1}), 1);
        ASSERT_EQ(s.find_first_not_of({"e", 1}), 0);

        ASSERT_EQ(s.find_first_not_of("bcd"), 0);
        ASSERT_EQ(s.find_first_not_of("bcd", 1, 3), std::string::npos);
        ASSERT_EQ(s.find_first_not_of("bcd", 2), std::string::npos);
        ASSERT_EQ(s.find_first_not_of("bcd", 4), std::string::npos);
        ASSERT_EQ(s.find_first_not_of("abcd"), std::string::npos);
        ASSERT_EQ(s.find_first_not_of("e"), 0);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.find_first_not_of('d'), 0);
        ASSERT_EQ(s_1.find_first_not_of({"c", 1}), 0);
        ASSERT_EQ(s_1.find_first_not_of({"a", 1}), 1);
        ASSERT_EQ(s_1.find_first_not_of({"e", 1}), 0);

        ASSERT_EQ(s_1.find_first_not_of("bcd"), 0);
        ASSERT_EQ(s_1.find_first_not_of("bcd", 1, 3), std::string::npos);
        ASSERT_EQ(s_1.find_first_not_of("bcd", 2), std::string::npos);
        ASSERT_EQ(s_1.find_first_not_of("bcd", 4), std::string::npos);
        ASSERT_EQ(s_1.find_first_not_of("abcd"), std::string::npos);
        ASSERT_EQ(s_1.find_first_not_of("e"), 0);
    }

    TEST_F(StackStringTest, stack_string_find_last_not_of)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s.find_last_not_of('d'), 2);
        ASSERT_EQ(s.find_last_not_of({"c", 1}), 3);
        ASSERT_EQ(s.find_last_not_of({"a", 1}), 3);
        ASSERT_EQ(s.find_last_not_of({"e", 1}), 3);

        ASSERT_EQ(s.find_last_not_of("bcd"), 0);
        ASSERT_EQ(s.find_last_not_of("cd", 1), 1);
        ASSERT_EQ(s.find_last_not_of("abcd"), std::string::npos);
        ASSERT_EQ(s.find_last_not_of("abcde"), std::string::npos);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1.find_last_not_of('d'), 2);
        ASSERT_EQ(s_1.find_last_not_of({"c", 1}), 3);
        ASSERT_EQ(s_1.find_last_not_of({"a", 1}), 3);
        ASSERT_EQ(s_1.find_last_not_of({"e", 1}), 3);

        ASSERT_EQ(s_1.find_last_not_of("bcd"), 0);
        ASSERT_EQ(s_1.find_last_not_of("cd", 1), 1);
        ASSERT_EQ(s_1.find_last_not_of("abcd"), std::string::npos);
        ASSERT_EQ(s_1.find_last_not_of("abcde"), std::string::npos);
    }

    TEST_F(StackStringTest, stack_string_comparison)
    {
        small::stack_string s = std::string_view{"abcd"};
        ASSERT_EQ(s == "abcd", true);
        ASSERT_EQ(s == "abcde", false);
        ASSERT_EQ(s == m_test, false);

        ASSERT_EQ(s != "abcd", false);
        ASSERT_EQ(s != "abcde", true);
        ASSERT_EQ(s != m_test, true);

        ASSERT_EQ(s < "abcd", false);
        ASSERT_EQ(s < "abcde", true);
        ASSERT_EQ(s <= "abcd", true);
        ASSERT_EQ(s <= "abcde", true);

        ASSERT_EQ(s > "abcd", false);
        ASSERT_EQ(s > "abcde", false);
        ASSERT_EQ(s >= "abcd", true);
        ASSERT_EQ(s >= "abcde", false);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = std::string_view{"abcd"};
        ASSERT_EQ(s_1 == "abcd", true);
        ASSERT_EQ(s_1 == "abcde", false);
        ASSERT_EQ(s_1 == m_test, false);

        ASSERT_EQ(s_1 != "abcd", false);
        ASSERT_EQ(s_1 != "abcde", true);
        ASSERT_EQ(s_1 != m_test, true);

        ASSERT_EQ(s_1 < "abcd", false);
        ASSERT_EQ(s_1 < "abcde", true);
        ASSERT_EQ(s_1 <= "abcd", true);
        ASSERT_EQ(s_1 <= "abcde", true);

        ASSERT_EQ(s_1 > "abcd", false);
        ASSERT_EQ(s_1 > "abcde", false);
        ASSERT_EQ(s_1 >= "abcd", true);
        ASSERT_EQ(s_1 >= "abcde", false);
    }

    TEST_F(StackStringTest, stack_string_hash)
    {
        small::stack_string s = m_test;
        auto                h = std::hash<std::string_view>{}(s);
        ASSERT_EQ(h, std::hash<std::string>{}(m_test)); // 3766111187626408651 in linux

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = m_test;
        auto                   h_1 = std::hash<std::string_view>{}(s_1);
        ASSERT_EQ(h_1, std::hash<std::string>{}(m_test)); // 3766111187626408651 in linux
    }

    TEST_F(StackStringTest, stack_string_conversions)
    {
        small::stack_string s = m_test;

        auto utf16text = L"Some text zß水🍌"sv;
        auto utf8text  = "Some text z\xC3\x9F\xE6\xB0\xB4\xF0\x9F\x8D\x8C"sv; // "Some text z\u00df\u6c34\U0001f34c"sv;

        s.set(0, utf16text);
        ASSERT_EQ(s.c_view(), utf8text);

        // same tests as above for stack_string<1> which will use std::string
        small::stack_string<1> s_1 = m_test;
        s_1.set(0, utf16text);
        ASSERT_EQ(s_1.c_view(), utf8text);
    }

} // namespace