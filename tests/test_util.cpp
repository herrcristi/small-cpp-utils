#include "test_common.h"

#include "../include/util.h"

namespace {
    using namespace std::literals;

    class UtilTest : public testing::Test
    {
    protected:
        UtilTest() = default;

        void SetUp() override
        {
            // setup before test
        }
        void TearDown() override
        {
            // clean after test
        }
    };

    //
    // stricmp
    //
    TEST_F(UtilTest, stricmp)
    {
        auto comp = small::stricmp("aB", "Ab");
        ASSERT_EQ(comp, 0);

        comp = small::stricmp("a", "b");
        ASSERT_EQ(comp, -1);

        comp = small::stricmp("a", "B");
        ASSERT_EQ(comp, -1);

        comp = small::stricmp("A", "b");
        ASSERT_EQ(comp, -1);

        comp = small::stricmp("B", "a");
        ASSERT_EQ(comp, 1);
    }

    //
    // strnicmp
    //
    TEST_F(UtilTest, strnicmp)
    {
        auto comp = small::strnicmp("aB", "Ab", 1);
        ASSERT_EQ(comp, 0);
        comp = small::strnicmp("aB", "AbC", 2);
        ASSERT_EQ(comp, 0);
        comp = small::strnicmp("aB", "AbC", 3);
        ASSERT_EQ(comp, -1);

        comp = small::strnicmp("a", "ab", 1);
        ASSERT_EQ(comp, 0);
        comp = small::strnicmp("a", "ab", 2);
        ASSERT_EQ(comp, -1);

        comp = small::strnicmp("a", "b", 3);
        ASSERT_EQ(comp, -1);

        comp = small::strnicmp("a", "B", 2);
        ASSERT_EQ(comp, -1);

        comp = small::strnicmp("A", "b", 1);
        ASSERT_EQ(comp, -1);

        comp = small::strnicmp("B", "a", 2);
        ASSERT_EQ(comp, 1);
    }

    //
    // util function icasecmp
    //
    TEST_F(UtilTest, icasecmp)
    {
        auto comp = small::icasecmp{}("aB", "Ab");
        ASSERT_EQ(comp, false);
        comp = small::icasecmp{}("a", "Ab");
        ASSERT_EQ(comp, true);
        comp = small::icasecmp{}("Ab", "a");
        ASSERT_EQ(comp, false);
    }

    //
    // icasecmp used in a map
    //
    TEST_F(UtilTest, icasecmp_map)
    {
        std::map<std::string, int, small::icasecmp> m;
        ASSERT_EQ(m.size(), 0);

        m["B"] = 2;
        ASSERT_EQ(m.size(), 1);

        m["a"] = 1;
        ASSERT_EQ(m.size(), 2);

        m["A"] = 1;
        ASSERT_EQ(m.size(), 2); // still 2

        auto it = m.find("a");
        ASSERT_NE(it, m.end());
        ASSERT_EQ(it->first, "a");

        it = m.find("A");
        ASSERT_NE(it, m.end());
        ASSERT_EQ(it->first, "a");
    }

