#include "LocalPlaybackController.h"

#include <QMetaObject>

#include <algorithm>
#include <vector>

#include "audio/AudioBufferStats.h"
#include "audio/AudioRingBuffer.h"
#include "audio/AudioStream.h"
#include "platform/IAudioOutput.h"
#include "state/PlaybackMode.h"
#include "state/PlaybackState.h"
#include "utils/Logging.h"

LocalPlaybackController::LocalPlaybackController(IAudioOutput* audioOutput, PlaybackState* playbackState,
                                                 const QString& deviceName, QObject* parent)
    : QObject(parent)
    , m_audioOutput(audioOutput)
    , m_playbackState(playbackState)
    , m_deviceName(deviceName)
    , m_ringBuffer(std::make_unique<AudioRingBuffer>(kBufferFrames, kChannels))
    , m_stats(std::make_unique<AudioBufferStats>())
{
}

LocalPlaybackController::~LocalPlaybackController()
{
    stopThread();
}

void LocalPlaybackController::play(std::shared_ptr<AudioStream> stream)
{
    // Auto-stop current playback (architectural exclusivity)
    if (m_thread.isRunning())
    {
        stopThread();
    }

    if (m_currentStream)
    {
        m_currentStream->close();
        m_currentStream.reset();
    }

    if (!stream)
    {
        qCWarning(mediaAudio) << "LocalPlaybackController: play() called with null stream";
        m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
        return;
    }

    m_currentStream = std::move(stream);

    if (!m_currentStream->open())
    {
        qCWarning(mediaAudio) << "LocalPlaybackController: failed to open audio stream";
        m_currentStream.reset();
        m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
        return;
    }

    if (!m_audioOutput->open(m_deviceName, kSampleRate, kChannels, kBitDepth))
    {
        qCWarning(mediaAudio) << "LocalPlaybackController: failed to open audio output" << m_deviceName;
        m_currentStream->close();
        m_currentStream.reset();
        m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
        return;
    }

    // Set duration from stream
    qint64 durationMs = framesToMs(m_currentStream->totalFrames());
    m_playbackState->setDurationMs(durationMs);
    m_playbackState->setPositionMs(0);

    // Reset control flags
    m_stopRequested.store(false, std::memory_order_relaxed);
    m_pauseRequested.store(false, std::memory_order_relaxed);
    m_seekPending.store(false, std::memory_order_relaxed);

    // Reset playback state
    m_framesPlayed = 0;
    m_streamEndReached = false;
    m_ringBuffer->clear();
    m_stats->reset();
    m_lastPositionUpdate = std::chrono::steady_clock::now();

    m_playbackState->setPlaybackMode(PlaybackMode::Playing);

    qCInfo(mediaAudio) << "LocalPlaybackController: starting playback, duration" << durationMs << "ms";

    // Start playback thread
    // Use DirectConnection so the lambda runs on m_thread, not the caller's thread
    auto* connection = new QMetaObject::Connection;
    *connection = connect(
        &m_thread, &QThread::started, this,
        [this, connection]()
        {
            playbackLoop();
            disconnect(*connection);
            delete connection;
        },
        Qt::DirectConnection);
    m_thread.start();
}

void LocalPlaybackController::pause()
{
    m_pauseRequested.store(true, std::memory_order_release);
    // Immediate silence per user decision — ALSA reset/flush, no buffer drain
    m_audioOutput->reset();
    m_playbackState->setPlaybackMode(PlaybackMode::Paused);
    qCInfo(mediaAudio) << "LocalPlaybackController: paused";
}

void LocalPlaybackController::resume()
{
    m_pauseRequested.store(false, std::memory_order_release);
    m_audioOutput->pause(false);
    m_playbackState->setPlaybackMode(PlaybackMode::Playing);
    qCInfo(mediaAudio) << "LocalPlaybackController: resumed";
}

void LocalPlaybackController::stop()
{
    stopThread();

    if (m_currentStream)
    {
        m_currentStream->close();
        m_currentStream.reset();
    }

    m_audioOutput->close();
    m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
    m_playbackState->setPositionMs(0);

    // Log stats
    qCInfo(mediaAudio) << "LocalPlaybackController: stopped. Stats — xruns:" << m_stats->xrunCount
                       << "errors:" << m_stats->errorCount << "avg_latency:" << m_stats->readLatencyAvgUs << "us"
                       << "max_latency:" << m_stats->readLatencyMaxUs << "us";
}

void LocalPlaybackController::seek(qint64 positionMs)
{
    // Optimistic UI update per user decision
    m_playbackState->setPositionMs(positionMs);
    m_seekTargetMs.store(positionMs, std::memory_order_relaxed);
    m_seekPending.store(true, std::memory_order_release);
    qCInfo(mediaAudio) << "LocalPlaybackController: seek to" << positionMs << "ms";
}

bool LocalPlaybackController::isActive() const
{
    return m_thread.isRunning();
}

