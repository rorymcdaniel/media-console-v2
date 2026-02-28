#pragma once

#include <QString>

#include "audio/AudioStream.h"

// Forward declarations to avoid libcdio-paranoia header pollution
typedef struct cdrom_drive_s cdrom_drive_t;
typedef struct cdrom_paranoia_s cdrom_paranoia_t;

/// AudioStream implementation that extracts CD audio using libcdio-paranoia
/// for error-corrected reading. Outputs 44100Hz 16-bit stereo interleaved PCM.
/// Only compiled on Linux when HAS_CDIO is defined.
class CdAudioStream : public AudioStream
{
public:
    CdAudioStream(const QString& devicePath, int startSector, int endSector);
    ~CdAudioStream() override;

    bool open() override;
    void close() override;
    long readFrames(int16_t* buffer, size_t frames) override;
    size_t totalFrames() const override;
    size_t positionFrames() const override;
    bool seek(size_t framePosition) override;
    unsigned int sampleRate() const override { return 44100; }
    unsigned int channels() const override { return 2; }
    unsigned int bitDepth() const override { return 16; }

    /// Constants exposed for testing
    static constexpr int kFramesPerSector = 588;
    static constexpr int kBytesPerFrame = 4; // 16-bit stereo = 4 bytes
    static constexpr int kSamplesPerSector = 588 * 2; // stereo

    /// Helper: convert frame position to sector offset from start
    int framesToSectors(size_t frames) const;

private:
    QString m_devicePath;
    int m_startSector;
    int m_endSector;
    int m_currentSector;

    cdrom_drive_t* m_cdda = nullptr;
    cdrom_paranoia_t* m_paranoia = nullptr;

    // Partial sector buffer for readFrames that don't align to sector boundaries
    int16_t m_sectorBuffer[kSamplesPerSector] = {};
    int m_sectorBufferOffset = 0; // Next sample index to read from buffer
    int m_sectorBufferAvailable = 0; // Total samples available in buffer
};
