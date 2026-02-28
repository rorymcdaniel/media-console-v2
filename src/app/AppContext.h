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
};
