#pragma once

class IAudioOutput;
class ICdDrive;
class IGpioMonitor;
class IDisplayControl;

struct AppContext
{
    // Non-owning pointers -- AppBuilder manages lifetime via QObject parent chain
    // and unique_ptr ownership. These are convenience pointers for consumers.
    IAudioOutput* audioOutput = nullptr;
    ICdDrive* cdDrive = nullptr;
    IGpioMonitor* gpioMonitor = nullptr;
    IDisplayControl* displayControl = nullptr;

    // Future phases add:
    // ReceiverState* receiverState
    // PlaybackState* playbackState
    // UIState* uiState
    // Controllers, etc.
};
