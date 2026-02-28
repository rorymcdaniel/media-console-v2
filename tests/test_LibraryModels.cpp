#include <gtest/gtest.h>

#ifdef HAS_SNDFILE

#include <QCoreApplication>
#include <QTemporaryDir>

#include "library/LibraryAlbumArtProvider.h"
#include "library/LibraryAlbumModel.h"
#include "library/LibraryArtistModel.h"
#include "library/LibraryDatabase.h"
#include "library/LibraryTrackModel.h"

class LibraryModelsTest : public ::testing::Test
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
        m_dbPath = m_tempDir->path() + "/test_models.db";
        m_artCacheDir = m_tempDir->path() + "/art-cache";

        // Open database and populate with test data
        m_database = std::make_unique<LibraryDatabase>();
        ASSERT_TRUE(m_database->open(m_dbPath));
        populateTestData();
    }

    void TearDown() override
    {
        m_database.reset();
        m_tempDir.reset();
    }

    void populateTestData()
    {
        // Artist 1: Zeppelin with 2 albums
        m_database->upsertTrack(
            makeTrack("/music/zep/houses/01.flac", "Led Zeppelin", "Houses of the Holy", 1, 1, 1973));
        m_database->upsertTrack(
            makeTrack("/music/zep/houses/02.flac", "Led Zeppelin", "Houses of the Holy", 2, 1, 1973));
        m_database->upsertTrack(makeTrack("/music/zep/iv/01.flac", "Led Zeppelin", "Led Zeppelin IV", 1, 1, 1971));
        m_database->upsertTrack(makeTrack("/music/zep/iv/02.flac", "Led Zeppelin", "Led Zeppelin IV", 2, 1, 1971));

        // Artist 2: Beatles with 1 album (multi-disc)
        m_database->upsertTrack(
            makeTrack("/music/beatles/white/d1-01.flac", "The Beatles", "The White Album", 1, 1, 1968));
        m_database->upsertTrack(
            makeTrack("/music/beatles/white/d1-02.flac", "The Beatles", "The White Album", 2, 1, 1968));
        m_database->upsertTrack(
            makeTrack("/music/beatles/white/d2-01.flac", "The Beatles", "The White Album", 1, 2, 1968));
    }

    LibraryTrack makeTrack(const QString& path, const QString& artist, const QString& album, int trackNum, int discNum,
                           int year)
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
    std::unique_ptr<LibraryDatabase> m_database;
    QString m_dbPath;
    QString m_artCacheDir;
};

// ---- LibraryArtistModel Tests ----

TEST_F(LibraryModelsTest, ArtistModelRowCountMatchesArtists)
{
    LibraryArtistModel model(m_database.get());
    EXPECT_EQ(model.rowCount(), 0); // Not yet refreshed

    model.refresh();
    EXPECT_EQ(model.rowCount(), 2); // Led Zeppelin, The Beatles
}

TEST_F(LibraryModelsTest, ArtistModelDataReturnsCorrectValues)
{
    LibraryArtistModel model(m_database.get());
    model.refresh();

    // Artists sorted alphabetically
    QModelIndex idx0 = model.index(0);
    EXPECT_EQ(model.data(idx0, LibraryArtistModel::AlbumArtistRole).toString(), "Led Zeppelin");
    EXPECT_EQ(model.data(idx0, LibraryArtistModel::AlbumCountRole).toInt(), 2);

    QModelIndex idx1 = model.index(1);
    EXPECT_EQ(model.data(idx1, LibraryArtistModel::AlbumArtistRole).toString(), "The Beatles");
    EXPECT_EQ(model.data(idx1, LibraryArtistModel::AlbumCountRole).toInt(), 1);
}

TEST_F(LibraryModelsTest, ArtistModelRoleNames)
{
    LibraryArtistModel model(m_database.get());
    auto roles = model.roleNames();
    EXPECT_TRUE(roles.contains(LibraryArtistModel::AlbumArtistRole));
    EXPECT_TRUE(roles.contains(LibraryArtistModel::AlbumCountRole));
    EXPECT_EQ(roles[LibraryArtistModel::AlbumArtistRole], "albumArtist");
    EXPECT_EQ(roles[LibraryArtistModel::AlbumCountRole], "albumCount");
}

// ---- LibraryAlbumModel Tests ----

