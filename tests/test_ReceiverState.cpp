#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "state/ReceiverState.h"

class ReceiverStateTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            static int argc = 1;
            static char appName[] = "test";
            static char* argv[] = { appName, nullptr };
            new QCoreApplication(argc, argv);
        }
    }
};

TEST_F(ReceiverStateTest, DefaultValues)
{
    ReceiverState state;
    EXPECT_EQ(state.volume(), 0);
    EXPECT_FALSE(state.powered());
    EXPECT_FALSE(state.muted());
    EXPECT_EQ(state.currentInput(), MediaSource::None);
    EXPECT_TRUE(state.title().isEmpty());
    EXPECT_TRUE(state.artist().isEmpty());
    EXPECT_TRUE(state.album().isEmpty());
    EXPECT_TRUE(state.albumArtUrl().isEmpty());
    EXPECT_TRUE(state.fileInfo().isEmpty());
    EXPECT_TRUE(state.serviceName().isEmpty());
    EXPECT_EQ(state.streamingService(), StreamingService::Unknown);
}

TEST_F(ReceiverStateTest, VolumeEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::volumeChanged);

    state.setVolume(42);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 42);
    EXPECT_EQ(state.volume(), 42);
}

TEST_F(ReceiverStateTest, VolumeDoesNotEmitWhenUnchanged)
{
    ReceiverState state;
    state.setVolume(42);

    QSignalSpy spy(&state, &ReceiverState::volumeChanged);
    state.setVolume(42);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ReceiverStateTest, PoweredEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::poweredChanged);

    state.setPowered(true);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
    EXPECT_TRUE(state.powered());
}

TEST_F(ReceiverStateTest, PoweredDoesNotEmitWhenUnchanged)
{
    ReceiverState state;
    state.setPowered(true);

    QSignalSpy spy(&state, &ReceiverState::poweredChanged);
    state.setPowered(true);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ReceiverStateTest, MutedEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::mutedChanged);

    state.setMuted(true);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(ReceiverStateTest, CurrentInputEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::currentInputChanged);

    state.setCurrentInput(MediaSource::CD);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(state.currentInput(), MediaSource::CD);
}

TEST_F(ReceiverStateTest, TitleEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::titleChanged);

    state.setTitle("Test Song");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Test Song");
    EXPECT_EQ(state.title(), "Test Song");
}

TEST_F(ReceiverStateTest, TitleDoesNotEmitWhenUnchanged)
{
    ReceiverState state;
    state.setTitle("Test Song");

    QSignalSpy spy(&state, &ReceiverState::titleChanged);
    state.setTitle("Test Song");

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ReceiverStateTest, ArtistEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::artistChanged);

    state.setArtist("Test Artist");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Test Artist");
}

TEST_F(ReceiverStateTest, AlbumEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::albumChanged);

    state.setAlbum("Test Album");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Test Album");
}

TEST_F(ReceiverStateTest, AlbumArtUrlEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::albumArtUrlChanged);

    state.setAlbumArtUrl("http://example.com/art.jpg");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "http://example.com/art.jpg");
}

TEST_F(ReceiverStateTest, FileInfoEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::fileInfoChanged);

    state.setFileInfo("FLAC 44.1kHz/16bit");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "FLAC 44.1kHz/16bit");
}

TEST_F(ReceiverStateTest, ServiceNameEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::serviceNameChanged);

    state.setServiceName("Spotify");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Spotify");
}

TEST_F(ReceiverStateTest, StreamingServiceEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::streamingServiceChanged);

    state.setStreamingService(StreamingService::Spotify);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(state.streamingService(), StreamingService::Spotify);
}
