#include "StubCdDrive.h"

#include "utils/Logging.h"

bool StubCdDrive::openDevice(const QString& devicePath)
{
    qCInfo(mediaCd) << "StubCdDrive: openDevice" << devicePath;
    return true;
}

QVector<TocEntry> StubCdDrive::readToc()
{
    // No disc in stub -- return empty
    return {};
}

QString StubCdDrive::getDiscId()
{
    // No disc in stub
    return {};
}

bool StubCdDrive::eject()
{
    qCInfo(mediaCd) << "StubCdDrive: eject";
    m_discPresent = false;
    return true;
}

bool StubCdDrive::stopSpindle()
{
    qCInfo(mediaCd) << "StubCdDrive: stopSpindle";
    return true;
}

bool StubCdDrive::isDiscPresent() const
{
    return m_discPresent;
}

bool StubCdDrive::isAudioDisc() const
{
    return m_discPresent;
}

int StubCdDrive::trackCount() const
{
    return 0;
}

void StubCdDrive::setDiscPresent(bool present)
{
    m_discPresent = present;
}
