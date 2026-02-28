#pragma once

#include "platform/ICdDrive.h"

class StubCdDrive : public ICdDrive
{
public:
    StubCdDrive() = default;
    ~StubCdDrive() override = default;

    bool openDevice(const QString& devicePath) override;
    QVector<TocEntry> readToc() override;
    QString getDiscId() override;
    bool eject() override;
    bool stopSpindle() override;
    bool isDiscPresent() const override;
    bool isAudioDisc() const override;
    int trackCount() const override;

    // Programmatic control for testing
    void setDiscPresent(bool present);

private:
    bool m_discPresent = false;
};
