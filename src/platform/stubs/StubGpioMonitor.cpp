#include "StubGpioMonitor.h"

#include "utils/Logging.h"

StubGpioMonitor::StubGpioMonitor(QObject* parent)
    : IGpioMonitor(parent)
{
}

bool StubGpioMonitor::start()
{
    qCInfo(mediaGpio) << "StubGpioMonitor: started";
    return true;
}

void StubGpioMonitor::stop()
{
    qCInfo(mediaGpio) << "StubGpioMonitor: stopped";
}

void StubGpioMonitor::simulateVolumeUp()
{
    emit volumeUp();
}

void StubGpioMonitor::simulateVolumeDown()
{
    emit volumeDown();
}

void StubGpioMonitor::simulateMuteToggle()
{
    emit muteToggled();
}

void StubGpioMonitor::simulateInputNext()
{
    emit inputNext();
}

void StubGpioMonitor::simulateInputPrevious()
{
    emit inputPrevious();
}

void StubGpioMonitor::simulateInputSelect()
{
    emit inputSelect();
}

void StubGpioMonitor::simulateReedSwitch(bool magnetsApart)
{
    emit reedSwitchChanged(magnetsApart);
}
