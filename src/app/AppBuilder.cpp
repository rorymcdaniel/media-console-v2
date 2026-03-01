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

#ifdef HAS_SNDFILE
#include "library/FlacLibraryController.h"
#endif

#include "api/HttpApiServer.h"
#include "display/ScreenTimeoutController.h"
#include "orchestration/AlbumArtResolver.h"
#include "orchestration/PlaybackRouter.h"
#include "spotify/SpotifyAuth.h"
#include "spotify/SpotifyClient.h"
#include "spotify/SpotifyController.h"

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
    m_gpioMonitor = PlatformFactory::createGpioMonitor(config.gpio, this);
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

    // Bug 5 (ORCH-03): ReceiverController volume → VolumeGestureController (gesture-aware reconciliation)
    // Previously: ReceiverController called m_receiverState->setVolume() directly, bypassing gesture suppression.
    // Now: ReceiverController emits volumeReceivedFromReceiver; VolumeGestureController applies update
    // only when no gesture is active, then calls m_receiverState->setVolume().
    connect(m_receiverController.get(), &ReceiverController::volumeReceivedFromReceiver,
            m_volumeGestureController.get(), &VolumeGestureController::onExternalVolumeUpdate);

    // Wire GPIO monitor signals
    // Volume encoder -> VolumeGestureController (pre-scaled delta)
    connect(m_gpioMonitor.get(), &IGpioMonitor::volumeChanged, m_volumeGestureController.get(),
            &VolumeGestureController::onEncoderTick);

    // Input encoder navigation -> receiver input cycling
    connect(m_gpioMonitor.get(), &IGpioMonitor::inputNext, m_receiverController.get(), &ReceiverController::inputNext);
    connect(m_gpioMonitor.get(), &IGpioMonitor::inputPrevious, m_receiverController.get(),
            &ReceiverController::inputPrevious);

    // Push button -> mute toggle (context-dependent routing deferred to Phase 10
    // when InputCarousel exists; for now, push always toggles mute)
    connect(m_gpioMonitor.get(), &IGpioMonitor::inputSelect, m_receiverController.get(),
            &ReceiverController::toggleMute);

    // Reed switch -> UIState.doorOpen (Phase 9 DisplayController reacts to state changes)
    connect(m_gpioMonitor.get(), &IGpioMonitor::reedSwitchChanged, m_uiState.get(), &UIState::setDoorOpen);

    // Start GPIO monitoring (non-fatal on failure)
    if (!m_gpioMonitor->start())
    {
        qCWarning(mediaApp) << "AppBuilder: GPIO monitor failed to start (continuing with no hardware input)";
    }

    // Start receiver connection
    m_receiverController->start(config.receiver.host, config.receiver.port);

    qCInfo(mediaApp) << "AppBuilder: receiver and GPIO control initialized";

    // Create audio playback controller
    m_localPlaybackController = std::make_unique<LocalPlaybackController>(m_audioOutput.get(), m_playbackState.get(),
                                                                          config.audio.deviceName, this);

    qCInfo(mediaApp) << "AppBuilder: audio playback controller initialized";

    // Bug 2 (AUDIO-06): ALSA recovery failure → audio error dialog
    // LocalPlaybackController emits audioRecoveryFailed after exhausting all ALSA EIO retries.
    // UIState.setAudioError() sets UIState.audioError property, which triggers the AudioErrorDialog
    // in main.qml. The dialog is already wired (visible: UIState.audioError !== "").
    connect(m_localPlaybackController.get(), &LocalPlaybackController::audioRecoveryFailed, m_uiState.get(), [this]()
            { m_uiState->setAudioError(QStringLiteral("Audio Error encountered, please restart the application")); });

    // Create CD subsystem controller
    m_cdController = std::make_unique<CdController>(m_cdDrive.get(), m_localPlaybackController.get(),
                                                    m_playbackState.get(), config.cd, this);
    m_cdController->start();

    qCInfo(mediaApp) << "AppBuilder: CD controller initialized";

    // Create FLAC library controller (Linux only with libsndfile/taglib)
