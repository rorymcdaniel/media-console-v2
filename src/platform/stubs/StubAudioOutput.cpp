#include "StubAudioOutput.h"

#include "utils/Logging.h"

bool StubAudioOutput::open(const QString& deviceName, unsigned int sampleRate, unsigned int channels,
                           unsigned int bitDepth)
{
    m_deviceName = "stub:null";
    m_open = true;
    qCInfo(mediaAudio) << "StubAudioOutput: opened" << deviceName << sampleRate << "Hz" << channels << "ch" << bitDepth
                       << "bit";
    return true;
}

void StubAudioOutput::close()
{
    m_open = false;
    qCInfo(mediaAudio) << "StubAudioOutput: closed";
}

void StubAudioOutput::reset()
{
    qCInfo(mediaAudio) << "StubAudioOutput: reset";
}

void StubAudioOutput::pause(bool paused)
{
    qCInfo(mediaAudio) << "StubAudioOutput: pause =" << paused;
}

long StubAudioOutput::writeFrames(const int16_t* /* interleaved */, size_t frames)
{
    // Accept frames silently -- stub behavior
    return static_cast<long>(frames);
}

bool StubAudioOutput::isOpen() const
{
    return m_open;
}

QString StubAudioOutput::deviceName() const
{
    return m_deviceName;
}
