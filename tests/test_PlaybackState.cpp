#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "state/PlaybackState.h"

class PlaybackStateTest : public ::testing::Test
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

TEST_F(PlaybackStateTest, DefaultValues)
{
    PlaybackState state;
    EXPECT_EQ(state.playbackMode(), PlaybackMode::Stopped);
    EXPECT_EQ(state.activeSource(), MediaSource::None);
    EXPECT_EQ(state.positionMs(), 0);
    EXPECT_EQ(state.durationMs(), 0);
    EXPECT_TRUE(state.title().isEmpty());
    EXPECT_TRUE(state.artist().isEmpty());
    EXPECT_TRUE(state.album().isEmpty());
    EXPECT_TRUE(state.albumArtUrl().isEmpty());
    EXPECT_EQ(state.trackNumber(), 0);
    EXPECT_EQ(state.trackCount(), 0);
}

TEST_F(PlaybackStateTest, PlaybackModeEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::playbackModeChanged);

    state.setPlaybackMode(PlaybackMode::Playing);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(state.playbackMode(), PlaybackMode::Playing);
}

TEST_F(PlaybackStateTest, PlaybackModeDoesNotEmitWhenUnchanged)
{
    PlaybackState state;
    state.setPlaybackMode(PlaybackMode::Playing);

    QSignalSpy spy(&state, &PlaybackState::playbackModeChanged);
    state.setPlaybackMode(PlaybackMode::Playing);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(PlaybackStateTest, ActiveSourceEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::activeSourceChanged);

    state.setActiveSource(MediaSource::CD);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(state.activeSource(), MediaSource::CD);
}

TEST_F(PlaybackStateTest, PositionMsEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::positionMsChanged);

    state.setPositionMs(5000);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toLongLong(), 5000);
    EXPECT_EQ(state.positionMs(), 5000);
}

TEST_F(PlaybackStateTest, PositionMsDoesNotEmitWhenUnchanged)
{
    PlaybackState state;
    state.setPositionMs(5000);

    QSignalSpy spy(&state, &PlaybackState::positionMsChanged);
    state.setPositionMs(5000);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(PlaybackStateTest, DurationMsEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::durationMsChanged);

    state.setDurationMs(180000);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toLongLong(), 180000);
}

TEST_F(PlaybackStateTest, TitleEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::titleChanged);

    state.setTitle("Track Title");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Track Title");
}

TEST_F(PlaybackStateTest, ArtistEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::artistChanged);

    state.setArtist("Track Artist");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Track Artist");
}

TEST_F(PlaybackStateTest, TrackNumberEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::trackNumberChanged);

    state.setTrackNumber(5);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 5);
}

TEST_F(PlaybackStateTest, TrackCountEmitsChanged)
{
    PlaybackState state;
    QSignalSpy spy(&state, &PlaybackState::trackCountChanged);

    state.setTrackCount(12);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 12);
}
