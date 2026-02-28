#include <gtest/gtest.h>

#include "state/MediaSource.h"

// --- toHexCode tests ---

TEST(MediaSourceTest, ToHexCodeStreaming)
{
    EXPECT_EQ(toHexCode(MediaSource::Streaming), 0x2B);
}

TEST(MediaSourceTest, ToHexCodePhono)
{
    EXPECT_EQ(toHexCode(MediaSource::Phono), 0x22);
}

TEST(MediaSourceTest, ToHexCodeCD)
{
    EXPECT_EQ(toHexCode(MediaSource::CD), 0x23);
}

TEST(MediaSourceTest, ToHexCodeComputer)
{
    EXPECT_EQ(toHexCode(MediaSource::Computer), 0x05);
}

TEST(MediaSourceTest, ToHexCodeBluetooth)
{
    EXPECT_EQ(toHexCode(MediaSource::Bluetooth), 0x2E);
}

TEST(MediaSourceTest, ToHexCodeLibrary)
{
    // Library shares CD input on the receiver
    EXPECT_EQ(toHexCode(MediaSource::Library), 0x23);
}

TEST(MediaSourceTest, ToHexCodeNone)
{
    EXPECT_EQ(toHexCode(MediaSource::None), 0x00);
}

// --- fromHexCode tests ---

TEST(MediaSourceTest, FromHexCodeStreaming)
{
    EXPECT_EQ(fromHexCode(0x2B), MediaSource::Streaming);
}

TEST(MediaSourceTest, FromHexCodePhono)
{
    EXPECT_EQ(fromHexCode(0x22), MediaSource::Phono);
}

TEST(MediaSourceTest, FromHexCodeCD)
{
    // 0x23 is ambiguous (CD and Library) — defaults to CD
    EXPECT_EQ(fromHexCode(0x23), MediaSource::CD);
}

TEST(MediaSourceTest, FromHexCodeComputer)
{
    EXPECT_EQ(fromHexCode(0x05), MediaSource::Computer);
}

TEST(MediaSourceTest, FromHexCodeBluetooth)
{
    EXPECT_EQ(fromHexCode(0x2E), MediaSource::Bluetooth);
}

TEST(MediaSourceTest, FromHexCodeUnknown)
{
    EXPECT_EQ(fromHexCode(0xFF), MediaSource::None);
}

TEST(MediaSourceTest, FromHexCodeZero)
{
    EXPECT_EQ(fromHexCode(0x00), MediaSource::None);
}

// --- Roundtrip tests ---

TEST(MediaSourceTest, RoundtripStreaming)
{
    EXPECT_EQ(fromHexCode(toHexCode(MediaSource::Streaming)), MediaSource::Streaming);
}

TEST(MediaSourceTest, RoundtripPhono)
{
    EXPECT_EQ(fromHexCode(toHexCode(MediaSource::Phono)), MediaSource::Phono);
}

TEST(MediaSourceTest, RoundtripCD)
{
    EXPECT_EQ(fromHexCode(toHexCode(MediaSource::CD)), MediaSource::CD);
}

TEST(MediaSourceTest, RoundtripComputer)
{
    EXPECT_EQ(fromHexCode(toHexCode(MediaSource::Computer)), MediaSource::Computer);
}

TEST(MediaSourceTest, RoundtripBluetooth)
{
    EXPECT_EQ(fromHexCode(toHexCode(MediaSource::Bluetooth)), MediaSource::Bluetooth);
}

// Note: Library roundtrip returns CD (both map to 0x23) — this is by design.
TEST(MediaSourceTest, RoundtripLibraryReturnsCd)
{
    EXPECT_EQ(fromHexCode(toHexCode(MediaSource::Library)), MediaSource::CD);
}
