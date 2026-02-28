#include <gtest/gtest.h>

#include "platform/stubs/StubCdDrive.h"

TEST(StubCdDrive, ReadTocReturnsEmpty)
{
    StubCdDrive cd;
    auto toc = cd.readToc();
    EXPECT_TRUE(toc.isEmpty());
}

TEST(StubCdDrive, GetDiscIdReturnsEmpty)
{
    StubCdDrive cd;
    EXPECT_TRUE(cd.getDiscId().isEmpty());
}

TEST(StubCdDrive, IsDiscPresentDefaultsFalse)
{
    StubCdDrive cd;
    EXPECT_FALSE(cd.isDiscPresent());
}

TEST(StubCdDrive, EjectReturnsTrue)
{
    StubCdDrive cd;
    EXPECT_TRUE(cd.eject());
}

TEST(StubCdDrive, SetDiscPresentUpdatesState)
{
    StubCdDrive cd;
    EXPECT_FALSE(cd.isDiscPresent());

    cd.setDiscPresent(true);
    EXPECT_TRUE(cd.isDiscPresent());
    EXPECT_TRUE(cd.isAudioDisc());

    cd.setDiscPresent(false);
    EXPECT_FALSE(cd.isDiscPresent());
}
