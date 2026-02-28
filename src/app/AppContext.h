#pragma once

class IAudioOutput;
class ICdDrive;
class IGpioMonitor;
class IDisplayControl;
class ReceiverState;
class PlaybackState;
class UIState;
class ReceiverController;
class VolumeGestureController;
class LocalPlaybackController;
class CdController;
class FlacLibraryController;
class SpotifyController;

struct AppContext
{
    // Non-owning pointers -- AppBuilder manages lifetime via QObject parent chain
    // and unique_ptr ownership. These are convenience pointers for consumers.
    IAudioOutput* audioOutput = nullptr;
    ICdDrive* cdDrive = nullptr;
    IGpioMonitor* gpioMonitor = nullptr;
    IDisplayControl* displayControl = nullptr;

    // State layer -- thin Q_PROPERTY bags for QML binding
    ReceiverState* receiverState = nullptr;
    PlaybackState* playbackState = nullptr;
    UIState* uiState = nullptr;

    // Receiver control -- eISCP communication and volume gestures
    ReceiverController* receiverController = nullptr;
    VolumeGestureController* volumeGestureController = nullptr;

    // Audio playback -- unified controller for any AudioStream source
    LocalPlaybackController* localPlaybackController = nullptr;

    // CD subsystem -- lifecycle orchestrator for disc detection, metadata, playback
    CdController* cdController = nullptr;

    // FLAC Library -- lifecycle orchestrator for scanning, browsing, playback
    FlacLibraryController* flacLibraryController = nullptr;

    // Spotify -- business logic orchestrator for Spotify integration
    SpotifyController* spotifyController = nullptr;
};