    //
    // conversion to_lower_case, to_upper_case
    //
    TEST_F(UtilTest, conversion_strings)
    {
        std::string s = "Some Text";
        small::to_lower_case(s);
        ASSERT_EQ(s, "some text");

        small::to_upper_case(s);
        ASSERT_EQ(s, "SOME TEXT");

        small::to_capitalize_case(s);
        ASSERT_EQ(s, "Some text");

        ASSERT_EQ(small::to_hex(false), "0");
        ASSERT_EQ(small::to_hex(true), "1");

        ASSERT_EQ(small::to_hex(-1), "ffffffff");
        ASSERT_EQ(small::to_hex(5), "5");
        ASSERT_EQ(small::to_hex(15), "f");
        ASSERT_EQ(small::to_hex(25), "19");
        ASSERT_EQ(small::to_hex(45), "2d");

        ASSERT_EQ(small::to_hex(-1LL), "ffffffffffffffff");
        ASSERT_EQ(small::to_hex(45LL), "2d");
        ASSERT_EQ(small::to_hex(45ULL), "2d");

        // with fill
        ASSERT_EQ(small::to_hex(false, {.fill = true}), "00");
        ASSERT_EQ(small::to_hex(true, {.fill = true}), "01");
        ASSERT_EQ(small::to_hex(-1, {.fill = true}), "ffffffff");
        ASSERT_EQ(small::to_hex(5, {.fill = true}), "00000005");
        ASSERT_EQ(small::to_hex(45, {.fill = true}), "0000002d");
        ASSERT_EQ(small::to_hex(5LL, {.fill = true}), "0000000000000005");
        ASSERT_EQ(small::to_hex(-1LL, {.fill = true}), "ffffffffffffffff");
        ASSERT_EQ(small::to_hex(45ULL, {.fill = true}), "000000000000002d");

        // fill short version
        ASSERT_EQ(small::to_hex_fill(5), "00000005");
        ASSERT_EQ(small::to_hex_fill(45ULL), "000000000000002d");

        // conversions
        auto utf16text = L"Some text zß水🍌"sv;
        auto utf8text  = "Some text z\xC3\x9F\xE6\xB0\xB4\xF0\x9F\x8D\x8C"sv; // "Some text z\u00df\u6c34\U0001f34c"sv;

        // to utf16
        ASSERT_EQ(small::to_utf16("", 0), L"\0");
        ASSERT_EQ(small::to_utf16(nullptr, 0), L"\0");
        ASSERT_EQ(small::to_utf16(utf8text), utf16text.data());
        ASSERT_EQ(small::to_utf16(utf8text.data(), utf8text.size() - 11), L"Some text");

        // to utf8
        ASSERT_EQ(small::to_utf8(L"", 0), "\0");
        ASSERT_EQ(small::to_utf8(nullptr, 0), "\0");
        ASSERT_EQ(small::to_utf8(utf16text), utf8text.data());
        ASSERT_EQ(small::to_utf8(utf16text.data(), 9), "Some text");
    }

    //
    // time
    //
    TEST_F(UtilTest, time)
    {
        auto timeStart = small::time_now();

        small::sleep(100 /*ms*/);

        auto timeElapsedMs    = small::time_diff_ms(timeStart);
        auto timeElapsedMicro = small::time_diff_micro(timeStart);
        auto timeElapsedNano  = small::time_diff_nano(timeStart);

        ASSERT_GE(timeElapsedMs, 100 - 1);             // due conversion
        ASSERT_GE(timeElapsedMicro, (100 - 1) * 1000); // due conversion
        ASSERT_GE(timeElapsedNano, (100 - 1) * 1000 * 1000);
    }

    TEST_F(UtilTest, time_micro)
    {
        auto timeStart = small::time_now();

        small::sleep_micro(100 /*ms*/ * 1000);

        auto timeElapsedMs    = small::time_diff_ms(timeStart);
        auto timeElapsedMicro = small::time_diff_micro(timeStart);
        auto timeElapsedNano  = small::time_diff_nano(timeStart);

        ASSERT_GE(timeElapsedMs, 100 - 1);             // due conversion
        ASSERT_GE(timeElapsedMicro, (100 - 1) * 1000); // due conversion
        ASSERT_GE(timeElapsedNano, (100 - 1) * 1000 * 1000);
    }

    TEST_F(UtilTest, time_to_iso_string_and_UnixTimestamp)
    {
        std::time_t t    = 0;
        auto        from = std::chrono::system_clock::from_time_t(t);
        ASSERT_EQ(small::to_iso_string(from), "1970-01-01T00:00:00.000Z");
        ASSERT_EQ(small::to_unix_timestamp(from), 0);

        t    = 1733172168; // time_t is in seconds
        from = std::chrono::system_clock::from_time_t(t);
        ASSERT_EQ(small::to_iso_string(from), "2024-12-02T20:42:48.000Z");
        ASSERT_EQ(small::to_unix_timestamp(from), 1733172168000ULL);
    }

