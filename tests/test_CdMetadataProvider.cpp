#include <gtest/gtest.h>

#include "cd/CdMetadataProvider.h"
#include "platform/ICdDrive.h"

/// Tests for CdMetadataProvider parsing and validation logic.
/// Network tests are not included since they require real HTTP endpoints.
/// These tests focus on the deterministic functions: freedb disc ID, DTITLE parsing, validation.

class CdMetadataProviderTest : public ::testing::Test
{
protected:
    QVector<TocEntry> makeTestToc(int numTracks = 10)
    {
        QVector<TocEntry> toc;
        int sector = 150; // Standard 2-second pregap
        for (int i = 0; i < numTracks; ++i)
        {
            TocEntry entry;
            entry.trackNumber = i + 1;
            entry.startSector = sector;
            int trackSectors = 15000 + (i * 1000); // ~200 seconds per track, varying
            entry.endSector = sector + trackSectors - 1;
            entry.durationSeconds = trackSectors / 75;
            toc.append(entry);
            sector += trackSectors;
        }
        return toc;
    }
};

TEST_F(CdMetadataProviderTest, ParseDTitleWithSeparator)
{
    auto [artist, album] = CdMetadataProvider::parseDTitle("Pink Floyd / The Dark Side of the Moon");
    EXPECT_EQ(artist, "Pink Floyd");
    EXPECT_EQ(album, "The Dark Side of the Moon");
}

TEST_F(CdMetadataProviderTest, ParseDTitleWithSlashInAlbum)
{
    // Only first " / " is the separator
    auto [artist, album] = CdMetadataProvider::parseDTitle("AC/DC / Highway to Hell");
    // "AC" is not right -- the CDDB protocol uses " / " (with spaces) as separator
    // Since "AC/DC" has no space around the slash, the first " / " is after "AC/DC"
    EXPECT_EQ(artist, "AC/DC");
    EXPECT_EQ(album, "Highway to Hell");
}

TEST_F(CdMetadataProviderTest, ParseDTitleNoSeparator)
{
    auto [artist, album] = CdMetadataProvider::parseDTitle("Various Artists Compilation");
    EXPECT_TRUE(artist.isEmpty());
    EXPECT_EQ(album, "Various Artists Compilation");
}

TEST_F(CdMetadataProviderTest, ParseDTitleEmptyString)
{
    auto [artist, album] = CdMetadataProvider::parseDTitle("");
    EXPECT_TRUE(artist.isEmpty());
    EXPECT_TRUE(album.isEmpty());
}

TEST_F(CdMetadataProviderTest, ParseDTitleWithExtraSpaces)
{
    auto [artist, album] = CdMetadataProvider::parseDTitle("  Led Zeppelin  /  IV  ");
    EXPECT_EQ(artist, "Led Zeppelin");
    EXPECT_EQ(album, "IV");
}

TEST_F(CdMetadataProviderTest, FreedbDiscIdNotZero)
{
    auto toc = makeTestToc(10);
    quint32 id = CdMetadataProvider::computeFreedbDiscId(toc);
    EXPECT_NE(id, 0u);
}

TEST_F(CdMetadataProviderTest, FreedbDiscIdEncodesTrackCount)
{
    auto toc5 = makeTestToc(5);
    auto toc10 = makeTestToc(10);
    quint32 id5 = CdMetadataProvider::computeFreedbDiscId(toc5);
    quint32 id10 = CdMetadataProvider::computeFreedbDiscId(toc10);

    // Low byte should be track count
    EXPECT_EQ(id5 & 0xFF, 5u);
    EXPECT_EQ(id10 & 0xFF, 10u);
}

TEST_F(CdMetadataProviderTest, FreedbDiscIdEmptyToc)
{
    QVector<TocEntry> emptyToc;
    quint32 id = CdMetadataProvider::computeFreedbDiscId(emptyToc);
    EXPECT_EQ(id, 0u);
}

TEST_F(CdMetadataProviderTest, FreedbDiscIdDeterministic)
{
    auto toc = makeTestToc(8);
    quint32 id1 = CdMetadataProvider::computeFreedbDiscId(toc);
    quint32 id2 = CdMetadataProvider::computeFreedbDiscId(toc);
    EXPECT_EQ(id1, id2);
}

TEST_F(CdMetadataProviderTest, FreedbDiscIdDifferentTocsDiffer)
{
    auto toc5 = makeTestToc(5);
    auto toc10 = makeTestToc(10);
    quint32 id5 = CdMetadataProvider::computeFreedbDiscId(toc5);
    quint32 id10 = CdMetadataProvider::computeFreedbDiscId(toc10);
    EXPECT_NE(id5, id10);
}
