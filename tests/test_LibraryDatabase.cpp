#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "library/LibraryDatabase.h"

class LibraryDatabaseTest : public ::testing::Test
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

        m_tempDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(m_tempDir->isValid());
        m_dbPath = m_tempDir->path() + "/test_library.db";
    }

    void TearDown() override { m_tempDir.reset(); }

    LibraryTrack makeTrack(const QString& path, const QString& artist, const QString& album, int trackNum = 1,
                           int discNum = 1, int year = 2020)
    {
        LibraryTrack track;
        track.filePath = path;
        track.title = QString("Track %1").arg(trackNum);
        track.artist = artist;
        track.albumArtist = artist;
        track.album = album;
        track.trackNumber = trackNum;
        track.discNumber = discNum;
        track.year = year;
        track.genre = "Rock";
        track.durationSeconds = 300;
        track.sampleRate = 44100;
        track.bitDepth = 16;
        track.mtime = 1700000000;
        return track;
    }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_dbPath;
};

TEST_F(LibraryDatabaseTest, OpenCreatesDatabase)
{
    LibraryDatabase db;
    EXPECT_TRUE(db.open(m_dbPath));
    EXPECT_TRUE(QFile::exists(m_dbPath));
    db.close();
}

TEST_F(LibraryDatabaseTest, UpsertTrackStoresAndRetrieves)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    auto track = makeTrack("/music/artist/album/01.flac", "Artist", "Album");
    EXPECT_TRUE(db.upsertTrack(track));
    EXPECT_EQ(db.trackCount(), 1);

    db.close();
}

TEST_F(LibraryDatabaseTest, GetTrackMtimeReturnsStoredValue)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    auto track = makeTrack("/music/test.flac", "Artist", "Album");
    track.mtime = 1234567890;
    ASSERT_TRUE(db.upsertTrack(track));

    EXPECT_EQ(db.getTrackMtime("/music/test.flac"), 1234567890);
    db.close();
}

TEST_F(LibraryDatabaseTest, GetTrackMtimeReturnsNegativeForUnknown)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    EXPECT_EQ(db.getTrackMtime("/nonexistent.flac"), -1);
    db.close();
}

TEST_F(LibraryDatabaseTest, GetArtistsSortedAlphabetically)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    db.upsertTrack(makeTrack("/music/z/a/01.flac", "Zeppelin", "Album1"));
    db.upsertTrack(makeTrack("/music/a/a/01.flac", "Aerosmith", "Album1"));
    db.upsertTrack(makeTrack("/music/a/b/01.flac", "Aerosmith", "Album2"));
    db.upsertTrack(makeTrack("/music/m/a/01.flac", "Metallica", "Album1"));

    auto artists = db.getArtists();
    ASSERT_EQ(artists.size(), 3);
    EXPECT_EQ(artists[0].albumArtist, "Aerosmith");
    EXPECT_EQ(artists[0].albumCount, 2);
    EXPECT_EQ(artists[1].albumArtist, "Metallica");
    EXPECT_EQ(artists[1].albumCount, 1);
    EXPECT_EQ(artists[2].albumArtist, "Zeppelin");
    EXPECT_EQ(artists[2].albumCount, 1);

    db.close();
}

TEST_F(LibraryDatabaseTest, GetAlbumsByArtistSortedByYear)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    db.upsertTrack(makeTrack("/music/a/newer/01.flac", "Artist", "Newer Album", 1, 1, 2023));
    db.upsertTrack(makeTrack("/music/a/older/01.flac", "Artist", "Older Album", 1, 1, 2018));
    db.upsertTrack(makeTrack("/music/a/older/02.flac", "Artist", "Older Album", 2, 1, 2018));

    auto albums = db.getAlbumsByArtist("Artist");
    ASSERT_EQ(albums.size(), 2);
    EXPECT_EQ(albums[0].album, "Older Album");
    EXPECT_EQ(albums[0].year, 2018);
    EXPECT_EQ(albums[0].trackCount, 2);
    EXPECT_EQ(albums[1].album, "Newer Album");
    EXPECT_EQ(albums[1].year, 2023);
    EXPECT_EQ(albums[1].trackCount, 1);

    db.close();
}

