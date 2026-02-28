#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/// Circular buffer for interleaved 16-bit PCM audio frames.
/// Provides lock-free single-producer/single-consumer buffering between
/// the stream reader and the audio output writer on the playback thread.
///
/// All operations are in frames (not samples). Internally stores
/// frames * channels int16_t samples.
class AudioRingBuffer
{
public:
    /// Create ring buffer with given capacity in frames.
    /// @param capacityFrames Maximum number of frames the buffer can hold.
    /// @param channels Number of interleaved channels (default: 2 for stereo).
    explicit AudioRingBuffer(size_t capacityFrames, unsigned int channels = 2);

    /// Write interleaved frames into the buffer.
    /// Returns number of frames actually written (may be less if buffer full).
    size_t write(const int16_t* data, size_t frames);

    /// Read interleaved frames from the buffer.
    /// Returns number of frames actually read (may be less if buffer empty).
    size_t read(int16_t* data, size_t frames);

    /// Number of frames available to read.
    size_t availableFrames() const;

    /// Number of frames that can be written.
    size_t freeFrames() const;

    /// Total capacity in frames.
    size_t capacityFrames() const;

    /// Clear all data, reset positions.
    void clear();

private:
    std::vector<int16_t> m_buffer;
    size_t m_readPos = 0; // In samples (not frames)
    size_t m_writePos = 0; // In samples (not frames)
    size_t m_count = 0; // Samples currently in buffer
    size_t m_capacitySamples;
    unsigned int m_channels;
};
