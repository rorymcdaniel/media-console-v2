#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "cd/CdMetadataCache.h"

class CdMetadataCacheTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            static int argc = 1;
            static char arg0[] = "test";
            static char* argv[] = { arg0 };
            new QCoreApplication(argc, argv);
        }

        m_tempDir.reset(new QTemporaryDir());
        ASSERT_TRUE(m_tempDir->isValid());
        m_dbPath = m_tempDir->path() + "/test_metadata.db";
        m_cache = new CdMetadataCache();
    }

    void TearDown() override
    {
        delete m_cache;
        m_cache = nullptr;
        m_tempDir.reset();
    }

    CdMetadata makeTestMetadata(const QString& discId = "test-disc-001")
    {
        CdMetadata metadata;
        metadata.discId = discId;
        metadata.artist = "Test Artist";
        metadata.album = "Test Album";
        metadata.genre = "Rock";
        metadata.year = 2024;
        metadata.source = "musicbrainz";
        metadata.musicbrainzReleaseId = "mb-release-001";

        CdTrackInfo track1;
        track1.trackNumber = 1;
        track1.title = "Track One";
        track1.artist = "Test Artist";
        track1.durationSeconds = 240;

        CdTrackInfo track2;
        track2.trackNumber = 2;
        track2.title = "Track Two";
        track2.artist = "Test Artist";
        track2.durationSeconds = 180;

        metadata.tracks.append(track1);
        metadata.tracks.append(track2);

        return metadata;
    }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_dbPath;
    CdMetadataCache* m_cache = nullptr;
};

TEST_F(CdMetadataCacheTest, OpenCreatesDatabaseFile)
{
    EXPECT_TRUE(m_cache->open(m_dbPath));
    EXPECT_TRUE(QFile::exists(m_dbPath));
}

TEST_F(CdMetadataCacheTest, LookupForUnknownDiscReturnsEmpty)
{
    ASSERT_TRUE(m_cache->open(m_dbPath));
    auto result = m_cache->lookup("nonexistent-disc");
    EXPECT_FALSE(result.has_value());
}

TEST_F(CdMetadataCacheTest, StoreAndLookupRoundTrip)
{
    ASSERT_TRUE(m_cache->open(m_dbPath));

    auto metadata = makeTestMetadata();
    EXPECT_TRUE(m_cache->store(metadata));

    auto result = m_cache->lookup("test-disc-001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->discId, "test-disc-001");
    EXPECT_EQ(result->artist, "Test Artist");
    EXPECT_EQ(result->album, "Test Album");
    EXPECT_EQ(result->genre, "Rock");
    EXPECT_EQ(result->year, 2024);
    EXPECT_EQ(result->source, "musicbrainz");
    EXPECT_EQ(result->musicbrainzReleaseId, "mb-release-001");
    ASSERT_EQ(result->tracks.size(), 2);
    EXPECT_EQ(result->tracks[0].trackNumber, 1);
    EXPECT_EQ(result->tracks[0].title, "Track One");
    EXPECT_EQ(result->tracks[0].durationSeconds, 240);
    EXPECT_EQ(result->tracks[1].trackNumber, 2);
    EXPECT_EQ(result->tracks[1].title, "Track Two");
    EXPECT_EQ(result->tracks[1].durationSeconds, 180);
}

TEST_F(CdMetadataCacheTest, StoreUpsertReplacesPreviousData)
{
    ASSERT_TRUE(m_cache->open(m_dbPath));

    auto metadata = makeTestMetadata();
    EXPECT_TRUE(m_cache->store(metadata));

    // Update with new data
    metadata.artist = "Updated Artist";
    metadata.album = "Updated Album";
    metadata.tracks[0].title = "Updated Track One";
    EXPECT_TRUE(m_cache->store(metadata));

    auto result = m_cache->lookup("test-disc-001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->artist, "Updated Artist");
    EXPECT_EQ(result->album, "Updated Album");
    EXPECT_EQ(result->tracks[0].title, "Updated Track One");
}

TEST_F(CdMetadataCacheTest, StoreAlbumArtAndGetRoundTrip)
{
    ASSERT_TRUE(m_cache->open(m_dbPath));

    // Store disc metadata first (foreign key)
    auto metadata = makeTestMetadata();
    EXPECT_TRUE(m_cache->store(metadata));

    EXPECT_TRUE(m_cache->storeAlbumArt("test-disc-001", "/cache/front.jpg", "/cache/back.jpg", "coverartarchive"));

    auto result = m_cache->getAlbumArt("test-disc-001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->frontPath, "/cache/front.jpg");
    EXPECT_EQ(result->backPath, "/cache/back.jpg");
    EXPECT_EQ(result->source, "coverartarchive");
}

TEST_F(CdMetadataCacheTest, GetAlbumArtForUnknownDiscReturnsEmpty)
{
    ASSERT_TRUE(m_cache->open(m_dbPath));
    auto result = m_cache->getAlbumArt("nonexistent-disc");
    EXPECT_FALSE(result.has_value());
}

TEST_F(CdMetadataCacheTest, MultipleDiscsIndependent)
{
    ASSERT_TRUE(m_cache->open(m_dbPath));

    auto disc1 = makeTestMetadata("disc-aaa");
    disc1.artist = "Artist A";
    auto disc2 = makeTestMetadata("disc-bbb");
    disc2.artist = "Artist B";

    EXPECT_TRUE(m_cache->store(disc1));
    EXPECT_TRUE(m_cache->store(disc2));

    auto result1 = m_cache->lookup("disc-aaa");
    auto result2 = m_cache->lookup("disc-bbb");

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result1->artist, "Artist A");
    EXPECT_EQ(result2->artist, "Artist B");
}

TEST_F(CdMetadataCacheTest, EmptyTracksStoreSucceeds)
{
    ASSERT_TRUE(m_cache->open(m_dbPath));

    CdMetadata metadata;
    metadata.discId = "empty-disc";
    metadata.artist = "No Tracks";
    metadata.album = "Empty";

    EXPECT_TRUE(m_cache->store(metadata));

    auto result = m_cache->lookup("empty-disc");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->tracks.size(), 0);
}
