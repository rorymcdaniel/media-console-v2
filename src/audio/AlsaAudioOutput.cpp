#include "AlsaAudioOutput.h"

#include <alsa/asoundlib.h>

#include "utils/Logging.h"

AlsaAudioOutput::AlsaAudioOutput() = default;

AlsaAudioOutput::~AlsaAudioOutput()
{
    close();
}

bool AlsaAudioOutput::open(const QString& deviceName, unsigned int sampleRate, unsigned int channels,
                           unsigned int bitDepth)
{
    Q_UNUSED(bitDepth);

    if (m_open)
    {
        close();
    }

    int err = snd_pcm_open(&m_handle, deviceName.toLocal8Bit().constData(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: failed to open" << deviceName << "-" << snd_strerror(err);
        return false;
    }

    // Set hardware parameters
    snd_pcm_hw_params_t* hwParams = nullptr;
    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_hw_params_any(m_handle, hwParams);

    err = snd_pcm_hw_params_set_access(m_handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: set_access failed -" << snd_strerror(err);
        snd_pcm_close(m_handle);
        m_handle = nullptr;
        return false;
    }

    err = snd_pcm_hw_params_set_format(m_handle, hwParams, SND_PCM_FORMAT_S16_LE);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: set_format failed -" << snd_strerror(err);
        snd_pcm_close(m_handle);
        m_handle = nullptr;
        return false;
    }

    err = snd_pcm_hw_params_set_channels(m_handle, hwParams, channels);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: set_channels failed -" << snd_strerror(err);
        snd_pcm_close(m_handle);
        m_handle = nullptr;
        return false;
    }

    unsigned int requestedRate = sampleRate;
    err = snd_pcm_hw_params_set_rate_near(m_handle, hwParams, &requestedRate, nullptr);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: set_rate failed -" << snd_strerror(err);
        snd_pcm_close(m_handle);
        m_handle = nullptr;
        return false;
    }

    if (requestedRate != sampleRate)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: requested rate" << sampleRate << "but got" << requestedRate;
    }

    // Period = 1024 frames (~23ms at 44100Hz), Buffer = 8192 frames (~186ms)
    snd_pcm_uframes_t periodSize = 1024;
    err = snd_pcm_hw_params_set_period_size_near(m_handle, hwParams, &periodSize, nullptr);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: set_period_size failed -" << snd_strerror(err);
    }

    snd_pcm_uframes_t bufferSize = 8192;
    err = snd_pcm_hw_params_set_buffer_size_near(m_handle, hwParams, &bufferSize);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: set_buffer_size failed -" << snd_strerror(err);
    }

    err = snd_pcm_hw_params(m_handle, hwParams);
    if (err < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: hw_params install failed -" << snd_strerror(err);
        snd_pcm_close(m_handle);
        m_handle = nullptr;
        return false;
    }

    m_deviceName = deviceName;
    m_open = true;

    qCInfo(mediaAudio) << "AlsaAudioOutput: opened" << deviceName << "at" << requestedRate << "Hz" << channels << "ch"
                       << "period" << periodSize << "buffer" << bufferSize;

    return true;
}

void AlsaAudioOutput::close()
{
    if (m_handle != nullptr)
    {
        snd_pcm_close(m_handle);
        m_handle = nullptr;
    }
    m_open = false;
    qCInfo(mediaAudio) << "AlsaAudioOutput: closed";
}

void AlsaAudioOutput::reset()
{
    if (m_handle != nullptr)
    {
        // snd_pcm_drop: immediately stops playback and discards buffered data
        // This implements "pause cuts audio immediately" per user decision
        snd_pcm_drop(m_handle);
        snd_pcm_prepare(m_handle);
    }
    qCDebug(mediaAudio) << "AlsaAudioOutput: reset (drop + prepare)";
}

void AlsaAudioOutput::pause(bool paused)
{
    if (m_handle == nullptr)
    {
        return;
    }

    if (paused)
    {
        // Try hardware pause if supported
        int err = snd_pcm_pause(m_handle, 1);
        if (err == -ENOSYS)
        {
            // Hardware pause not supported — use drop + prepare
            snd_pcm_drop(m_handle);
            snd_pcm_prepare(m_handle);
        }
    }
    else
    {
        // Resume — try hardware unpause first
        int err = snd_pcm_pause(m_handle, 0);
        if (err < 0)
        {
            // If unpause fails, just prepare
            snd_pcm_prepare(m_handle);
        }
    }
    qCDebug(mediaAudio) << "AlsaAudioOutput: pause =" << paused;
}

long AlsaAudioOutput::writeFrames(const int16_t* interleaved, size_t frames)
{
    if (m_handle == nullptr)
    {
        return -1;
    }

    snd_pcm_sframes_t written = snd_pcm_writei(m_handle, interleaved, static_cast<snd_pcm_uframes_t>(frames));

    if (written == -EPIPE)
    {
        // Buffer underrun recovery
        qCWarning(mediaAudio) << "AlsaAudioOutput: underrun (EPIPE), recovering";
        snd_pcm_prepare(m_handle);
        written = snd_pcm_writei(m_handle, interleaved, static_cast<snd_pcm_uframes_t>(frames));
    }
    else if (written == -EIO)
    {
        // Hardware I/O error — caller should initiate close/reopen recovery
        qCWarning(mediaAudio) << "AlsaAudioOutput: I/O error (EIO)";
        return -1;
    }
    else if (written < 0)
    {
        // Other error — attempt generic recovery
        int recovered = snd_pcm_recover(m_handle, static_cast<int>(written), 1 /* silent */);
        if (recovered < 0)
        {
            qCWarning(mediaAudio) << "AlsaAudioOutput: recovery failed -" << snd_strerror(recovered);
            return -1;
        }
        written = snd_pcm_writei(m_handle, interleaved, static_cast<snd_pcm_uframes_t>(frames));
    }

    if (written < 0)
    {
        qCWarning(mediaAudio) << "AlsaAudioOutput: write failed after recovery -"
                              << snd_strerror(static_cast<int>(written));
        return -1;
    }

    return static_cast<long>(written);
}

bool AlsaAudioOutput::isOpen() const
{
    return m_open;
}

QString AlsaAudioOutput::deviceName() const
{
    return m_deviceName;
}
