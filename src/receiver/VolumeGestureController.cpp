#include "receiver/VolumeGestureController.h"

#include "state/ReceiverState.h"
#include "state/UIState.h"
#include "utils/Logging.h"

VolumeGestureController::VolumeGestureController(ReceiverState* receiverState, UIState* uiState, QObject* parent)
    : QObject(parent)
    , m_receiverState(receiverState)
    , m_uiState(uiState)
{
    m_gestureTimer.setSingleShot(true);
    m_gestureTimer.setInterval(kGestureTimeoutMs);
    connect(&m_gestureTimer, &QTimer::timeout, this, &VolumeGestureController::onGestureTimeout);
}

void VolumeGestureController::onEncoderTick(int delta)
{
    m_gestureActive = true;
    m_gestureTimer.start();

    int newVolume = qBound(0, m_receiverState->volume() + delta, 200);
    m_receiverState->setVolume(newVolume);
    m_uiState->setVolumeOverlayVisible(true);

    qCDebug(mediaReceiver) << "Encoder tick:" << delta << "-> volume:" << newVolume;
}

void VolumeGestureController::onExternalVolumeUpdate(int volume)
{
    if (m_gestureActive)
    {
        // Gesture has priority — suppress external updates during active gesture
        qCDebug(mediaReceiver) << "External volume update suppressed during gesture:" << volume;
        return;
    }

    // Reconciliation: snap to receiver-reported value
    m_receiverState->setVolume(volume);
    // Do NOT show volume overlay for external changes
    qCDebug(mediaReceiver) << "External volume update:" << volume;
}

bool VolumeGestureController::isGestureActive() const
{
    return m_gestureActive;
}

void VolumeGestureController::onGestureTimeout()
{
    m_gestureActive = false;
    int finalVolume = m_receiverState->volume();
    emit gestureEnded(finalVolume);

    qCInfo(mediaReceiver) << "Gesture ended at volume:" << finalVolume;
}
