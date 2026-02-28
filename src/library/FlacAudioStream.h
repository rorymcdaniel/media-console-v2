#pragma once

#include <QString>

#include "audio/AudioStream.h"

// Forward declarations to avoid header pollution
struct SNDFILE_tag;
typedef struct SNDFILE_tag SNDFILE;
struct SRC_STATE_tag;
typedef struct SRC_STATE_tag SRC_STATE;

/// AudioStream implementation that decodes FLAC files using libsndfile
/// and resamples to 44100Hz/16-bit/stereo using libsamplerate.
/// Only compiled on Linux when HAS_SNDFILE is defined.
class FlacAudioStream : public AudioStream
{
public:
    explicit FlacAudioStream(const QString& filePath);
    ~FlacAudioStream() override;

    bool open() override;
    void close() override;
    long readFrames(int16_t* buffer, size_t frames) override;
    size_t totalFrames() const override;
    size_t positionFrames() const override;
    bool seek(size_t framePosition) override;
    unsigned int sampleRate() const override { return 44100; }
    unsigned int channels() const override { return 2; }
    unsigned int bitDepth() const override { return 16; }

    /// File path accessor (for testing and logging)
    QString filePath() const { return m_filePath; }

private:
    long readAndResample(int16_t* buffer, size_t frames);
    long readDirect(int16_t* buffer, size_t frames);

    QString m_filePath;
    SNDFILE* m_sndfile = nullptr;

    // Native file properties
    int m_nativeRate = 0;
    int m_nativeChannels = 0;
    size_t m_totalNativeFrames = 0;

    // Resampling state
    bool m_needsResample = false;
    double m_srcRatio = 1.0;
    SRC_STATE* m_srcState = nullptr;

    // Position tracking
    size_t m_outputFramesRead = 0;

    // Constants
    static constexpr unsigned int kOutputSampleRate = 44100;
    static constexpr unsigned int kOutputChannels = 2;
    static constexpr unsigned int kOutputBitDepth = 16;
};
