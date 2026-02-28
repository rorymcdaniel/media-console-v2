#include "FlacAudioStream.h"

#include <algorithm>
#include <cmath>
#include <samplerate.h>
#include <sndfile.h>
#include <vector>

#include "utils/Logging.h"

FlacAudioStream::FlacAudioStream(const QString& filePath)
    : m_filePath(filePath)
{
}

FlacAudioStream::~FlacAudioStream()
{
    close();
}

bool FlacAudioStream::open()
{
    SF_INFO sfInfo = {};
    m_sndfile = sf_open(m_filePath.toLocal8Bit().data(), SFM_READ, &sfInfo);

    if (!m_sndfile)
    {
        qCWarning(mediaLibrary) << "FlacAudioStream::open: failed to open" << m_filePath << "-" << sf_strerror(nullptr);
        return false;
    }

    m_nativeRate = sfInfo.samplerate;
    m_nativeChannels = sfInfo.channels;
    m_totalNativeFrames = static_cast<size_t>(sfInfo.frames);

    m_needsResample = (m_nativeRate != static_cast<int>(kOutputSampleRate));

    if (m_needsResample)
    {
        int error = 0;
        m_srcState = src_new(SRC_SINC_MEDIUM_QUALITY, m_nativeChannels, &error);
        if (!m_srcState)
        {
            qCWarning(mediaLibrary) << "FlacAudioStream::open: failed to create resampler -" << src_strerror(error);
            sf_close(m_sndfile);
            m_sndfile = nullptr;
            return false;
        }
        m_srcRatio = static_cast<double>(kOutputSampleRate) / m_nativeRate;
    }

    m_outputFramesRead = 0;

    qCInfo(mediaLibrary) << "FlacAudioStream: opened" << m_filePath << "rate:" << m_nativeRate
                         << "ch:" << m_nativeChannels << "frames:" << m_totalNativeFrames
                         << "resample:" << m_needsResample;
    return true;
}

void FlacAudioStream::close()
{
    if (m_srcState)
    {
        src_delete(m_srcState);
        m_srcState = nullptr;
    }

    if (m_sndfile)
    {
        sf_close(m_sndfile);
        m_sndfile = nullptr;
    }
}

long FlacAudioStream::readFrames(int16_t* buffer, size_t frames)
{
    if (!m_sndfile)
    {
        return -1;
    }

    if (m_needsResample)
    {
        return readAndResample(buffer, frames);
    }
    return readDirect(buffer, frames);
}

long FlacAudioStream::readDirect(int16_t* buffer, size_t frames)
{
    if (m_nativeChannels == 1)
    {
        // Mono: read into temporary buffer, then duplicate to stereo
        std::vector<int16_t> mono(frames);
        long read = sf_readf_short(m_sndfile, mono.data(), static_cast<sf_count_t>(frames));
        if (read <= 0)
        {
            return read == 0 ? 0 : -1;
        }

        // Duplicate mono to stereo
        for (long i = 0; i < read; ++i)
        {
            buffer[i * 2] = mono[static_cast<size_t>(i)];
            buffer[i * 2 + 1] = mono[static_cast<size_t>(i)];
        }

        m_outputFramesRead += static_cast<size_t>(read);
        return read;
    }

    // Stereo (or multi-channel — use first 2 channels)
    if (m_nativeChannels == 2)
    {
        long read = sf_readf_short(m_sndfile, buffer, static_cast<sf_count_t>(frames));
        if (read <= 0)
        {
            return read == 0 ? 0 : -1;
        }
        m_outputFramesRead += static_cast<size_t>(read);
        return read;
    }

    // Multi-channel: read all channels, extract first two
    std::vector<int16_t> multi(frames * static_cast<size_t>(m_nativeChannels));
    long read = sf_readf_short(m_sndfile, multi.data(), static_cast<sf_count_t>(frames));
    if (read <= 0)
    {
        return read == 0 ? 0 : -1;
    }

    for (long i = 0; i < read; ++i)
    {
        buffer[i * 2] = multi[static_cast<size_t>(i * m_nativeChannels)];
        buffer[i * 2 + 1] = multi[static_cast<size_t>(i * m_nativeChannels + 1)];
    }

    m_outputFramesRead += static_cast<size_t>(read);
    return read;
}

