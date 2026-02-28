#include "LibcdioCdDrive.h"

#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include <discid/discid.h>

#include "utils/Logging.h"

#ifdef __linux__
#include <fcntl.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

LibcdioCdDrive::LibcdioCdDrive() = default;

LibcdioCdDrive::~LibcdioCdDrive()
{
    if (m_cdio)
    {
        cdio_destroy(m_cdio);
        m_cdio = nullptr;
    }
}

bool LibcdioCdDrive::openDevice(const QString& devicePath)
{
    m_devicePath = devicePath;

    m_cdio = cdio_open(devicePath.toLocal8Bit().constData(), DRIVER_DEVICE);
    if (!m_cdio)
    {
        qCWarning(mediaCd) << "LibcdioCdDrive: failed to open device" << devicePath;
        return false;
    }

    qCInfo(mediaCd) << "LibcdioCdDrive: opened device" << devicePath;
    return true;
}

QVector<TocEntry> LibcdioCdDrive::readToc()
{
    QVector<TocEntry> toc;

    if (!m_cdio)
    {
        qCWarning(mediaCd) << "LibcdioCdDrive::readToc: device not open";
        return toc;
    }

    track_t firstTrack = cdio_get_first_track_num(m_cdio);
    track_t numTracks = cdio_get_num_tracks(m_cdio);

    if (firstTrack == CDIO_INVALID_TRACK || numTracks == CDIO_INVALID_TRACK)
    {
        qCWarning(mediaCd) << "LibcdioCdDrive::readToc: could not read track info";
        return toc;
    }

    m_trackCount = static_cast<int>(numTracks);

    for (track_t t = firstTrack; t < firstTrack + numTracks; ++t)
    {
        lba_t startLba = cdio_get_track_lba(m_cdio, t);
        lba_t endLba;

        if (t < firstTrack + numTracks - 1)
        {
            endLba = cdio_get_track_lba(m_cdio, t + 1) - 1;
        }
        else
        {
            endLba = cdio_get_track_lba(m_cdio, CDIO_CDROM_LEADOUT_TRACK) - 1;
        }

        int sectors = endLba - startLba + 1;
        int durationSeconds = sectors / 75; // 75 sectors per second for CD audio

        TocEntry entry;
        entry.trackNumber = static_cast<int>(t);
        entry.startSector = static_cast<int>(startLba);
        entry.endSector = static_cast<int>(endLba);
        entry.durationSeconds = durationSeconds;

        toc.append(entry);
    }

    qCInfo(mediaCd) << "LibcdioCdDrive::readToc:" << toc.size() << "tracks";
    return toc;
}

QString LibcdioCdDrive::getDiscId()
{
    if (m_devicePath.isEmpty())
    {
        qCWarning(mediaCd) << "LibcdioCdDrive::getDiscId: no device path set";
        return {};
    }

    DiscId* disc = discid_new();
    if (!disc)
    {
        qCWarning(mediaCd) << "LibcdioCdDrive::getDiscId: discid_new failed";
        return {};
    }

    QString result;

    if (discid_read(disc, m_devicePath.toLocal8Bit().constData()))
    {
        result = QString::fromUtf8(discid_get_id(disc));
        qCInfo(mediaCd) << "LibcdioCdDrive::getDiscId:" << result;
    }
    else
    {
        qCWarning(mediaCd) << "LibcdioCdDrive::getDiscId: discid_read failed:" << discid_get_error_msg(disc);
    }

    discid_free(disc);
    return result;
}

bool LibcdioCdDrive::eject()
{
    qCInfo(mediaCd) << "LibcdioCdDrive: ejecting";
    if (m_cdio)
    {
        // cdio_eject_media NULLs the handle
        driver_return_code_t rc = cdio_eject_media(&m_cdio);
        m_cdio = nullptr;
        return rc == DRIVER_OP_SUCCESS;
    }
    return false;
}

bool LibcdioCdDrive::stopSpindle()
{
    qCInfo(mediaCd) << "LibcdioCdDrive: stopping spindle";

    if (m_cdio)
    {
        // Use cdio_set_speed(0) to spin down the drive
        driver_return_code_t rc = cdio_set_speed(m_cdio, 0);
        if (rc == DRIVER_OP_SUCCESS)
        {
            return true;
        }
        qCWarning(mediaCd) << "LibcdioCdDrive::stopSpindle: cdio_set_speed failed, trying ioctl";
    }

#ifdef __linux__
    // Fallback: use START STOP UNIT via ioctl
    int fd = ::open(m_devicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        qCWarning(mediaCd) << "LibcdioCdDrive::stopSpindle: failed to open device fd";
        return false;
    }

    // CDROMSTOP stops the motor
    int result = ::ioctl(fd, CDROMSTOP);
    ::close(fd);
    return result == 0;
#else
    return false;
#endif
}

bool LibcdioCdDrive::isDiscPresent() const
{
#ifdef __linux__
    int fd = ::open(m_devicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        return false;
    }

    int status = ::ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
    ::close(fd);
    return status == CDS_DISC_OK;
#else
    return false;
#endif
}

bool LibcdioCdDrive::isAudioDisc() const
{
#ifdef __linux__
    int fd = ::open(m_devicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        return false;
    }

    int discType = ::ioctl(fd, CDROM_DISC_STATUS);
    ::close(fd);
    return discType == CDS_AUDIO || discType == CDS_MIXED;
#else
    return false;
#endif
}

int LibcdioCdDrive::trackCount() const
{
    return m_trackCount;
}
