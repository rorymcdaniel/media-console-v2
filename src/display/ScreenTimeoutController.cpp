#include "display/ScreenTimeoutController.h"

ScreenTimeoutController::ScreenTimeoutController(IDisplayControl* displayControl, PlaybackState* playbackState,
                                                 UIState* uiState, const DisplayConfig& config, QObject* parent)
    : QObject(parent)
    , m_displayControl(displayControl)
    , m_playbackState(playbackState)
    , m_uiState(uiState)
{
    Q_UNUSED(config)
    // Stub - will be implemented in GREEN phase
}

void ScreenTimeoutController::activityDetected()
{
    // Stub
}

void ScreenTimeoutController::start()
{
    // Stub
}

void ScreenTimeoutController::stop()
{
    // Stub
}

void ScreenTimeoutController::onDimTimeout()
{
    // Stub
}

void ScreenTimeoutController::onOffTimeout()
{
    // Stub
}

void ScreenTimeoutController::onDimStep()
{
    // Stub
}

void ScreenTimeoutController::onPlaybackModeChanged(PlaybackMode mode)
{
    Q_UNUSED(mode)
    // Stub
}

void ScreenTimeoutController::onDoorOpenChanged(bool open)
{
    Q_UNUSED(open)
    // Stub
}

void ScreenTimeoutController::setState(ScreenState newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    emit stateChanged(m_state);
}
