#include <cstring>
#include <gtest/gtest.h>
#include <numeric>
#include <vector>

#include "audio/AudioBufferStats.h"
#include "audio/AudioRingBuffer.h"

// --- AudioRingBuffer Tests ---

TEST(AudioRingBufferTest, ConstructorSetsCapacity)
{
    AudioRingBuffer buf(1024, 2);
    EXPECT_EQ(buf.capacityFrames(), 1024u);
    EXPECT_EQ(buf.availableFrames(), 0u);
    EXPECT_EQ(buf.freeFrames(), 1024u);
}

TEST(AudioRingBufferTest, WriteToEmptyBuffer)
{
    AudioRingBuffer buf(1024, 2);
    std::vector<int16_t> data(1024 * 2, 42);

    size_t written = buf.write(data.data(), 1024);
    EXPECT_EQ(written, 1024u);
    EXPECT_EQ(buf.availableFrames(), 1024u);
    EXPECT_EQ(buf.freeFrames(), 0u);
}

TEST(AudioRingBufferTest, WriteToFullBufferReturnsZero)
{
    AudioRingBuffer buf(100, 2);
    std::vector<int16_t> data(100 * 2, 42);

    buf.write(data.data(), 100); // Fill it
    size_t written = buf.write(data.data(), 10);
    EXPECT_EQ(written, 0u);
}

TEST(AudioRingBufferTest, ReadFromEmptyBufferReturnsZero)
{
    AudioRingBuffer buf(1024, 2);
    std::vector<int16_t> data(1024 * 2);

    size_t readCount = buf.read(data.data(), 100);
    EXPECT_EQ(readCount, 0u);
}

TEST(AudioRingBufferTest, ReadReturnsWrittenData)
{
    AudioRingBuffer buf(1024, 2);
    std::vector<int16_t> writeData(512 * 2);
    std::iota(writeData.begin(), writeData.end(), 1); // 1, 2, 3, ...

    buf.write(writeData.data(), 512);

    std::vector<int16_t> readData(512 * 2, 0);
    size_t readCount = buf.read(readData.data(), 512);
    EXPECT_EQ(readCount, 512u);
    EXPECT_EQ(writeData, readData);
}

TEST(AudioRingBufferTest, AvailablePlusFreeEqualsCapacity)
{
    AudioRingBuffer buf(1000, 2);
    std::vector<int16_t> data(300 * 2, 42);

    buf.write(data.data(), 300);
    EXPECT_EQ(buf.availableFrames() + buf.freeFrames(), 1000u);

    std::vector<int16_t> out(100 * 2);
    buf.read(out.data(), 100);
    EXPECT_EQ(buf.availableFrames() + buf.freeFrames(), 1000u);
}

TEST(AudioRingBufferTest, ClearResetsState)
{
    AudioRingBuffer buf(1024, 2);
    std::vector<int16_t> data(512 * 2, 42);

    buf.write(data.data(), 512);
    EXPECT_EQ(buf.availableFrames(), 512u);

    buf.clear();
    EXPECT_EQ(buf.availableFrames(), 0u);
    EXPECT_EQ(buf.freeFrames(), 1024u);
}

TEST(AudioRingBufferTest, WraparoundDataIntegrity)
{
    AudioRingBuffer buf(100, 2);

    // Write 80 frames
    std::vector<int16_t> data1(80 * 2);
    std::iota(data1.begin(), data1.end(), 1);
    buf.write(data1.data(), 80);

    // Read 60 frames (frees space, read pointer advances)
    std::vector<int16_t> out1(60 * 2);
    buf.read(out1.data(), 60);

    // Write 70 more frames (wraps around end of buffer)
    std::vector<int16_t> data2(70 * 2);
    std::iota(data2.begin(), data2.end(), 1000);
    size_t written = buf.write(data2.data(), 70);
    EXPECT_EQ(written, 70u); // 80 - 60 = 20 used, 80 free, write 70

    // Read remaining 20 from first write
    std::vector<int16_t> out2(20 * 2);
    buf.read(out2.data(), 20);
    // Verify data from first write (positions 60-79, values 121-160)
    for (size_t i = 0; i < 20 * 2; ++i)
    {
        EXPECT_EQ(out2[i], static_cast<int16_t>(60 * 2 + 1 + i)) << "First write data mismatch at index " << i;
    }

    // Read the 70 wrapped frames
    std::vector<int16_t> out3(70 * 2);
    size_t readCount = buf.read(out3.data(), 70);
    EXPECT_EQ(readCount, 70u);
    for (size_t i = 0; i < 70 * 2; ++i)
    {
        EXPECT_EQ(out3[i], static_cast<int16_t>(1000 + i)) << "Wrapped data mismatch at index " << i;
    }
}

