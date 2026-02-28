#pragma once

#include <QString>
#include <QVector>

struct TocEntry
{
    int trackNumber = 0;
    int startSector = 0;
    int endSector = 0;
    int durationSeconds = 0;
};

class ICdDrive
{
public:
    virtual ~ICdDrive() = default;

    virtual bool openDevice(const QString& devicePath) = 0;
    virtual QVector<TocEntry> readToc() = 0;
    virtual QString getDiscId() = 0;
    virtual bool eject() = 0;
    virtual bool stopSpindle() = 0;
    virtual bool isDiscPresent() const = 0;
    virtual bool isAudioDisc() const = 0;
    virtual int trackCount() const = 0;
};
