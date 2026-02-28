#pragma once

#include "platform/IDisplayControl.h"

class LinuxDisplayControl : public IDisplayControl
{
    Q_OBJECT

public:
    explicit LinuxDisplayControl(QObject* parent = nullptr);
    ~LinuxDisplayControl() override = default;

    bool autoDetectDisplay() override;
    bool setPower(bool on) override;
    bool setBrightness(int percent) override;
    int brightness() const override;
    bool isPowered() const override;

    int busNumber() const { return m_busNumber; }

    static constexpr int kDdcTimeoutMs = 5000;

private:
    int m_busNumber = -1;
    int m_brightness = 100;
    bool m_powered = true;
};
