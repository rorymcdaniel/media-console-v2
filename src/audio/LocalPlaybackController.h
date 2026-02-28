#pragma once

#include <QObject>
#include <QString>
#include <QThread>

#include <atomic>
#include <chrono>
#include <memory>

class AudioStream;
class IAudioOutput;
class PlaybackState;
class AudioRingBuffer;
struct AudioBufferStats;

/// Unified playback controller for any AudioStream source.
/// Reads frames from AudioStream, buffers through AudioRingBuffer, and writes
/// to IAudioOutput on a background thread. Provides play/pause/stop/seek controls
/// from the main thread without blocking the UI.
///
/// Architectural exclusivity: play(newStream) auto-stops any current playback.
/// Only one audio source can play at a time — enforced by single controller ownership.
class LocalPlaybackController : public QObject
{
    Q_OBJECT

public:
    LocalPlaybackController(IAudioOutput* audioOutput, PlaybackState* playbackState, const QString& deviceName,
                            QObject* parent = nullptr);
    ~LocalPlaybackController() override;

    /// Start playing the given audio stream. Auto-stops any current playback.
    /// Takes shared ownership of the stream.
    void play(std::shared_ptr<AudioStream> stream);

    /// Pause playback — cuts audio immediately (ALSA reset/flush, no drain).
    void pause();

    /// Resume from pause.
    void resume();

    /// Stop playback and close the stream.
    void stop();

    /// Seek to position in milliseconds. Updates UI optimistically.
    void seek(qint64 positionMs);

    /// Returns true if currently playing or paused.
    bool isActive() const;

signals:
    /// Emitted when the current stream reaches its end.
    void trackFinished();

    /// Emitted when ALSA EIO recovery exhausts all retries.
    void audioRecoveryFailed();

private:
    void playbackLoop();
    void fillBuffer();
    void handleSeek();
    bool attemptRecovery();
    void updatePosition();
    void stopThread();

    // Helpers
    qint64 framesToMs(size_t frames) const;
    size_t msToFrames(qint64 ms) const;

    IAudioOutput* m_audioOutput; // Non-owning
    PlaybackState* m_playbackState; // Non-owning
    QString m_deviceName;

    QThread m_thread;
    std::shared_ptr<AudioStream> m_currentStream;
    std::unique_ptr<AudioRingBuffer> m_ringBuffer;
    std::unique_ptr<AudioBufferStats> m_stats;

    // Atomic control flags — written by main thread, read by playback thread
    std::atomic<bool> m_stopRequested { false };
    std::atomic<bool> m_pauseRequested { false };
    std::atomic<bool> m_seekPending { false };
    std::atomic<qint64> m_seekTargetMs { 0 };

    // Playback thread state (only accessed from playback thread)
    size_t m_framesPlayed = 0;
    std::chrono::steady_clock::time_point m_lastPositionUpdate;
    bool m_streamEndReached = false;

    // Named constants from requirements (locked, not configurable)
    static constexpr size_t kBufferFrames = 352800; // 8 seconds at 44100Hz
    static constexpr size_t kPrefillFrames = 44100; // 1 second prefill
    static constexpr size_t kPeriodFrames = 1024; // Read/write chunk size
    static constexpr int kMaxRecoveryRetries = 3;
    static constexpr int kRecoveryBackoffMs = 50;
    static constexpr int kPositionUpdateIntervalMs = 250;
    static constexpr unsigned int kSampleRate = 44100;
    static constexpr unsigned int kChannels = 2;
    static constexpr unsigned int kBitDepth = 16;
};