#ifdef HAS_SNDFILE
    m_flacLibraryController = std::make_unique<FlacLibraryController>(m_localPlaybackController.get(),
                                                                      m_playbackState.get(), config.library, this);
    m_flacLibraryController->start();
    qCInfo(mediaApp) << "AppBuilder: FLAC library controller initialized";
#else
    qCInfo(mediaApp) << "AppBuilder: FLAC library disabled (no libsndfile)";
#endif

    // Create Spotify integration
    m_spotifyAuth = std::make_unique<SpotifyAuth>(config.spotify, this);
    m_spotifyAuth->restoreTokens(); // Attempt to restore saved tokens

    m_spotifyClient = std::make_unique<SpotifyClient>(this);

    m_spotifyController = std::make_unique<SpotifyController>(
        m_spotifyAuth.get(), m_spotifyClient.get(), m_playbackState.get(), m_uiState.get(), config.spotify, this);

    qCInfo(mediaApp) << "AppBuilder: Spotify controller initialized"
                     << (m_spotifyAuth->isAuthenticated() ? "(authenticated)" : "(not authenticated)");

    // Phase 9: Display control, HTTP API, orchestration

    // Create ScreenTimeoutController
    m_screenTimeoutController = std::make_unique<ScreenTimeoutController>(m_displayControl.get(), m_playbackState.get(),
                                                                          m_uiState.get(), config.display, this);

    // Connect GPIO activity sources to ScreenTimeoutController
    connect(m_gpioMonitor.get(), &IGpioMonitor::volumeChanged, m_screenTimeoutController.get(),
            &ScreenTimeoutController::activityDetected);
    connect(m_gpioMonitor.get(), &IGpioMonitor::inputNext, m_screenTimeoutController.get(),
            &ScreenTimeoutController::activityDetected);
    connect(m_gpioMonitor.get(), &IGpioMonitor::inputPrevious, m_screenTimeoutController.get(),
            &ScreenTimeoutController::activityDetected);
    connect(m_gpioMonitor.get(), &IGpioMonitor::inputSelect, m_screenTimeoutController.get(),
            &ScreenTimeoutController::activityDetected);

    qCInfo(mediaApp) << "AppBuilder: ScreenTimeoutController initialized";

    // Create PlaybackRouter
    m_playbackRouter
        = std::make_unique<PlaybackRouter>(m_playbackState.get(), m_receiverController.get(), m_cdController.get(),
#ifdef HAS_SNDFILE
                                           m_flacLibraryController.get(),
#else
                                           nullptr,
#endif
                                           m_spotifyController.get(), m_localPlaybackController.get(), this);

    qCInfo(mediaApp) << "AppBuilder: PlaybackRouter initialized";

    // Create AlbumArtResolver
    m_albumArtResolver = std::make_unique<AlbumArtResolver>(m_playbackState.get(), m_receiverState.get(), this);

    qCInfo(mediaApp) << "AppBuilder: AlbumArtResolver initialized";

    // Create HttpApiServer
    m_httpApiServer = std::make_unique<HttpApiServer>(m_receiverController.get(), m_receiverState.get(),
                                                      m_playbackState.get(), m_displayControl.get(),
                                                      m_spotifyAuth.get(), m_spotifyController.get(), config.api, this);

    // Connect HTTP API activity to ScreenTimeoutController
    connect(m_httpApiServer.get(), &HttpApiServer::activityDetected, m_screenTimeoutController.get(),
            &ScreenTimeoutController::activityDetected);

    // Start services
    m_screenTimeoutController->start();

    if (!m_httpApiServer->start())
    {
        qCWarning(mediaApp) << "AppBuilder: HTTP API server failed to start";
    }

    qCInfo(mediaApp) << "AppBuilder: Phase 9 services started";

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
#ifdef HAS_SNDFILE
    ctx.flacLibraryController = m_flacLibraryController.get();
#endif
    ctx.spotifyController = m_spotifyController.get();
    ctx.screenTimeoutController = m_screenTimeoutController.get();
    ctx.httpApiServer = m_httpApiServer.get();
    ctx.playbackRouter = m_playbackRouter.get();
    ctx.albumArtResolver = m_albumArtResolver.get();

    qCInfo(mediaApp) << "AppBuilder: object graph complete";
    return ctx;
}
