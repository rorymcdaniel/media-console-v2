#pragma once

#include "platform/IAudioOutput.h"

class StubAudioOutput : public IAudioOutput
{
public:
    StubAudioOutput() = default;
    ~StubAudioOutput() override = default;

    bool open(const QString& deviceName, unsigned int sampleRate, unsigned int channels,
              unsigned int bitDepth) override;
    void close() override;
    void reset() override;
    void pause(bool paused) override;
    long writeFrames(const int16_t* interleaved, size_t frames) override;
    bool isOpen() const override;
    QString deviceName() const override;

private:
    bool m_open = false;
    QString m_deviceName;
};
