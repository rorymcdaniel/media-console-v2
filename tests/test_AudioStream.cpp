#include <algorithm>
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

#include "audio/AudioStream.h"

/// Test implementation of AudioStream that produces silence (zeros).
/// Used for unit testing the AudioStream interface contract and
/// for testing LocalPlaybackController without real audio sources.
class TestAudioStream : public AudioStream
{
public:
    explicit TestAudioStream(size_t totalFrames, unsigned int sampleRate = 44100, unsigned int channels = 2,
                             unsigned int bitDepth = 16)
        : m_totalFrames(totalFrames)
        , m_sampleRate(sampleRate)
        , m_channels(channels)
        , m_bitDepth(bitDepth)
    {
    }

    bool open() override
    {
        m_isOpen = true;
        m_position = 0;
        return true;
    }

    void close() override { m_isOpen = false; }

    long readFrames(int16_t* buffer, size_t frames) override
    {
        if (!m_isOpen)
        {
            return -1;
        }

        size_t remaining = m_totalFrames - m_position;
        size_t toRead = std::min(frames, remaining);

        if (toRead == 0)
        {
            return 0; // End of stream
        }

        // Write silence (zeros)
        std::memset(buffer, 0, toRead * m_channels * sizeof(int16_t));
        m_position += toRead;
        return static_cast<long>(toRead);
    }

    size_t totalFrames() const override { return m_totalFrames; }

    size_t positionFrames() const override { return m_position; }

    bool seek(size_t framePosition) override
    {
        if (framePosition > m_totalFrames)
        {
            return false;
        }
        m_position = framePosition;
        return true;
    }

    unsigned int sampleRate() const override { return m_sampleRate; }

    unsigned int channels() const override { return m_channels; }

    unsigned int bitDepth() const override { return m_bitDepth; }

    bool isOpen() const { return m_isOpen; }

private:
    size_t m_totalFrames;
    unsigned int m_sampleRate;
    unsigned int m_channels;
    unsigned int m_bitDepth;
    size_t m_position = 0;
    bool m_isOpen = false;
};

// --- Open/Close Tests ---

TEST(AudioStreamTest, OpenReturnsTrueAndSetsOpen)
{
    TestAudioStream stream(44100);
    EXPECT_TRUE(stream.open());
    EXPECT_TRUE(stream.isOpen());
}

TEST(AudioStreamTest, CloseSetsClosed)
{
    TestAudioStream stream(44100);
    stream.open();
    stream.close();
    EXPECT_FALSE(stream.isOpen());
}

// --- Format Accessor Tests ---

TEST(AudioStreamTest, SampleRateReturns44100)
{
    TestAudioStream stream(44100);
    EXPECT_EQ(stream.sampleRate(), 44100u);
}

TEST(AudioStreamTest, ChannelsReturns2)
{
    TestAudioStream stream(44100);
    EXPECT_EQ(stream.channels(), 2u);
}

TEST(AudioStreamTest, BitDepthReturns16)
{
    TestAudioStream stream(44100);
    EXPECT_EQ(stream.bitDepth(), 16u);
}

TEST(AudioStreamTest, CustomFormatAccessors)
{
    TestAudioStream stream(1000, 48000, 1, 24);
    EXPECT_EQ(stream.sampleRate(), 48000u);
    EXPECT_EQ(stream.channels(), 1u);
    EXPECT_EQ(stream.bitDepth(), 24u);
}

// --- TotalFrames and Position Tests ---

TEST(AudioStreamTest, TotalFramesReturnsConfiguredValue)
{
    TestAudioStream stream(88200);
    EXPECT_EQ(stream.totalFrames(), 88200u);
}

TEST(AudioStreamTest, PositionFramesStartsAtZeroAfterOpen)
{
    TestAudioStream stream(44100);
    stream.open();
    EXPECT_EQ(stream.positionFrames(), 0u);
}

// --- ReadFrames Tests ---