TEST(AudioRingBufferTest, WritePartialWhenFull)
{
    AudioRingBuffer buf(100, 2);
    std::vector<int16_t> data(100 * 2, 42);

    buf.write(data.data(), 80); // 80 used, 20 free
    size_t written = buf.write(data.data(), 50); // Only 20 fit
    EXPECT_EQ(written, 20u);
    EXPECT_EQ(buf.availableFrames(), 100u);
}

TEST(AudioRingBufferTest, ReadPartialWhenNotEnough)
{
    AudioRingBuffer buf(100, 2);
    std::vector<int16_t> data(30 * 2, 42);
    buf.write(data.data(), 30);

    std::vector<int16_t> out(50 * 2);
    size_t readCount = buf.read(out.data(), 50); // Only 30 available
    EXPECT_EQ(readCount, 30u);
}

// --- AudioBufferStats Tests ---

TEST(AudioBufferStatsTest, InitialValuesAreZero)
{
    AudioBufferStats stats;
    EXPECT_EQ(stats.xrunCount, 0u);
    EXPECT_DOUBLE_EQ(stats.readLatencyAvgUs, 0.0);
    EXPECT_DOUBLE_EQ(stats.readLatencyMaxUs, 0.0);
    EXPECT_EQ(stats.errorCount, 0u);
    EXPECT_EQ(stats.totalReads, 0u);
}

TEST(AudioBufferStatsTest, RecordReadLatency)
{
    AudioBufferStats stats;
    stats.recordReadLatency(100.0);
    EXPECT_DOUBLE_EQ(stats.readLatencyAvgUs, 100.0);
    EXPECT_DOUBLE_EQ(stats.readLatencyMaxUs, 100.0);
    EXPECT_EQ(stats.totalReads, 1u);

    stats.recordReadLatency(200.0);
    EXPECT_DOUBLE_EQ(stats.readLatencyAvgUs, 150.0);
    EXPECT_DOUBLE_EQ(stats.readLatencyMaxUs, 200.0);
    EXPECT_EQ(stats.totalReads, 2u);
}

TEST(AudioBufferStatsTest, RecordXrun)
{
    AudioBufferStats stats;
    stats.recordXrun();
    stats.recordXrun();
    EXPECT_EQ(stats.xrunCount, 2u);
}

TEST(AudioBufferStatsTest, RecordError)
{
    AudioBufferStats stats;
    stats.recordError();
    EXPECT_EQ(stats.errorCount, 1u);
}

TEST(AudioBufferStatsTest, ResetZerosAll)
{
    AudioBufferStats stats;
    stats.recordReadLatency(100.0);
    stats.recordXrun();
    stats.recordError();

    stats.reset();

    EXPECT_EQ(stats.xrunCount, 0u);
    EXPECT_DOUBLE_EQ(stats.readLatencyAvgUs, 0.0);
    EXPECT_DOUBLE_EQ(stats.readLatencyMaxUs, 0.0);
    EXPECT_EQ(stats.errorCount, 0u);
    EXPECT_EQ(stats.totalReads, 0u);
    EXPECT_DOUBLE_EQ(stats.readLatencySumUs, 0.0);
}
