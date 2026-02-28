#pragma once

#include <QObject>
#include <QTimer>

class ReceiverState;
class UIState;

class VolumeGestureController : public QObject
{
    Q_OBJECT

public:
    VolumeGestureController(ReceiverState* receiverState, UIState* uiState, QObject* parent = nullptr);

    /// Called when encoder generates a tick. Delta is +1 or -1 (or larger for fast rotation).
    /// Updates ReceiverState.volume optimistically. Shows volume overlay.
    void onEncoderTick(int delta);

    /// Called by ReceiverController when a volume update arrives from the receiver (poll or unsolicited).
    /// During active gesture: suppressed (gesture has priority).
    /// After gesture: updates ReceiverState.volume (reconciliation snap).
    void onExternalVolumeUpdate(int volume);

    /// Returns true if a gesture is currently in progress.
    bool isGestureActive() const;

signals:
    /// Emitted when gesture ends (timeout expired). Carries the final volume value.
    /// ReceiverController should send the MVL command with this value.
    void gestureEnded(int finalVolume);

private slots:
    void onGestureTimeout();

private:
    ReceiverState* m_receiverState;
    UIState* m_uiState;
    QTimer m_gestureTimer;
    bool m_gestureActive = false;

    static constexpr int kGestureTimeoutMs = 300;
};
