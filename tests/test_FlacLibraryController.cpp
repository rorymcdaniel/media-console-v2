#include <gtest/gtest.h>

#ifdef HAS_SNDFILE

#include <QCoreApplication>
#include <QTemporaryDir>

#include "app/AppConfig.h"
#include "library/FlacLibraryController.h"
#include "library/LibraryAlbumModel.h"
#include "library/LibraryArtistModel.h"
#include "library/LibraryTrackModel.h"
#include "state/PlaybackState.h"

class FlacLibraryControllerTest : public ::testing::Test
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

        m_playbackState = std::make_unique<PlaybackState>();
    }

    void TearDown() override
    {
        m_playbackState.reset();
        m_tempDir.reset();
    }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<PlaybackState> m_playbackState;
};

TEST_F(FlacLibraryControllerTest, ConstructionCreatesModels)
{
    LibraryConfig config;
    config.rootPath = m_tempDir->path();

    // Pass nullptr for playback controller (no audio in tests)
    FlacLibraryController controller(nullptr, m_playbackState.get(), config);

    EXPECT_NE(controller.artistModel(), nullptr);
    EXPECT_NE(controller.albumModel(), nullptr);
    EXPECT_NE(controller.trackModel(), nullptr);
    EXPECT_NE(controller.database(), nullptr);
}

TEST_F(FlacLibraryControllerTest, IsScanningReturnsFalseBeforeStart)
{
    LibraryConfig config;
    config.rootPath = m_tempDir->path();

    FlacLibraryController controller(nullptr, m_playbackState.get(), config);
    EXPECT_FALSE(controller.isScanning());
}

TEST_F(FlacLibraryControllerTest, StartOpensDatabase)
{
    LibraryConfig config;
    config.rootPath = m_tempDir->path();

    FlacLibraryController controller(nullptr, m_playbackState.get(), config);
    controller.start();

    // Database should be open and queryable
    EXPECT_EQ(controller.database()->trackCount(), 0);

    controller.stop();
}

TEST_F(FlacLibraryControllerTest, PlayTrackWithNullPlaybackControllerDoesNotCrash)
{
    LibraryConfig config;
    config.rootPath = m_tempDir->path();

    FlacLibraryController controller(nullptr, m_playbackState.get(), config);
    controller.start();

    // Should log warning but not crash
    controller.playTrack(0);

    controller.stop();
}

TEST_F(FlacLibraryControllerTest, NextWithEmptyPlaylistDoesNotCrash)
{
    LibraryConfig config;
    config.rootPath = m_tempDir->path();

    FlacLibraryController controller(nullptr, m_playbackState.get(), config);
    controller.next();
    controller.previous();
    // Should not crash
}

TEST_F(FlacLibraryControllerTest, StopClearsState)
{
    LibraryConfig config;
    config.rootPath = m_tempDir->path();

    FlacLibraryController controller(nullptr, m_playbackState.get(), config);
    controller.start();
    controller.stop();

    // Should be safely stopped
    EXPECT_FALSE(controller.isScanning());
}

#endif // HAS_SNDFILE
