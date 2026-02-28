#include "StubDisplayControl.h"

#include "utils/Logging.h"

StubDisplayControl::StubDisplayControl(QObject* parent)
    : IDisplayControl(parent)
{
}

bool StubDisplayControl::autoDetectDisplay()
{
    qCInfo(mediaApp) << "StubDisplayControl: auto-detect display (bus 1)";
    emit displayDetected(1);
    return true;
}

bool StubDisplayControl::setPower(bool on)
{
    m_powered = on;
    qCInfo(mediaApp) << "StubDisplayControl: power" << (on ? "ON" : "OFF");
    emit powerChanged(on);
    return true;
}

bool StubDisplayControl::setBrightness(int percent)
{
    m_brightness = percent;
    qCInfo(mediaApp) << "StubDisplayControl: brightness" << percent << "%";
    emit brightnessChanged(percent);
    return true;
}

int StubDisplayControl::brightness() const
{
    return m_brightness;
}

bool StubDisplayControl::isPowered() const
{
    return m_powered;
}