void LocalPlaybackController::playbackLoop()
{
    std::vector<int16_t> chunk(kPeriodFrames * kChannels);

    // Prefill ring buffer before starting playback
    qCDebug(mediaAudio) << "LocalPlaybackController: prefilling buffer...";
    while (m_ringBuffer->availableFrames() < kPrefillFrames && !m_streamEndReached
           && !m_stopRequested.load(std::memory_order_relaxed))
    {
        fillBuffer();
    }
    qCDebug(mediaAudio) << "LocalPlaybackController: prefill complete," << m_ringBuffer->availableFrames()
                        << "frames buffered";

    while (!m_stopRequested.load(std::memory_order_relaxed))
    {
        // Check for seek request
        if (m_seekPending.load(std::memory_order_acquire))
        {
            handleSeek();
            continue;
        }

        // Check for pause
        if (m_pauseRequested.load(std::memory_order_relaxed))
        {
            QThread::msleep(10);
            continue;
        }

        // Fill ring buffer from stream
        fillBuffer();

        // Check for end of stream
        if (m_ringBuffer->availableFrames() == 0 && m_streamEndReached)
        {
            qCInfo(mediaAudio) << "LocalPlaybackController: end of track at" << m_framesPlayed << "frames";
            QMetaObject::invokeMethod(
                this,
                [this]()
                {
                    m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
                    emit trackFinished();
                },
                Qt::QueuedConnection);
            break;
        }

        // Determine how much to write
        size_t available = m_ringBuffer->availableFrames();
        if (available == 0)
        {
            // Buffer empty but stream not ended — wait for data
            QThread::msleep(1);
            continue;
        }

        size_t toWrite = std::min(available, kPeriodFrames);
        m_ringBuffer->read(chunk.data(), toWrite);

        auto start = std::chrono::steady_clock::now();
        long written = m_audioOutput->writeFrames(chunk.data(), toWrite);
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);

        if (written < 0)
        {
            // I/O error — attempt recovery
            if (!attemptRecovery())
            {
                qCWarning(mediaAudio) << "LocalPlaybackController: recovery failed, stopping playback";
                QMetaObject::invokeMethod(
                    this,
                    [this]()
                    {
                        m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
                        emit audioRecoveryFailed();
                    },
                    Qt::QueuedConnection);
                break;
            }
            continue; // Retry after recovery
        }

        if (written > 0)
        {
            m_framesPlayed += static_cast<size_t>(written);
            m_stats->recordReadLatency(static_cast<double>(elapsed.count()));
        }

        updatePosition();
    }
}

void LocalPlaybackController::fillBuffer()
{
    std::vector<int16_t> temp(kPeriodFrames * kChannels);

    while (m_ringBuffer->freeFrames() >= kPeriodFrames && !m_streamEndReached)
    {
        long readResult = m_currentStream->readFrames(temp.data(), kPeriodFrames);
        if (readResult == 0)
        {
            m_streamEndReached = true;
            break;
        }
        if (readResult < 0)
        {
            m_stats->recordError();
            break;
        }
        m_ringBuffer->write(temp.data(), static_cast<size_t>(readResult));
    }
}

void LocalPlaybackController::handleSeek()
{
    m_seekPending.store(false, std::memory_order_release);
    qint64 targetMs = m_seekTargetMs.load(std::memory_order_relaxed);
    size_t targetFrame = msToFrames(targetMs);

    // Seek in the audio stream
    m_currentStream->seek(targetFrame);

    // Clear the ring buffer and refill from new position
    m_ringBuffer->clear();
    m_framesPlayed = targetFrame;
    m_streamEndReached = false;

    // Flush old audio from ALSA
    m_audioOutput->reset();

    // Prefill before resuming
    while (m_ringBuffer->availableFrames() < kPrefillFrames && !m_streamEndReached
           && !m_stopRequested.load(std::memory_order_relaxed))
    {
        fillBuffer();
    }

    qCDebug(mediaAudio) << "LocalPlaybackController: seeked to" << targetMs << "ms (frame" << targetFrame << ")";
}

bool LocalPlaybackController::attemptRecovery()
{
    for (int retry = 0; retry < kMaxRecoveryRetries; ++retry)
    {
        qCInfo(mediaAudio) << "LocalPlaybackController: recovery attempt" << (retry + 1) << "/" << kMaxRecoveryRetries;
        m_audioOutput->close();
        QThread::msleep(kRecoveryBackoffMs);

        if (m_audioOutput->open(m_deviceName, kSampleRate, kChannels, kBitDepth))
        {
            m_stats->recordXrun();
            qCInfo(mediaAudio) << "LocalPlaybackController: recovery succeeded on attempt" << (retry + 1);
            return true;
        }
        m_stats->recordError();
    }

    qCWarning(mediaAudio) << "LocalPlaybackController: all" << kMaxRecoveryRetries << "recovery retries exhausted";
    return false;
}

void LocalPlaybackController::updatePosition()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPositionUpdate);

    if (elapsed.count() >= kPositionUpdateIntervalMs)
    {
        qint64 posMs = framesToMs(m_framesPlayed);
        QMetaObject::invokeMethod(
            this, [this, posMs]() { m_playbackState->setPositionMs(posMs); }, Qt::QueuedConnection);
        m_lastPositionUpdate = now;
    }
}

void LocalPlaybackController::stopThread()
{
    if (!m_thread.isRunning())
    {
        return;
    }

    m_stopRequested.store(true, std::memory_order_release);
    m_thread.quit();
    if (!m_thread.wait(5000))
    {
        qCWarning(mediaAudio) << "LocalPlaybackController: thread did not stop within 5s, terminating";
        m_thread.terminate();
        m_thread.wait();
    }
}

qint64 LocalPlaybackController::framesToMs(size_t frames) const
{
    return static_cast<qint64>(frames * 1000 / kSampleRate);
}

size_t LocalPlaybackController::msToFrames(qint64 ms) const
{
    return static_cast<size_t>(ms * kSampleRate / 1000);
}