    TEST_F(UtilTest, high_time)
    {
        auto timeStart = small::high_time_now();

        small::sleep(100 /*ms*/);

        auto timeElapsedMs    = small::high_time_diff_ms(timeStart);
        auto timeElapsedMicro = small::high_time_diff_micro(timeStart);
        auto timeElapsedNano  = small::high_time_diff_nano(timeStart);

        ASSERT_GE(timeElapsedMs, 100 - 1);             // due conversion
        ASSERT_GE(timeElapsedMicro, (100 - 1) * 1000); // due conversion
        ASSERT_GE(timeElapsedNano, (100 - 1) * 1000 * 1000);
    }

    TEST_F(UtilTest, high_time_micro)
    {
        auto timeStart = small::high_time_now();

        small::sleep_micro(100 /*ms*/ * 1000);

        auto timeElapsedMs    = small::high_time_diff_ms(timeStart);
        auto timeElapsedMicro = small::high_time_diff_micro(timeStart);
        auto timeElapsedNano  = small::high_time_diff_nano(timeStart);

        ASSERT_GE(timeElapsedMs, 100 - 1);             // due conversion
        ASSERT_GE(timeElapsedMicro, (100 - 1) * 1000); // due conversion
        ASSERT_GE(timeElapsedNano, (100 - 1) * 1000 * 1000);
    }

    //
    // rand
    //
    TEST_F(UtilTest, rand)
    {
        // limits
        auto gen32 = std::mt19937(0 /*seed*/);
        ASSERT_EQ(gen32.min(), 0);
        ASSERT_EQ(gen32.max(), 4294967295U);

        auto gen64 = std::mt19937_64(0 /*seed*/);
        ASSERT_EQ(gen64.min(), 0);
        ASSERT_EQ(gen64.max(), 18446744073709551615UL);

        // seed 0 always produces the same number
        auto rdefault32 = gen32();
        ASSERT_EQ(rdefault32, 2357136044);

        auto rdefault64 = gen64();
        ASSERT_EQ(rdefault64, 2947667278772165694UL);

        // generate 2 random numbers
        // at least one of them should be equal to default
        auto r1 = small::rand64();
        auto r2 = small::rand64();

        if (r1 == rdefault64) {
            ASSERT_NE(r2, rdefault64);
        } else {
            ASSERT_NE(r1, rdefault64);
        }
    }

    //
    // uuid
    //
    TEST_F(UtilTest, uuid_help_string_function)
    {
        std::string u = small::to_hex_fill(0ULL);
        ASSERT_EQ(u, "0000000000000000");
        u += small::to_hex_fill((unsigned long long)UINT32_MAX);
        ASSERT_EQ(u, "000000000000000000000000ffffffff");

        small::uuid_add_hyphen(u);
        ASSERT_EQ(u, "00000000-0000-0000-0000-0000ffffffff");

        small::uuid_add_braces(u);
        ASSERT_EQ(u, "{00000000-0000-0000-0000-0000ffffffff}");

        std::string h1;
        small::uuid_add_hyphen(h1);
        ASSERT_EQ(h1, "");

        std::string h2{"00000000"};
        small::uuid_add_hyphen(h2);
        ASSERT_EQ(h2, "00000000");

        std::string h3{"0000000000000000"};
        small::uuid_add_hyphen(h3);
        ASSERT_EQ(h3, "00000000-0000-0000");

        std::string b1;
        small::uuid_add_braces(b1);
        ASSERT_EQ(b1, "{}");

        std::string u1{"abc0ABC1"};
        small::uuid_to_uppercase(u1);
        ASSERT_EQ(u1, "ABC0ABC1");
    }

    TEST_F(UtilTest, uuid)
    {
        auto [r1, r2] = small::uuidp();
        ASSERT_NE(r1, 0);
        ASSERT_NE(r2, 0);

        auto u = small::uuid();
        ASSERT_EQ(u.size(), 32);
        ASSERT_NE(u, "00000000000000000000000000000000");

        auto u1 = small::uuid({.add_hyphen = true, .add_braces = true});
        ASSERT_EQ(u1.size(), 38);
        ASSERT_NE(u1, "{00000000-0000-0000-0000-0000ffffffff}");

        auto uc = small::uuidc();
        ASSERT_EQ(uc.size(), 32);
        ASSERT_NE(uc, "00000000000000000000000000000000");
    }

} // namespace