#include "AudioRingBuffer.h"

#include <algorithm>
#include <cstring>

AudioRingBuffer::AudioRingBuffer(size_t capacityFrames, unsigned int channels)
    : m_capacitySamples(capacityFrames * channels)
    , m_channels(channels)
{
    m_buffer.resize(m_capacitySamples, 0);
}

size_t AudioRingBuffer::write(const int16_t* data, size_t frames)
{
    size_t samplesToWrite = frames * m_channels;
    size_t freeSamples = m_capacitySamples - m_count;
    size_t actualSamples = std::min(samplesToWrite, freeSamples);

    if (actualSamples == 0)
    {
        return 0;
    }

    // Copy in two parts if wrapping around end of buffer
    size_t firstChunk = std::min(actualSamples, m_capacitySamples - m_writePos);
    std::memcpy(&m_buffer[m_writePos], data, firstChunk * sizeof(int16_t));

    size_t secondChunk = actualSamples - firstChunk;
    if (secondChunk > 0)
    {
        std::memcpy(&m_buffer[0], data + firstChunk, secondChunk * sizeof(int16_t));
    }

    m_writePos = (m_writePos + actualSamples) % m_capacitySamples;
    m_count += actualSamples;

    return actualSamples / m_channels;
}

size_t AudioRingBuffer::read(int16_t* data, size_t frames)
{
    size_t samplesToRead = frames * m_channels;
    size_t actualSamples = std::min(samplesToRead, m_count);

    if (actualSamples == 0)
    {
        return 0;
    }

    // Copy in two parts if wrapping around end of buffer
    size_t firstChunk = std::min(actualSamples, m_capacitySamples - m_readPos);
    std::memcpy(data, &m_buffer[m_readPos], firstChunk * sizeof(int16_t));

    size_t secondChunk = actualSamples - firstChunk;
    if (secondChunk > 0)
    {
        std::memcpy(data + firstChunk, &m_buffer[0], secondChunk * sizeof(int16_t));
    }

    m_readPos = (m_readPos + actualSamples) % m_capacitySamples;
    m_count -= actualSamples;

    return actualSamples / m_channels;
}

size_t AudioRingBuffer::availableFrames() const
{
    return m_count / m_channels;
}

size_t AudioRingBuffer::freeFrames() const
{
    return (m_capacitySamples - m_count) / m_channels;
}

size_t AudioRingBuffer::capacityFrames() const
{
    return m_capacitySamples / m_channels;
}

void AudioRingBuffer::clear()
{
    m_readPos = 0;
    m_writePos = 0;
    m_count = 0;
}
