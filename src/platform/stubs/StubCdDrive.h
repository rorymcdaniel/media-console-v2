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
    void setAudioDisc(bool audio);
    void setToc(const QVector<TocEntry>& toc);
    void setDiscId(const QString& discId);

    // Test observation
    int stopSpindleCallCount() const { return m_stopSpindleCallCount; }
    int ejectCallCount() const { return m_ejectCallCount; }

private:
    bool m_discPresent = false;
    bool m_audioDisc = true;
    QVector<TocEntry> m_toc;
    QString m_discId;
    int m_stopSpindleCallCount = 0;
    int m_ejectCallCount = 0;
};
