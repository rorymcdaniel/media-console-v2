#pragma once

#include "platform/ICdDrive.h"

// Forward declarations to avoid libcdio header pollution
typedef struct _CdIo CdIo_t;

/// Real CD drive implementation using libcdio for TOC reading, disc detection,
/// and spindle control. Uses libdiscid for MusicBrainz disc ID calculation.
/// Only compiled on Linux when HAS_CDIO is defined.
class LibcdioCdDrive : public ICdDrive
{
public:
    LibcdioCdDrive();
    ~LibcdioCdDrive() override;

    bool openDevice(const QString& devicePath) override;
    QVector<TocEntry> readToc() override;
    QString getDiscId() override;
    bool eject() override;
    bool stopSpindle() override;
    bool isDiscPresent() const override;
    bool isAudioDisc() const override;
    int trackCount() const override;

private:
    CdIo_t* m_cdio = nullptr;
    QString m_devicePath;
    int m_trackCount = 0;
};
