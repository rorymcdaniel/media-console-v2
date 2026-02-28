#pragma once

#include "platform/IDisplayControl.h"

class StubDisplayControl : public IDisplayControl
{
    Q_OBJECT

public:
    explicit StubDisplayControl(QObject* parent = nullptr);
    ~StubDisplayControl() override = default;

    bool autoDetectDisplay() override;
    bool setPower(bool on) override;
    bool setBrightness(int percent) override;
    int brightness() const override;
    bool isPowered() const override;

private:
    bool m_powered = true;
    int m_brightness = 100;
};
