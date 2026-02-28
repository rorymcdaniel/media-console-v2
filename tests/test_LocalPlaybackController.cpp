#include <QCoreApplication>
#include <QSignalSpy>
#include <QThread>

#include <algorithm>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>

#include "audio/AudioStream.h"
#include "audio/LocalPlaybackController.h"
#include "platform/stubs/StubAudioOutput.h"
#include "state/PlaybackMode.h"
#include "state/PlaybackState.h"

/// Test AudioStream that produces silence with configurable frame count.
/// When throttled, readFrames sleeps 1ms per call to simulate real-time I/O.
class TestPlaybackStream : public AudioStream
{
public:
    explicit TestPlaybackStream(size_t totalFrames, bool throttle = false)
        : m_totalFrames(totalFrames)
        , m_throttle(throttle)
    {
    }

    bool open() override
    {
        m_isOpen = true;
        m_position = 0;
        return true;
    }

    void close() override { m_isOpen = false; }

    long readFrames(int16_t* buffer, size_t frames) override
    {
        if (!m_isOpen)
        {
            return -1;
        }

        size_t remaining = m_totalFrames - m_position;
        size_t toRead = std::min(frames, remaining);

        if (toRead == 0)
        {
            return 0;
        }

        if (m_throttle)
        {
            QThread::msleep(1);
        }

        std::memset(buffer, 0, toRead * 2 * sizeof(int16_t));
        m_position += toRead;
        return static_cast<long>(toRead);
    }

    size_t totalFrames() const override { return m_totalFrames; }
    size_t positionFrames() const override { return m_position; }

    bool seek(size_t framePosition) override
    {
        if (framePosition > m_totalFrames)
        {
            return false;
        }
        m_position = framePosition;
        return true;
    }

    unsigned int sampleRate() const override { return 44100; }
    unsigned int channels() const override { return 2; }
    unsigned int bitDepth() const override { return 16; }

private:
    size_t m_totalFrames;
    bool m_throttle;
    size_t m_position = 0;
    bool m_isOpen = false;
};

class LocalPlaybackControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (QCoreApplication::instance() == nullptr)
        {
            static int argc = 1;
            static char appName[] = "test";
            static char* argv[] = { appName };
            m_app = new QCoreApplication(argc, argv);
        }

        m_audioOutput = new StubAudioOutput();
        m_playbackState = new PlaybackState();
        m_controller = new LocalPlaybackController(m_audioOutput, m_playbackState, "stub:null");
    }

    void TearDown() override
    {
        delete m_controller;
        delete m_playbackState;
        delete m_audioOutput;
    }

    /// Process events to allow queued signal/slot connections to fire.
    void processEvents(int timeoutMs = 100)
    {
        QCoreApplication::processEvents();
        if (timeoutMs > 0)
        {
            QThread::msleep(timeoutMs);
            QCoreApplication::processEvents();
        }
    }

    static QCoreApplication* m_app;
    StubAudioOutput* m_audioOutput = nullptr;
    PlaybackState* m_playbackState = nullptr;
    LocalPlaybackController* m_controller = nullptr;
};

QCoreApplication* LocalPlaybackControllerTest::m_app = nullptr;

// --- Play Tests ---

TEST_F(LocalPlaybackControllerTest, PlaySetsPlayingState)
{
    // Throttled stream so playback loop can't finish before we check state
    auto stream = std::make_shared<TestPlaybackStream>(44100 * 10, true);
    m_controller->play(stream);

    // play() sets Playing synchronously on the main thread
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Playing);
    EXPECT_TRUE(m_controller->isActive());

    m_controller->stop();
}

TEST_F(LocalPlaybackControllerTest, PlaySetsDuration)
{
    auto stream = std::make_shared<TestPlaybackStream>(44100 * 5); // 5 seconds
    m_controller->play(stream);

    processEvents(50);
    // 5 seconds = 5000ms (may be slightly different due to integer division)
    EXPECT_GE(m_playbackState->durationMs(), 4999);
    EXPECT_LE(m_playbackState->durationMs(), 5001);

    m_controller->stop();
}

TEST_F(LocalPlaybackControllerTest, PlayNullStreamSetsStoppedState)
{
    m_controller->play(nullptr);
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Stopped);
    EXPECT_FALSE(m_controller->isActive());
}

TEST_F(LocalPlaybackControllerTest, PlayNewStreamAutoStopsCurrent)
{
    // Throttled streams so playback loop can't finish before we check state
    auto stream1 = std::make_shared<TestPlaybackStream>(44100 * 10, true);
    m_controller->play(stream1);
    EXPECT_TRUE(m_controller->isActive());

    // Play a new stream — should auto-stop the first
    auto stream2 = std::make_shared<TestPlaybackStream>(44100 * 5, true);
    m_controller->play(stream2);
    EXPECT_TRUE(m_controller->isActive());
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Playing);

    // Duration should reflect stream2
    EXPECT_GE(m_playbackState->durationMs(), 4999);
    EXPECT_LE(m_playbackState->durationMs(), 5001);

    m_controller->stop();
}

// --- Stop Tests ---

TEST_F(LocalPlaybackControllerTest, StopSetsStoppedState)
{
    auto stream = std::make_shared<TestPlaybackStream>(44100 * 10, true);
    m_controller->play(stream);

    m_controller->stop();
    processEvents(50);
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Stopped);
    EXPECT_FALSE(m_controller->isActive());
    EXPECT_EQ(m_playbackState->positionMs(), 0);
}

// --- Pause/Resume Tests ---

TEST_F(LocalPlaybackControllerTest, PauseSetsPausedState)
{
    auto stream = std::make_shared<TestPlaybackStream>(44100 * 10, true);
    m_controller->play(stream);

    m_controller->pause();
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Paused);

    m_controller->stop();
}

TEST_F(LocalPlaybackControllerTest, ResumeSetsPlayingState)
{
    auto stream = std::make_shared<TestPlaybackStream>(44100 * 10, true);
    m_controller->play(stream);

    m_controller->pause();
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Paused);

    m_controller->resume();
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Playing);

    m_controller->stop();
}

// --- Track Finished Test ---

TEST_F(LocalPlaybackControllerTest, TrackFinishedEmittedAtEnd)
{
    // Very short stream — should finish quickly
    auto stream = std::make_shared<TestPlaybackStream>(1024); // ~23ms of audio
    QSignalSpy finishedSpy(m_controller, &LocalPlaybackController::trackFinished);

    m_controller->play(stream);

    // Wait for playback to complete (should be very fast with StubAudioOutput)
    for (int i = 0; i < 50 && finishedSpy.count() == 0; ++i)
    {
        processEvents(50);
    }

    EXPECT_GE(finishedSpy.count(), 1);
}

// --- Seek Test ---

TEST_F(LocalPlaybackControllerTest, SeekUpdatesPositionOptimistically)
{
    auto stream = std::make_shared<TestPlaybackStream>(44100 * 10, true);
    m_controller->play(stream);

    m_controller->seek(5000); // Seek to 5 seconds
    // Optimistic update — position should be set immediately
    EXPECT_EQ(m_playbackState->positionMs(), 5000);

    m_controller->stop();
}

// --- Destructor Safety Test ---

TEST_F(LocalPlaybackControllerTest, DestructorStopsThread)
{
    auto stream = std::make_shared<TestPlaybackStream>(44100 * 10, true);
    m_controller->play(stream);

    // Destroy controller while playing — should not crash
    delete m_controller;
    m_controller = nullptr;

    // Recreate for TearDown
    m_controller = new LocalPlaybackController(m_audioOutput, m_playbackState, "stub:null");
}
