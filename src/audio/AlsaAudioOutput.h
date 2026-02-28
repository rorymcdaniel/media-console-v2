#pragma once

#include "platform/IAudioOutput.h"

// Forward declare ALSA types to avoid header pollution
typedef struct _snd_pcm snd_pcm_t;

/// ALSA PCM audio output implementing IAudioOutput.
/// Opens the specified device in SND_PCM_ACCESS_RW_INTERLEAVED mode with
/// hardware parameters matching 44100Hz/16-bit/stereo for S/PDIF HAT output.
///
/// Error handling:
/// - EPIPE (underrun): automatically calls snd_pcm_prepare and retries
/// - EIO: returns -1 for caller to initiate close/reopen recovery
/// - Other errors: attempts snd_pcm_recover, returns -1 on failure
class AlsaAudioOutput : public IAudioOutput
{
public:
    AlsaAudioOutput();
    ~AlsaAudioOutput() override;

    bool open(const QString& deviceName, unsigned int sampleRate, unsigned int channels,
              unsigned int bitDepth) override;
    void close() override;
    void reset() override;
    void pause(bool paused) override;
    long writeFrames(const int16_t* interleaved, size_t frames) override;
    bool isOpen() const override;
    QString deviceName() const override;

private:
    snd_pcm_t* m_handle = nullptr;
    QString m_deviceName;
    bool m_open = false;
};