TEST(AudioStreamTest, ReadFramesReturnsSilence)
{
    TestAudioStream stream(44100);
    stream.open();

    std::vector<int16_t> buffer(1024 * 2); // 1024 frames * 2 channels
    // Set to non-zero to verify memset
    std::fill(buffer.begin(), buffer.end(), 42);

    long read = stream.readFrames(buffer.data(), 1024);
    EXPECT_EQ(read, 1024);

    // Verify all samples are zero (silence)
    for (size_t i = 0; i < 1024 * 2; ++i)
    {
        EXPECT_EQ(buffer[i], 0) << "Non-zero sample at index " << i;
    }
}

TEST(AudioStreamTest, ReadFramesAdvancesPosition)
{
    TestAudioStream stream(44100);
    stream.open();

    std::vector<int16_t> buffer(1024 * 2);
    stream.readFrames(buffer.data(), 1024);
    EXPECT_EQ(stream.positionFrames(), 1024u);

    stream.readFrames(buffer.data(), 512);
    EXPECT_EQ(stream.positionFrames(), 1536u);
}

TEST(AudioStreamTest, ReadFramesReturnsZeroAtEnd)
{
    TestAudioStream stream(100);
    stream.open();

    std::vector<int16_t> buffer(200 * 2);
    long read = stream.readFrames(buffer.data(), 200);
    EXPECT_EQ(read, 100); // Only 100 frames available

    // Now at end of stream
    read = stream.readFrames(buffer.data(), 100);
    EXPECT_EQ(read, 0);
}

TEST(AudioStreamTest, ReadFramesReturnsPartialAtEnd)
{
    TestAudioStream stream(1000);
    stream.open();

    std::vector<int16_t> buffer(2048 * 2);

    // Read 800 frames
    long read = stream.readFrames(buffer.data(), 800);
    EXPECT_EQ(read, 800);

    // Only 200 remain, request 500
    read = stream.readFrames(buffer.data(), 500);
    EXPECT_EQ(read, 200);
}

TEST(AudioStreamTest, ReadFramesReturnsErrorWhenClosed)
{
    TestAudioStream stream(44100);
    // Not opened — should return error
    std::vector<int16_t> buffer(1024 * 2);
    long read = stream.readFrames(buffer.data(), 1024);
    EXPECT_EQ(read, -1);
}

// --- Seek Tests ---

TEST(AudioStreamTest, SeekToValidPosition)
{
    TestAudioStream stream(44100);
    stream.open();

    EXPECT_TRUE(stream.seek(22050));
    EXPECT_EQ(stream.positionFrames(), 22050u);
}

TEST(AudioStreamTest, SeekToBeginning)
{
    TestAudioStream stream(44100);
    stream.open();

    std::vector<int16_t> buffer(1024 * 2);
    stream.readFrames(buffer.data(), 1024);
    EXPECT_EQ(stream.positionFrames(), 1024u);

    EXPECT_TRUE(stream.seek(0));
    EXPECT_EQ(stream.positionFrames(), 0u);
}

TEST(AudioStreamTest, SeekToEnd)
{
    TestAudioStream stream(44100);
    stream.open();

    EXPECT_TRUE(stream.seek(44100));
    EXPECT_EQ(stream.positionFrames(), 44100u);

    // Reading from end returns 0
    std::vector<int16_t> buffer(1024 * 2);
    long read = stream.readFrames(buffer.data(), 1024);
    EXPECT_EQ(read, 0);
}

TEST(AudioStreamTest, SeekBeyondEndFails)
{
    TestAudioStream stream(44100);
    stream.open();

    EXPECT_FALSE(stream.seek(44101));
    // Position unchanged after failed seek
    EXPECT_EQ(stream.positionFrames(), 0u);
}

TEST(AudioStreamTest, SeekThenReadFromNewPosition)
{
    TestAudioStream stream(44100);
    stream.open();

    // Read some frames
    std::vector<int16_t> buffer(1024 * 2);
    stream.readFrames(buffer.data(), 1024);
    EXPECT_EQ(stream.positionFrames(), 1024u);

    // Seek to position 500
    EXPECT_TRUE(stream.seek(500));
    EXPECT_EQ(stream.positionFrames(), 500u);

    // Read from new position
    long read = stream.readFrames(buffer.data(), 100);
    EXPECT_EQ(read, 100);
    EXPECT_EQ(stream.positionFrames(), 600u);
}
