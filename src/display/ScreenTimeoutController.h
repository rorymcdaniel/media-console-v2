#pragma once

#include <QObject>
#include <QTimer>

#include "app/AppConfig.h"
#include "state/PlaybackMode.h"

class IDisplayControl;
class PlaybackState;
class UIState;

enum class ScreenState
{
    Active,
    Dimming,
    Dimmed,
    Off
};

class ScreenTimeoutController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int screenState READ screenStateInt NOTIFY stateChanged)

public:
    explicit ScreenTimeoutController(IDisplayControl* displayControl, PlaybackState* playbackState, UIState* uiState,
                                     const DisplayConfig& config, QObject* parent = nullptr);
    ~ScreenTimeoutController() override = default;

    ScreenState state() const { return m_state; }
    int screenStateInt() const { return static_cast<int>(m_state); }

public slots:
    void activityDetected();
    void start();
    void stop();

signals:
    void stateChanged(ScreenState state);

private slots:
    void onDimTimeout();
    void onOffTimeout();
    void onDimStep();
    void onPlaybackModeChanged(PlaybackMode mode);
    void onDoorOpenChanged(bool open);

private:
    void setState(ScreenState newState);

    IDisplayControl* m_displayControl = nullptr;
    PlaybackState* m_playbackState = nullptr;
    UIState* m_uiState = nullptr;

    ScreenState m_state = ScreenState::Active;
    QTimer m_dimTimer;
    QTimer m_offTimer;
    QTimer m_dimStepTimer;

    int m_dimTimeoutMs = 300000; // 5 minutes default
    int m_offTimeoutMs = 1200000; // 20 minutes default
    int m_dimBrightness = 25;
    bool m_timeoutEnabled = true;

    int m_currentStepBrightness = 100;
    int m_targetBrightness = 25;
    int m_brightnessStep = 4;

    bool m_playbackSuppressing = false;
    bool m_doorCloseMode = false; // When true, use short off delay after dimming
};
