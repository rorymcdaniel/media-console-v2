#pragma once

#include <cstddef>
#include <cstdint>

/// Abstract interface for audio sources. Implementations include CdAudioStream (Phase 5)
/// and FlacAudioStream (Phase 6). The LocalPlaybackController reads frames from this
/// interface and writes them to IAudioOutput.
///
/// All implementations MUST output 44100Hz, 16-bit, stereo interleaved PCM.
/// Any format conversion (resampling, bit depth) is the implementation's responsibility.
class AudioStream
{
public:
    virtual ~AudioStream() = default;

    /// Open the audio source. Returns true on success.
    virtual bool open() = 0;

    /// Close the audio source and release resources.
    virtual void close() = 0;

    /// Read interleaved 16-bit PCM frames into buffer.
    /// Returns number of frames actually read (0 = end of stream, -1 = error).
    virtual long readFrames(int16_t* buffer, size_t frames) = 0;

    /// Total number of frames in the stream (0 if unknown/streaming).
    virtual size_t totalFrames() const = 0;

    /// Current read position in frames.
    virtual size_t positionFrames() const = 0;

    /// Seek to a frame position. Returns true on success.
    virtual bool seek(size_t framePosition) = 0;

    /// Audio format accessors.
    virtual unsigned int sampleRate() const = 0;
    virtual unsigned int channels() const = 0;
    virtual unsigned int bitDepth() const = 0;
};
