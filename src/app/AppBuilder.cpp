#include "AppBuilder.h"

#include "platform/IAudioOutput.h"
#include "platform/ICdDrive.h"
#include "platform/IDisplayControl.h"
#include "platform/IGpioMonitor.h"
#include "platform/PlatformFactory.h"
#include "utils/Logging.h"

AppBuilder::AppBuilder(QObject* parent)
    : QObject(parent)
{
}

AppBuilder::~AppBuilder() = default;

AppContext AppBuilder::build(const AppConfig& config)
{
    // Initialize logging first
    initLogging(config.logging.filterRules);

    qCInfo(mediaApp) << "AppBuilder: constructing object graph";

    // Create platform implementations via factory
    m_audioOutput = PlatformFactory::createAudioOutput();
    m_cdDrive = PlatformFactory::createCdDrive();
    m_gpioMonitor = PlatformFactory::createGpioMonitor(this);
    m_displayControl = PlatformFactory::createDisplayControl(this);

    qCInfo(mediaApp) << "AppBuilder: platform:" << (PlatformFactory::isLinux() ? "Linux (real)" : "non-Linux (stubs)");

    // Build context with non-owning pointers
    AppContext ctx;
    ctx.audioOutput = m_audioOutput.get();
    ctx.cdDrive = m_cdDrive.get();
    ctx.gpioMonitor = m_gpioMonitor.get();
    ctx.displayControl = m_displayControl.get();

    qCInfo(mediaApp) << "AppBuilder: object graph complete";
    return ctx;
}
