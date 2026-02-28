#pragma once

#include <QString>

#include <cstddef>
#include <cstdint>

class IAudioOutput
{
public:
    virtual ~IAudioOutput() = default;

    virtual bool open(const QString& deviceName, unsigned int sampleRate, unsigned int channels, unsigned int bitDepth)
        = 0;
    virtual void close() = 0;
    virtual void reset() = 0;
    virtual void pause(bool paused) = 0;
    virtual long writeFrames(const int16_t* interleaved, size_t frames) = 0;
    virtual bool isOpen() const = 0;
    virtual QString deviceName() const = 0;
};
