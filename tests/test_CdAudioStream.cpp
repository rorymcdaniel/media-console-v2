#include <gtest/gtest.h>

#include "cd/CdAudioStream.h"

/// Tests for CdAudioStream math/state logic.
/// Note: Actual paranoia reads require a CD drive and cannot be tested on macOS.
/// These tests focus on frame/sector conversion, totalFrames, positionFrames, and format accessors.

class CdAudioStreamTest : public ::testing::Test
{
protected:
    // A track spanning sectors 0-100 (100 sectors)
    static constexpr int kStartSector = 0;
    static constexpr int kEndSector = 100;
};

TEST_F(CdAudioStreamTest, TotalFramesCalculation)
{
    CdAudioStream stream("/dev/null", kStartSector, kEndSector);
    // 100 sectors * 588 frames/sector = 58800 frames
    EXPECT_EQ(stream.totalFrames(), 58800u);
}

TEST_F(CdAudioStreamTest, TotalFramesWithOffsetStart)
{
    // Track starts at sector 150 (typical first track offset), ends at 1000
    CdAudioStream stream("/dev/null", 150, 1000);
    // 850 sectors * 588 = 499800 frames
    EXPECT_EQ(stream.totalFrames(), 499800u);
}

TEST_F(CdAudioStreamTest, TotalFramesSingleSector)
{
    CdAudioStream stream("/dev/null", 500, 501);
    EXPECT_EQ(stream.totalFrames(), 588u);
}

TEST_F(CdAudioStreamTest, TotalFramesZeroSectors)
{
    CdAudioStream stream("/dev/null", 100, 100);
    EXPECT_EQ(stream.totalFrames(), 0u);
}

TEST_F(CdAudioStreamTest, PositionFramesAtConstruction)
{
    CdAudioStream stream("/dev/null", kStartSector, kEndSector);
    EXPECT_EQ(stream.positionFrames(), 0u);
}

TEST_F(CdAudioStreamTest, SampleRateIs44100)
{
    CdAudioStream stream("/dev/null", kStartSector, kEndSector);
    EXPECT_EQ(stream.sampleRate(), 44100u);
}

TEST_F(CdAudioStreamTest, ChannelsIs2)
{
    CdAudioStream stream("/dev/null", kStartSector, kEndSector);
    EXPECT_EQ(stream.channels(), 2u);
}

TEST_F(CdAudioStreamTest, BitDepthIs16)
{
    CdAudioStream stream("/dev/null", kStartSector, kEndSector);
    EXPECT_EQ(stream.bitDepth(), 16u);
}

TEST_F(CdAudioStreamTest, FramesPerSectorConstant)
{
    EXPECT_EQ(CdAudioStream::kFramesPerSector, 588);
}

TEST_F(CdAudioStreamTest, BytesPerFrameConstant)
{
    // 16-bit stereo = 2 bytes * 2 channels = 4 bytes per frame
    EXPECT_EQ(CdAudioStream::kBytesPerFrame, 4);
}

TEST_F(CdAudioStreamTest, SamplesPerSectorConstant)
{
    // 588 frames * 2 channels = 1176 samples per sector
    EXPECT_EQ(CdAudioStream::kSamplesPerSector, 1176);
}

TEST_F(CdAudioStreamTest, FramesToSectorsExactMultiple)
{
    CdAudioStream stream("/dev/null", 0, 100);
    // 588 * 10 = 5880 frames = exactly 10 sectors
    EXPECT_EQ(stream.framesToSectors(5880), 10);
}

TEST_F(CdAudioStreamTest, FramesToSectorsPartialSector)
{
    CdAudioStream stream("/dev/null", 0, 100);
    // 589 frames = 1 sector (integer division)
    EXPECT_EQ(stream.framesToSectors(589), 1);
}

TEST_F(CdAudioStreamTest, FramesToSectorsZero)
{
    CdAudioStream stream("/dev/null", 0, 100);
    EXPECT_EQ(stream.framesToSectors(0), 0);
}

TEST_F(CdAudioStreamTest, FramesToSectorsLessThanOneSector)
{
    CdAudioStream stream("/dev/null", 0, 100);
    EXPECT_EQ(stream.framesToSectors(100), 0);
}

TEST_F(CdAudioStreamTest, OpenWithoutHasCdioReturnsFalse)
{
    CdAudioStream stream("/dev/null", 0, 100);
    // On macOS (no HAS_CDIO), open() returns false
#ifndef HAS_CDIO
    EXPECT_FALSE(stream.open());
#else
    // On Linux with HAS_CDIO, /dev/null is not a CD drive so it should also fail
    EXPECT_FALSE(stream.open());
#endif
}

TEST_F(CdAudioStreamTest, ReadFramesWithoutOpenReturnsError)
{
    CdAudioStream stream("/dev/null", 0, 100);
    int16_t buffer[1024] = {};
    long result = stream.readFrames(buffer, 512);
    EXPECT_EQ(result, -1);
}

TEST_F(CdAudioStreamTest, SeekWithoutOpenReturnsFalse)
{
    CdAudioStream stream("/dev/null", 0, 100);
    EXPECT_FALSE(stream.seek(0));
}