long FlacAudioStream::readAndResample(int16_t* buffer, size_t frames)
{
    // Calculate how many native frames we need to read
    size_t nativeFramesNeeded = static_cast<size_t>(std::ceil(frames / m_srcRatio)) + 16;

    // Read native frames as float (libsamplerate works with floats)
    std::vector<float> nativeFloat(nativeFramesNeeded * static_cast<size_t>(m_nativeChannels));
    long nativeRead = sf_readf_float(m_sndfile, nativeFloat.data(), static_cast<sf_count_t>(nativeFramesNeeded));

    if (nativeRead <= 0)
    {
        return nativeRead == 0 ? 0 : -1;
    }

    // Prepare output buffer (float, then convert to int16)
    size_t maxOutputFrames = static_cast<size_t>(std::ceil(nativeRead * m_srcRatio)) + 16;
    std::vector<float> outputFloat(maxOutputFrames * static_cast<size_t>(m_nativeChannels));

    SRC_DATA srcData = {};
    srcData.data_in = nativeFloat.data();
    srcData.data_out = outputFloat.data();
    srcData.input_frames = nativeRead;
    srcData.output_frames = static_cast<long>(maxOutputFrames);
    srcData.src_ratio = m_srcRatio;
    srcData.end_of_input = (nativeRead < static_cast<long>(nativeFramesNeeded)) ? 1 : 0;

    int error = src_process(m_srcState, &srcData);
    if (error != 0)
    {
        qCWarning(mediaLibrary) << "FlacAudioStream::readAndResample: src_process error -" << src_strerror(error);
        return -1;
    }

    long outputFrames = std::min(srcData.output_frames_gen, static_cast<long>(frames));

    // Convert float to int16 with stereo handling
    if (m_nativeChannels == 1)
    {
        // Mono to stereo
        for (long i = 0; i < outputFrames; ++i)
        {
            float sample = std::clamp(outputFloat[static_cast<size_t>(i)], -1.0f, 1.0f);
            auto s = static_cast<int16_t>(sample * 32767.0f);
            buffer[i * 2] = s;
            buffer[i * 2 + 1] = s;
        }
    }
    else if (m_nativeChannels == 2)
    {
        // Stereo
        for (long i = 0; i < outputFrames; ++i)
        {
            size_t idx = static_cast<size_t>(i) * 2;
            float left = std::clamp(outputFloat[idx], -1.0f, 1.0f);
            float right = std::clamp(outputFloat[idx + 1], -1.0f, 1.0f);
            buffer[i * 2] = static_cast<int16_t>(left * 32767.0f);
            buffer[i * 2 + 1] = static_cast<int16_t>(right * 32767.0f);
        }
    }
    else
    {
        // Multi-channel: take first two
        for (long i = 0; i < outputFrames; ++i)
        {
            size_t idx = static_cast<size_t>(i * m_nativeChannels);
            float left = std::clamp(outputFloat[idx], -1.0f, 1.0f);
            float right = std::clamp(outputFloat[idx + 1], -1.0f, 1.0f);
            buffer[i * 2] = static_cast<int16_t>(left * 32767.0f);
            buffer[i * 2 + 1] = static_cast<int16_t>(right * 32767.0f);
        }
    }

    m_outputFramesRead += static_cast<size_t>(outputFrames);
    return outputFrames;
}

size_t FlacAudioStream::totalFrames() const
{
    if (m_needsResample)
    {
        return static_cast<size_t>(m_totalNativeFrames * m_srcRatio);
    }
    return m_totalNativeFrames;
}

size_t FlacAudioStream::positionFrames() const
{
    return m_outputFramesRead;
}

bool FlacAudioStream::seek(size_t framePosition)
{
    if (!m_sndfile)
    {
        return false;
    }

    sf_count_t nativeFrame;
    if (m_needsResample)
    {
        nativeFrame = static_cast<sf_count_t>(framePosition / m_srcRatio);
    }
    else
    {
        nativeFrame = static_cast<sf_count_t>(framePosition);
    }

    sf_count_t result = sf_seek(m_sndfile, nativeFrame, SEEK_SET);
    if (result < 0)
    {
        qCWarning(mediaLibrary) << "FlacAudioStream::seek: failed at frame" << framePosition;
        return false;
    }

    // Reset resampler state to avoid artifacts from stale buffers
    if (m_srcState)
    {
        src_reset(m_srcState);
    }

    m_outputFramesRead = framePosition;
    return true;
}
