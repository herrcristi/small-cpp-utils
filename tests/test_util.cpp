#include <gtest/gtest.h>

#include "../include/util.h"

namespace {
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
    // time
    //
    TEST_F(UtilTest, time)
    {
        auto timeStart = small::timeNow();

        small::sleep(100 /*ms*/);

        auto timeElapsedMs = small::timeDiffMs(timeStart);
        auto timeElapsedMicro = small::timeDiffMicro(timeStart);
        auto timeElapsedNano = small::timeDiffNano(timeStart);

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
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        ss << std::setw(16) << 0;
        ASSERT_EQ(ss.str(), "0000000000000000");

        ss << std::setw(16) << UINT32_MAX;
        auto u = ss.str();
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
        auto [r1, r2] = small::uuid128();
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