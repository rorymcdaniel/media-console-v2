#include "StubCdDrive.h"

#include "utils/Logging.h"

bool StubCdDrive::openDevice(const QString& devicePath)
{
    qCInfo(mediaCd) << "StubCdDrive: openDevice" << devicePath;
    return true;
}

QVector<TocEntry> StubCdDrive::readToc()
{
    return m_toc;
}

QString StubCdDrive::getDiscId()
{
    return m_discId;
}

bool StubCdDrive::eject()
{
    qCInfo(mediaCd) << "StubCdDrive: eject";
    m_ejectCallCount++;
    m_discPresent = false;
    return true;
}

bool StubCdDrive::stopSpindle()
{
    qCInfo(mediaCd) << "StubCdDrive: stopSpindle";
    m_stopSpindleCallCount++;
    return true;
}

bool StubCdDrive::isDiscPresent() const
{
    return m_discPresent;
}

bool StubCdDrive::isAudioDisc() const
{
    return m_discPresent && m_audioDisc;
}

int StubCdDrive::trackCount() const
{
    return m_toc.size();
}

void StubCdDrive::setDiscPresent(bool present)
{
    m_discPresent = present;
}

void StubCdDrive::setAudioDisc(bool audio)
{
    m_audioDisc = audio;
}

void StubCdDrive::setToc(const QVector<TocEntry>& toc)
{
    m_toc = toc;
}

void StubCdDrive::setDiscId(const QString& discId)
{
    m_discId = discId;
}
