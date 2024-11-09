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

} // namespace