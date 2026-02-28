#include "PlatformFactory.h"

#include <QSysInfo>

#include "platform/IAudioOutput.h"
#include "platform/ICdDrive.h"
#include "platform/IDisplayControl.h"
#include "platform/IGpioMonitor.h"
#include "platform/stubs/StubAudioOutput.h"
#include "platform/stubs/StubCdDrive.h"
#include "platform/stubs/StubDisplayControl.h"
#include "platform/stubs/StubGpioMonitor.h"

std::unique_ptr<IAudioOutput> PlatformFactory::createAudioOutput()
{
    // TODO: Phase 4+ add runtime detection:
    // if (isLinux()) return std::make_unique<AlsaAudioOutput>();
    return std::make_unique<StubAudioOutput>();
}

std::unique_ptr<ICdDrive> PlatformFactory::createCdDrive()
{
    // TODO: Phase 5+ add runtime detection:
    // if (isLinux()) return std::make_unique<LibcdioCdDrive>();
    return std::make_unique<StubCdDrive>();
}

std::unique_ptr<IGpioMonitor> PlatformFactory::createGpioMonitor(QObject* parent)
{
    // TODO: Phase 7+ add runtime detection:
    // if (isLinux()) return std::make_unique<LinuxGpioMonitor>(parent);
    return std::make_unique<StubGpioMonitor>(parent);
}

std::unique_ptr<IDisplayControl> PlatformFactory::createDisplayControl(QObject* parent)
{
    // TODO: Phase 9+ add runtime detection:
    // if (isLinux()) return std::make_unique<DdcDisplayControl>(parent);
    return std::make_unique<StubDisplayControl>(parent);
}

bool PlatformFactory::isLinux()
{
    return QSysInfo::kernelType() == "linux";
}
