#include "display/LinuxDisplayControl.h"

LinuxDisplayControl::LinuxDisplayControl(QObject* parent)
    : IDisplayControl(parent)
{
}

bool LinuxDisplayControl::autoDetectDisplay()
{
    return false; // stub - tests should fail on detect behavior
}

bool LinuxDisplayControl::setPower(bool on)
{
    Q_UNUSED(on)
    return false;
}

bool LinuxDisplayControl::setBrightness(int percent)
{
    Q_UNUSED(percent)
    return false;
}

int LinuxDisplayControl::brightness() const
{
    return m_brightness;
}

bool LinuxDisplayControl::isPowered() const
{
    return m_powered;
}
