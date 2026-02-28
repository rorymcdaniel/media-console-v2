#pragma once

#include "platform/IGpioMonitor.h"

class StubGpioMonitor : public IGpioMonitor
{
    Q_OBJECT

public:
    explicit StubGpioMonitor(QObject* parent = nullptr);
    ~StubGpioMonitor() override = default;

    bool start() override;
    void stop() override;

    // Programmatic control for testing
    void simulateVolumeChange(int delta);
    void simulateMuteToggle();
    void simulateInputNext();
    void simulateInputPrevious();
    void simulateInputSelect();
    void simulateReedSwitch(bool magnetsApart);
};
