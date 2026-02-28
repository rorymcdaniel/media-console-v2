#include <gtest/gtest.h>

#ifdef HAS_SNDFILE

#include "library/FlacAudioStream.h"

TEST(FlacAudioStreamTest, ConstructionStoresPath)
{
    FlacAudioStream stream("/tmp/test.flac");
    EXPECT_EQ(stream.filePath(), "/tmp/test.flac");
}

TEST(FlacAudioStreamTest, SampleRateIs44100)
{
    FlacAudioStream stream("/tmp/test.flac");
    EXPECT_EQ(stream.sampleRate(), 44100u);
}

TEST(FlacAudioStreamTest, ChannelsIs2)
{
    FlacAudioStream stream("/tmp/test.flac");
    EXPECT_EQ(stream.channels(), 2u);
}

TEST(FlacAudioStreamTest, BitDepthIs16)
{
    FlacAudioStream stream("/tmp/test.flac");
    EXPECT_EQ(stream.bitDepth(), 16u);
}

TEST(FlacAudioStreamTest, PositionStartsAtZero)
{
    FlacAudioStream stream("/tmp/test.flac");
    EXPECT_EQ(stream.positionFrames(), 0u);
}

TEST(FlacAudioStreamTest, OpenFailsForMissingFile)
{
    FlacAudioStream stream("/tmp/nonexistent_file_12345.flac");
    EXPECT_FALSE(stream.open());
}

TEST(FlacAudioStreamTest, ReadFramesWithoutOpenReturnsError)
{
    FlacAudioStream stream("/tmp/test.flac");
    int16_t buffer[1024] = {};
    EXPECT_EQ(stream.readFrames(buffer, 512), -1);
}

TEST(FlacAudioStreamTest, TotalFramesZeroBeforeOpen)
{
    FlacAudioStream stream("/tmp/test.flac");
    EXPECT_EQ(stream.totalFrames(), 0u);
}

#endif // HAS_SNDFILE
