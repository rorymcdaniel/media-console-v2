#include "AppBuilder.h"

#include "audio/LocalPlaybackController.h"
#include "cd/CdController.h"
#include "platform/IAudioOutput.h"
#include "platform/ICdDrive.h"
#include "platform/IDisplayControl.h"
#include "platform/IGpioMonitor.h"
#include "platform/PlatformFactory.h"
#include "receiver/EiscpConnection.h"
#include "receiver/ReceiverController.h"
#include "receiver/VolumeGestureController.h"
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

    // Create receiver control stack
    m_eiscpConnection = std::make_unique<EiscpConnection>(this);
    m_receiverController = std::make_unique<ReceiverController>(m_eiscpConnection.get(), m_receiverState.get(),
                                                                m_playbackState.get(), m_uiState.get(), this);
    m_volumeGestureController = std::make_unique<VolumeGestureController>(m_receiverState.get(), m_uiState.get(), this);

    // Wire gesture controller: gestureEnded -> setVolume on receiver
    connect(m_volumeGestureController.get(), &VolumeGestureController::gestureEnded, m_receiverController.get(),
            &ReceiverController::setVolume);

    // Start receiver connection
    m_receiverController->start(config.receiver.host, config.receiver.port);

    qCInfo(mediaApp) << "AppBuilder: receiver control initialized";

    // Create audio playback controller
    m_localPlaybackController = std::make_unique<LocalPlaybackController>(m_audioOutput.get(), m_playbackState.get(),
                                                                          config.audio.deviceName, this);

    qCInfo(mediaApp) << "AppBuilder: audio playback controller initialized";

    // Create CD subsystem controller
    m_cdController = std::make_unique<CdController>(m_cdDrive.get(), m_localPlaybackController.get(),
                                                    m_playbackState.get(), config.cd, this);
    m_cdController->start();

    qCInfo(mediaApp) << "AppBuilder: CD controller initialized";

    // Build context with non-owning pointers
    AppContext ctx;
    ctx.audioOutput = m_audioOutput.get();
    ctx.cdDrive = m_cdDrive.get();
    ctx.gpioMonitor = m_gpioMonitor.get();
    ctx.displayControl = m_displayControl.get();
    ctx.receiverState = m_receiverState.get();
    ctx.playbackState = m_playbackState.get();
    ctx.uiState = m_uiState.get();
    ctx.receiverController = m_receiverController.get();
    ctx.volumeGestureController = m_volumeGestureController.get();
    ctx.localPlaybackController = m_localPlaybackController.get();
    ctx.cdController = m_cdController.get();

    qCInfo(mediaApp) << "AppBuilder: object graph complete";
    return ctx;
}