TEST_F(LibraryDatabaseTest, GetTracksByAlbumSortedByDiscThenTrack)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    // Multi-disc album: disc 1 has tracks 1-3, disc 2 has tracks 1-2
    db.upsertTrack(makeTrack("/music/a/01-03.flac", "Artist", "Album", 3, 1, 2020));
    db.upsertTrack(makeTrack("/music/a/01-01.flac", "Artist", "Album", 1, 1, 2020));
    db.upsertTrack(makeTrack("/music/a/02-01.flac", "Artist", "Album", 1, 2, 2020));
    db.upsertTrack(makeTrack("/music/a/01-02.flac", "Artist", "Album", 2, 1, 2020));
    db.upsertTrack(makeTrack("/music/a/02-02.flac", "Artist", "Album", 2, 2, 2020));

    auto tracks = db.getTracksByAlbum("Artist", "Album");
    ASSERT_EQ(tracks.size(), 5);

    // Should be sorted: disc 1 track 1, disc 1 track 2, disc 1 track 3, disc 2 track 1, disc 2 track 2
    EXPECT_EQ(tracks[0].discNumber, 1);
    EXPECT_EQ(tracks[0].trackNumber, 1);
    EXPECT_EQ(tracks[1].discNumber, 1);
    EXPECT_EQ(tracks[1].trackNumber, 2);
    EXPECT_EQ(tracks[2].discNumber, 1);
    EXPECT_EQ(tracks[2].trackNumber, 3);
    EXPECT_EQ(tracks[3].discNumber, 2);
    EXPECT_EQ(tracks[3].trackNumber, 1);
    EXPECT_EQ(tracks[4].discNumber, 2);
    EXPECT_EQ(tracks[4].trackNumber, 2);

    db.close();
}

TEST_F(LibraryDatabaseTest, RemoveStaleEntries)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    db.upsertTrack(makeTrack("/music/keep.flac", "Artist", "Album"));
    db.upsertTrack(makeTrack("/music/remove.flac", "Artist", "Album", 2));
    EXPECT_EQ(db.trackCount(), 2);

    QSet<QString> validPaths = { "/music/keep.flac" };
    int removed = db.removeStaleEntries(validPaths);
    EXPECT_EQ(removed, 1);
    EXPECT_EQ(db.trackCount(), 1);
    EXPECT_EQ(db.getTrackMtime("/music/keep.flac"), 1700000000);
    EXPECT_EQ(db.getTrackMtime("/music/remove.flac"), -1);

    db.close();
}

TEST_F(LibraryDatabaseTest, TrackCountReturnsCorrectValue)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    EXPECT_EQ(db.trackCount(), 0);
    db.upsertTrack(makeTrack("/music/1.flac", "A", "B"));
    EXPECT_EQ(db.trackCount(), 1);
    db.upsertTrack(makeTrack("/music/2.flac", "A", "B", 2));
    EXPECT_EQ(db.trackCount(), 2);

    db.close();
}

TEST_F(LibraryDatabaseTest, UpsertTrackBatchInTransaction)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    QVector<LibraryTrack> batch;
    for (int i = 1; i <= 10; ++i)
    {
        batch.append(makeTrack(QString("/music/track%1.flac").arg(i), "Artist", "Album", i));
    }

    EXPECT_TRUE(db.upsertTrackBatch(batch));
    EXPECT_EQ(db.trackCount(), 10);

    db.close();
}

TEST_F(LibraryDatabaseTest, GetAllMtimes)
{
    LibraryDatabase db;
    ASSERT_TRUE(db.open(m_dbPath));

    auto track1 = makeTrack("/music/a.flac", "A", "B");
    track1.mtime = 100;
    auto track2 = makeTrack("/music/b.flac", "A", "B", 2);
    track2.mtime = 200;
    db.upsertTrack(track1);
    db.upsertTrack(track2);

    auto mtimes = db.getAllMtimes();
    EXPECT_EQ(mtimes.size(), 2);
    EXPECT_EQ(mtimes["/music/a.flac"], 100);
    EXPECT_EQ(mtimes["/music/b.flac"], 200);

    db.close();
}
