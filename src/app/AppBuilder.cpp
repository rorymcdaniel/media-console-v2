#include "AppBuilder.h"

#include "platform/IAudioOutput.h"
#include "platform/ICdDrive.h"
#include "platform/IDisplayControl.h"
#include "platform/IGpioMonitor.h"
#include "platform/PlatformFactory.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"
#include "state/UIState.h"
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

    // Create state layer — thin Q_PROPERTY bags for QML binding
    m_receiverState = std::make_unique<ReceiverState>(this);
    m_playbackState = std::make_unique<PlaybackState>(this);
    m_uiState = std::make_unique<UIState>(this);

    qCInfo(mediaApp) << "AppBuilder: state layer initialized";

    // Build context with non-owning pointers
    AppContext ctx;
    ctx.audioOutput = m_audioOutput.get();
    ctx.cdDrive = m_cdDrive.get();
    ctx.gpioMonitor = m_gpioMonitor.get();
    ctx.displayControl = m_displayControl.get();
    ctx.receiverState = m_receiverState.get();
    ctx.playbackState = m_playbackState.get();
    ctx.uiState = m_uiState.get();

    qCInfo(mediaApp) << "AppBuilder: object graph complete";
    return ctx;
}
