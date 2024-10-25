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
    };

    //
    // buffer
    //
    TEST_F(BufferTest, buffer)
    {
        // auto comp = small::stricmp("aB", "Ab");
        // ASSERT_EQ(comp, 0);
    }

} // namespace