TEST_F(LibraryModelsTest, AlbumModelEmptyWithoutFilter)
{
    LibraryAlbumArtProvider artProvider(m_artCacheDir);
    LibraryAlbumModel model(m_database.get(), &artProvider);
    model.refresh();
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(LibraryModelsTest, AlbumModelFilteredByArtist)
{
    LibraryAlbumArtProvider artProvider(m_artCacheDir);
    LibraryAlbumModel model(m_database.get(), &artProvider);
    model.setArtistFilter("Led Zeppelin");

    ASSERT_EQ(model.rowCount(), 2);

    // Sorted by year: Led Zeppelin IV (1971) before Houses of the Holy (1973)
    QModelIndex idx0 = model.index(0);
    EXPECT_EQ(model.data(idx0, LibraryAlbumModel::AlbumRole).toString(), "Led Zeppelin IV");
    EXPECT_EQ(model.data(idx0, LibraryAlbumModel::YearRole).toInt(), 1971);
    EXPECT_EQ(model.data(idx0, LibraryAlbumModel::TrackCountRole).toInt(), 2);

    QModelIndex idx1 = model.index(1);
    EXPECT_EQ(model.data(idx1, LibraryAlbumModel::AlbumRole).toString(), "Houses of the Holy");
    EXPECT_EQ(model.data(idx1, LibraryAlbumModel::YearRole).toInt(), 1973);
}

TEST_F(LibraryModelsTest, AlbumModelRoleNames)
{
    LibraryAlbumArtProvider artProvider(m_artCacheDir);
    LibraryAlbumModel model(m_database.get(), &artProvider);
    auto roles = model.roleNames();
    EXPECT_EQ(roles[LibraryAlbumModel::AlbumRole], "album");
    EXPECT_EQ(roles[LibraryAlbumModel::YearRole], "year");
    EXPECT_EQ(roles[LibraryAlbumModel::TrackCountRole], "trackCount");
    EXPECT_EQ(roles[LibraryAlbumModel::ArtPathRole], "artPath");
}

// ---- LibraryTrackModel Tests ----

TEST_F(LibraryModelsTest, TrackModelEmptyWithoutFilters)
{
    LibraryTrackModel model(m_database.get());
    model.refresh();
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(LibraryModelsTest, TrackModelFilteredByArtistAndAlbum)
{
    LibraryTrackModel model(m_database.get());
    model.setArtistFilter("Led Zeppelin");
    model.setAlbumFilter("Houses of the Holy");

    ASSERT_EQ(model.rowCount(), 2);

    QModelIndex idx0 = model.index(0);
    EXPECT_EQ(model.data(idx0, LibraryTrackModel::TrackNumberRole).toInt(), 1);
    EXPECT_EQ(model.data(idx0, LibraryTrackModel::TitleRole).toString(), "Track 1");

    QModelIndex idx1 = model.index(1);
    EXPECT_EQ(model.data(idx1, LibraryTrackModel::TrackNumberRole).toInt(), 2);
}

TEST_F(LibraryModelsTest, TrackModelMultiDiscSortedByDiscThenTrack)
{
    LibraryTrackModel model(m_database.get());
    model.setArtistFilter("The Beatles");
    model.setAlbumFilter("The White Album");

    ASSERT_EQ(model.rowCount(), 3);

    // Disc 1 track 1, disc 1 track 2, disc 2 track 1
    QModelIndex idx0 = model.index(0);
    EXPECT_EQ(model.data(idx0, LibraryTrackModel::DiscNumberRole).toInt(), 1);
    EXPECT_EQ(model.data(idx0, LibraryTrackModel::TrackNumberRole).toInt(), 1);

    QModelIndex idx1 = model.index(1);
    EXPECT_EQ(model.data(idx1, LibraryTrackModel::DiscNumberRole).toInt(), 1);
    EXPECT_EQ(model.data(idx1, LibraryTrackModel::TrackNumberRole).toInt(), 2);

    QModelIndex idx2 = model.index(2);
    EXPECT_EQ(model.data(idx2, LibraryTrackModel::DiscNumberRole).toInt(), 2);
    EXPECT_EQ(model.data(idx2, LibraryTrackModel::TrackNumberRole).toInt(), 1);
}

TEST_F(LibraryModelsTest, TrackModelTrackAtReturnsCorrectData)
{
    LibraryTrackModel model(m_database.get());
    model.setArtistFilter("Led Zeppelin");
    model.setAlbumFilter("Led Zeppelin IV");

    auto track = model.trackAt(0);
    EXPECT_EQ(track.trackNumber, 1);
    EXPECT_EQ(track.album, "Led Zeppelin IV");
    EXPECT_FALSE(track.filePath.isEmpty());
}

TEST_F(LibraryModelsTest, TrackModelTrackAtOutOfBoundsReturnsDefault)
{
    LibraryTrackModel model(m_database.get());
    model.setArtistFilter("Led Zeppelin");
    model.setAlbumFilter("Led Zeppelin IV");

    auto track = model.trackAt(999);
    EXPECT_TRUE(track.filePath.isEmpty());
}

TEST_F(LibraryModelsTest, TrackModelAllTracksReturnsFullList)
{
    LibraryTrackModel model(m_database.get());
    model.setArtistFilter("The Beatles");
    model.setAlbumFilter("The White Album");

    auto tracks = model.allTracks();
    EXPECT_EQ(tracks.size(), 3);
}

TEST_F(LibraryModelsTest, TrackModelRoleNames)
{
    LibraryTrackModel model(m_database.get());
    auto roles = model.roleNames();
    EXPECT_EQ(roles[LibraryTrackModel::TrackNumberRole], "trackNumber");
    EXPECT_EQ(roles[LibraryTrackModel::DiscNumberRole], "discNumber");
    EXPECT_EQ(roles[LibraryTrackModel::TitleRole], "title");
    EXPECT_EQ(roles[LibraryTrackModel::ArtistRole], "artist");
    EXPECT_EQ(roles[LibraryTrackModel::DurationSecondsRole], "durationSeconds");
    EXPECT_EQ(roles[LibraryTrackModel::FilePathRole], "filePath");
}

#endif // HAS_SNDFILE
