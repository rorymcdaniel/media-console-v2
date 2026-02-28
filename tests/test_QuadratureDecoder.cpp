#include <gtest/gtest.h>

#include "platform/QuadratureDecoder.h"

class QuadratureDecoderTest : public ::testing::Test
{
protected:
    QuadratureDecoder decoder;
};

// CW sequence: 00 -> 01 -> 11 -> 10 -> 00 (each step = +1)
TEST_F(QuadratureDecoderTest, CWFullCycle)
{
    decoder.reset(0, 0); // state 00

    EXPECT_EQ(decoder.update(0, 1), +1); // 00 -> 01
    EXPECT_EQ(decoder.update(1, 1), +1); // 01 -> 11
    EXPECT_EQ(decoder.update(1, 0), +1); // 11 -> 10
    EXPECT_EQ(decoder.update(0, 0), +1); // 10 -> 00
}

// CCW sequence: 00 -> 10 -> 11 -> 01 -> 00 (each step = -1)
TEST_F(QuadratureDecoderTest, CCWFullCycle)
{
    decoder.reset(0, 0); // state 00

    EXPECT_EQ(decoder.update(1, 0), -1); // 00 -> 10
    EXPECT_EQ(decoder.update(1, 1), -1); // 10 -> 11
    EXPECT_EQ(decoder.update(0, 1), -1); // 11 -> 01
    EXPECT_EQ(decoder.update(0, 0), -1); // 01 -> 00
}

// Same state = 0 (no movement)
TEST_F(QuadratureDecoderTest, SameStateReturnsZero)
{
    decoder.reset(0, 0);

    EXPECT_EQ(decoder.update(0, 0), 0); // 00 -> 00 (no change)
}

// Invalid jump: 00 -> 11 returns 0 (noise rejection)
TEST_F(QuadratureDecoderTest, InvalidJumpReturnsZero)
{
    decoder.reset(0, 0);

    EXPECT_EQ(decoder.update(1, 1), 0); // 00 -> 11 (skipped state)
}

// Reset to specific state works
TEST_F(QuadratureDecoderTest, ResetSetsState)
{
    decoder.reset(1, 0); // state 10 (binary: 2)

    // From 10, going to 00 is CW (+1)
    EXPECT_EQ(decoder.update(0, 0), +1); // 10 -> 00
}

// Full CW revolution: 24 sequential transitions for volume encoder
TEST_F(QuadratureDecoderTest, FullCWRevolution24Steps)
{
    decoder.reset(0, 0);

    // CW cycle: 00 -> 01 -> 11 -> 10 -> 00 ... repeated 6 times = 24 transitions
    const int steps[][2] = {
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, // cycle 1
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, // cycle 2
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, // cycle 3
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, // cycle 4
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, // cycle 5
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, // cycle 6
    };

    for (int i = 0; i < 24; ++i)
    {
        EXPECT_EQ(decoder.update(steps[i][0], steps[i][1]), +1) << "Failed at step " << i;
    }
}

// Full CCW revolution: 24 sequential transitions
TEST_F(QuadratureDecoderTest, FullCCWRevolution24Steps)
{
    decoder.reset(0, 0);

    // CCW cycle: 00 -> 10 -> 11 -> 01 -> 00 ... repeated 6 times = 24 transitions
    const int steps[][2] = {
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 }, // cycle 1
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 }, // cycle 2
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 }, // cycle 3
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 }, // cycle 4
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 }, // cycle 5
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 }, // cycle 6
    };

    for (int i = 0; i < 24; ++i)
    {
        EXPECT_EQ(decoder.update(steps[i][0], steps[i][1]), -1) << "Failed at step " << i;
    }
}

// Direction change: CW then CCW
TEST_F(QuadratureDecoderTest, DirectionChange)
{
    decoder.reset(0, 0);

    // CW: 00 -> 01
    EXPECT_EQ(decoder.update(0, 1), +1);

    // CCW from 01: 01 -> 00
    EXPECT_EQ(decoder.update(0, 0), -1);
}

// Multiple invalid jumps in sequence
TEST_F(QuadratureDecoderTest, MultipleInvalidJumps)
{
    decoder.reset(0, 0);

    // 00 -> 11 (invalid, skip one state)
    EXPECT_EQ(decoder.update(1, 1), 0);

    // Now at state 11, 11 -> 00 (invalid, skip one state)
    EXPECT_EQ(decoder.update(0, 0), 0);
}
