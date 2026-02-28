#include "PlatformFactory.h"

#include <QSysInfo>

#include "app/AppConfig.h"
#include "platform/IAudioOutput.h"
#include "platform/ICdDrive.h"
#include "platform/IDisplayControl.h"
#include "platform/IGpioMonitor.h"
#include "platform/stubs/StubAudioOutput.h"
#include "platform/stubs/StubCdDrive.h"
#include "platform/stubs/StubDisplayControl.h"
#include "platform/stubs/StubGpioMonitor.h"

#ifdef HAS_ALSA
#include "audio/AlsaAudioOutput.h"
#endif

#ifdef HAS_CDIO
#include "cd/LibcdioCdDrive.h"
#endif

#ifdef HAS_GPIOD
#include "platform/LinuxGpioMonitor.h"
#endif

std::unique_ptr<IAudioOutput> PlatformFactory::createAudioOutput()
{
#ifdef HAS_ALSA
    return std::make_unique<AlsaAudioOutput>();
#else
    return std::make_unique<StubAudioOutput>();
#endif
}

std::unique_ptr<ICdDrive> PlatformFactory::createCdDrive()
{
#ifdef HAS_CDIO
    return std::make_unique<LibcdioCdDrive>();
#else
    return std::make_unique<StubCdDrive>();
#endif
}

std::unique_ptr<IGpioMonitor> PlatformFactory::createGpioMonitor(const GpioConfig& config, QObject* parent)
{
#ifdef HAS_GPIOD
    return std::make_unique<LinuxGpioMonitor>(config, parent);
#else
    Q_UNUSED(config)
    return std::make_unique<StubGpioMonitor>(parent);
#endif
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
