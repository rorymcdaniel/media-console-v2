#include "CdAudioStream.h"

#include "utils/Logging.h"

#ifdef HAS_CDIO
#include <cdio/paranoia/cdda.h>
#include <cdio/paranoia/paranoia.h>
#endif

#include <algorithm>
#include <cstring>

CdAudioStream::CdAudioStream(const QString& devicePath, int startSector, int endSector)
    : m_devicePath(devicePath)
    , m_startSector(startSector)
    , m_endSector(endSector)
    , m_currentSector(startSector)
{
}

CdAudioStream::~CdAudioStream()
{
    close();
}

bool CdAudioStream::open()
{
#ifdef HAS_CDIO
    m_cdda = cdio_cddap_identify(m_devicePath.toLocal8Bit().data(), CDDA_MESSAGE_FORGETIT, nullptr);
    if (!m_cdda)
    {
        qCWarning(mediaCd) << "CdAudioStream::open: cdio_cddap_identify failed";
        return false;
    }

    int rc = cdio_cddap_open(m_cdda);
    if (rc != 0)
    {
        qCWarning(mediaCd) << "CdAudioStream::open: cdio_cddap_open failed with rc" << rc;
        cdio_cddap_close(m_cdda);
        m_cdda = nullptr;
        return false;
    }

    m_paranoia = cdio_paranoia_init(m_cdda);
    if (!m_paranoia)
    {
        qCWarning(mediaCd) << "CdAudioStream::open: cdio_paranoia_init failed";
        cdio_cddap_close(m_cdda);
        m_cdda = nullptr;
        return false;
    }

    cdio_paranoia_modeset(m_paranoia, PARANOIA_MODE_FULL);
    cdio_paranoia_seek(m_paranoia, m_startSector, SEEK_SET);
    m_currentSector = m_startSector;
    m_sectorBufferOffset = 0;
    m_sectorBufferAvailable = 0;

    qCInfo(mediaCd) << "CdAudioStream::open: sectors" << m_startSector << "-" << m_endSector << "(" << totalFrames()
                    << "frames )";
    return true;
#else
    qCWarning(mediaCd) << "CdAudioStream::open: not compiled with HAS_CDIO";
    return false;
#endif
}

void CdAudioStream::close()
{
#ifdef HAS_CDIO
    if (m_paranoia)
    {
        cdio_paranoia_free(m_paranoia);
        m_paranoia = nullptr;
    }
    if (m_cdda)
    {
        cdio_cddap_close(m_cdda);
        m_cdda = nullptr;
    }
#endif

    m_sectorBufferOffset = 0;
    m_sectorBufferAvailable = 0;
}

long CdAudioStream::readFrames(int16_t* buffer, size_t frames)
{
#ifdef HAS_CDIO
    if (!m_paranoia)
    {
        return -1;
    }

    size_t framesRead = 0;
    int16_t* out = buffer;

    // First: drain any remaining frames from the sector buffer
    if (m_sectorBufferAvailable > m_sectorBufferOffset)
    {
        int samplesAvailable = m_sectorBufferAvailable - m_sectorBufferOffset;
        int framesAvailable = samplesAvailable / 2; // stereo: 2 samples per frame
        int framesToCopy = static_cast<int>(std::min(static_cast<size_t>(framesAvailable), frames));
        int samplesToCopy = framesToCopy * 2;

        std::memcpy(out, m_sectorBuffer + m_sectorBufferOffset, static_cast<size_t>(samplesToCopy) * sizeof(int16_t));
        out += samplesToCopy;
        framesRead += static_cast<size_t>(framesToCopy);
        m_sectorBufferOffset += samplesToCopy;
    }

    // Then: read full sectors until we have enough frames
    while (framesRead < frames && m_currentSector < m_endSector)
    {
        int16_t* sector = cdio_paranoia_read_limited(m_paranoia, nullptr, 10);
        if (!sector)
        {
            qCWarning(mediaCd) << "CdAudioStream::readFrames: paranoia read error at sector" << m_currentSector;
            return -1;
        }

        m_currentSector++;

        size_t framesRemaining = frames - framesRead;
        if (framesRemaining >= static_cast<size_t>(kFramesPerSector))
        {
            // Copy full sector directly to output
            std::memcpy(out, sector, static_cast<size_t>(kSamplesPerSector) * sizeof(int16_t));
            out += kSamplesPerSector;
            framesRead += kFramesPerSector;
        }
        else
        {
            // Partial sector: copy what we need, buffer the rest
            int samplesToCopy = static_cast<int>(framesRemaining) * 2;
            std::memcpy(out, sector, static_cast<size_t>(samplesToCopy) * sizeof(int16_t));
            out += samplesToCopy;
            framesRead += framesRemaining;

            // Buffer remaining samples
            int samplesRemaining = kSamplesPerSector - samplesToCopy;
            std::memcpy(m_sectorBuffer, sector + samplesToCopy,
                        static_cast<size_t>(samplesRemaining) * sizeof(int16_t));
            m_sectorBufferOffset = 0;
            m_sectorBufferAvailable = samplesRemaining;
        }
    }

    return static_cast<long>(framesRead);
#else
    Q_UNUSED(buffer);
    Q_UNUSED(frames);
    return -1;
#endif
}

size_t CdAudioStream::totalFrames() const
{
    return static_cast<size_t>(m_endSector - m_startSector) * kFramesPerSector;
}

size_t CdAudioStream::positionFrames() const
{
    size_t sectorFrames = static_cast<size_t>(m_currentSector - m_startSector) * kFramesPerSector;

    // Subtract any buffered frames that haven't been consumed yet
    if (m_sectorBufferAvailable > m_sectorBufferOffset)
    {
        int bufferedSamples = m_sectorBufferAvailable - m_sectorBufferOffset;
        int bufferedFrames = bufferedSamples / 2;
        if (static_cast<size_t>(bufferedFrames) <= sectorFrames)
        {
            sectorFrames -= static_cast<size_t>(bufferedFrames);
        }
    }

    return sectorFrames;
}

bool CdAudioStream::seek(size_t framePosition)
{
#ifdef HAS_CDIO
    if (!m_paranoia)
    {
        return false;
    }

    int sectorOffset = static_cast<int>(framePosition / kFramesPerSector);
    int sector = m_startSector + sectorOffset;

    if (sector >= m_endSector)
    {
        return false;
    }

    cdio_paranoia_seek(m_paranoia, sector, SEEK_SET);
    m_currentSector = sector;

    // Handle sub-sector offset
    int subSectorFrames = static_cast<int>(framePosition % kFramesPerSector);
    m_sectorBufferOffset = 0;
    m_sectorBufferAvailable = 0;

    if (subSectorFrames > 0)
    {
        // Read one sector and skip the leading frames
        int16_t* sectorData = cdio_paranoia_read_limited(m_paranoia, nullptr, 10);
        if (!sectorData)
        {
            return false;
        }
        m_currentSector++;

        int samplesToSkip = subSectorFrames * 2;
        int samplesRemaining = kSamplesPerSector - samplesToSkip;
        std::memcpy(m_sectorBuffer, sectorData + samplesToSkip,
                    static_cast<size_t>(samplesRemaining) * sizeof(int16_t));
        m_sectorBufferOffset = 0;
        m_sectorBufferAvailable = samplesRemaining;
    }

    return true;
#else
    Q_UNUSED(framePosition);
    return false;
#endif
}

int CdAudioStream::framesToSectors(size_t frames) const
{
    return static_cast<int>(frames / kFramesPerSector);
}
