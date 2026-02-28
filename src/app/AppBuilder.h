#pragma once

#include <QObject>

#include <memory>

#include "app/AppConfig.h"
#include "app/AppContext.h"

class IAudioOutput;
class ICdDrive;
class IGpioMonitor;
class IDisplayControl;
class ReceiverState;
class PlaybackState;
class UIState;
class EiscpConnection;
class ReceiverController;
class VolumeGestureController;
class LocalPlaybackController;
class CdController;
class FlacLibraryController;

class AppBuilder : public QObject
{
    Q_OBJECT

public:
    explicit AppBuilder(QObject* parent = nullptr);
    ~AppBuilder() override;

    AppContext build(const AppConfig& config);

private:
    std::unique_ptr<IAudioOutput> m_audioOutput;
    std::unique_ptr<ICdDrive> m_cdDrive;
    std::unique_ptr<IGpioMonitor> m_gpioMonitor;
    std::unique_ptr<IDisplayControl> m_displayControl;
    std::unique_ptr<ReceiverState> m_receiverState;
    std::unique_ptr<PlaybackState> m_playbackState;
    std::unique_ptr<UIState> m_uiState;
    std::unique_ptr<EiscpConnection> m_eiscpConnection;
    std::unique_ptr<ReceiverController> m_receiverController;
    std::unique_ptr<VolumeGestureController> m_volumeGestureController;
    std::unique_ptr<LocalPlaybackController> m_localPlaybackController;
    std::unique_ptr<CdController> m_cdController;
#ifdef HAS_SNDFILE
    std::unique_ptr<FlacLibraryController> m_flacLibraryController;
#endif
};
