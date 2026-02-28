#include "display/ScreenTimeoutController.h"

#include "platform/IDisplayControl.h"
#include "state/PlaybackState.h"
#include "state/UIState.h"
#include "utils/Logging.h"

// Number of steps for smooth dimming animation
static constexpr int kDimSteps = 20;
// Interval between dimming steps in ms
static constexpr int kDimStepIntervalMs = 50;
// Delay before turning off after door close dimming completes
static constexpr int kDoorCloseOffDelayMs = 2000;

ScreenTimeoutController::ScreenTimeoutController(IDisplayControl* displayControl, PlaybackState* playbackState,
                                                 UIState* uiState, const DisplayConfig& config, QObject* parent)
    : QObject(parent)
    , m_displayControl(displayControl)
    , m_playbackState(playbackState)
    , m_uiState(uiState)
    , m_dimTimeoutMs(config.dimTimeoutSeconds * 1000)
    , m_offTimeoutMs(config.offTimeoutSeconds * 1000)
    , m_dimBrightness(config.dimBrightness)
    , m_timeoutEnabled(config.timeoutEnabled)
{
    // Single-shot timers for dim and off timeouts
    m_dimTimer.setSingleShot(true);
    m_offTimer.setSingleShot(true);

    // Repeating timer for smooth dimming steps
    m_dimStepTimer.setSingleShot(false);
    m_dimStepTimer.setInterval(kDimStepIntervalMs);

    // Connect timer signals
    connect(&m_dimTimer, &QTimer::timeout, this, &ScreenTimeoutController::onDimTimeout);
    connect(&m_offTimer, &QTimer::timeout, this, &ScreenTimeoutController::onOffTimeout);
    connect(&m_dimStepTimer, &QTimer::timeout, this, &ScreenTimeoutController::onDimStep);

    // Connect to PlaybackState
    if (m_playbackState)
    {
        connect(m_playbackState, &PlaybackState::playbackModeChanged, this,
                &ScreenTimeoutController::onPlaybackModeChanged);
    }

    // Connect to UIState (door sensor)
    if (m_uiState)
    {
        connect(m_uiState, &UIState::doorOpenChanged, this, &ScreenTimeoutController::onDoorOpenChanged);
    }
}

void ScreenTimeoutController::activityDetected()
{
    if (!m_timeoutEnabled)
        return;

    qCDebug(mediaApp) << "ScreenTimeoutController: activity detected, state =" << static_cast<int>(m_state);

    // Stop any in-progress dimming animation
    m_dimStepTimer.stop();
    m_offTimer.stop();
    m_doorCloseMode = false;

    if (m_state != ScreenState::Active)
    {
        // Restore full brightness and power
        if (m_displayControl)
        {
            m_displayControl->setPower(true);
            m_displayControl->setBrightness(100);
        }
        m_currentStepBrightness = 100;

        if (m_uiState)
        {
            m_uiState->setScreenDimmed(false);
        }

        setState(ScreenState::Active);
    }

    // Restart dim timer (even if already Active, reset the countdown)
    if (!m_playbackSuppressing)
    {
        m_dimTimer.start(m_dimTimeoutMs);
    }
}

void ScreenTimeoutController::start()
{
    if (!m_timeoutEnabled)
    {
        qCInfo(mediaApp) << "ScreenTimeoutController: timeout disabled, staying active";
        return;
    }

    setState(ScreenState::Active);
    m_currentStepBrightness = 100;

    // Check if playback is already suppressing
    if (m_playbackState && m_playbackState->playbackMode() == PlaybackMode::Playing)
    {
        m_playbackSuppressing = true;
        qCInfo(mediaApp) << "ScreenTimeoutController: started, playback active - timers suppressed";
        return;
    }

    m_playbackSuppressing = false;
    m_dimTimer.start(m_dimTimeoutMs);
    qCInfo(mediaApp) << "ScreenTimeoutController: started, dim timeout =" << m_dimTimeoutMs << "ms";
}

void ScreenTimeoutController::stop()
{
    m_dimTimer.stop();
    m_offTimer.stop();
    m_dimStepTimer.stop();
    qCInfo(mediaApp) << "ScreenTimeoutController: stopped";
}

void ScreenTimeoutController::onDimTimeout()
{
    if (m_playbackSuppressing)
        return;

    qCInfo(mediaApp) << "ScreenTimeoutController: dim timeout expired, beginning dimming animation";

    setState(ScreenState::Dimming);
    m_currentStepBrightness = 100;
    m_targetBrightness = m_dimBrightness;

    // Calculate step size: (100 - dimBrightness) / kDimSteps
    int range = 100 - m_dimBrightness;
    m_brightnessStep = qMax(1, range / kDimSteps);

    m_dimStepTimer.start();
}

void ScreenTimeoutController::onOffTimeout()
{
    if (m_playbackSuppressing)
        return;

    qCInfo(mediaApp) << "ScreenTimeoutController: off timeout expired, powering off display";

    if (m_displayControl)
    {
        m_displayControl->setPower(false);
    }

    if (m_uiState)
    {
        m_uiState->setScreenDimmed(true);
    }

    setState(ScreenState::Off);
}

void ScreenTimeoutController::onDimStep()
{
    m_currentStepBrightness -= m_brightnessStep;

    if (m_currentStepBrightness <= m_targetBrightness)
    {
        // Reached target brightness
        m_currentStepBrightness = m_targetBrightness;
        m_dimStepTimer.stop();

        if (m_displayControl)
        {
            m_displayControl->setBrightness(m_currentStepBrightness);
        }

        setState(ScreenState::Dimmed);

        // Start off timer
        int offDelayMs;
        if (m_doorCloseMode)
        {
            // Door close: short delay after dimming completes
            offDelayMs = kDoorCloseOffDelayMs;
            m_doorCloseMode = false;
        }
        else
        {
            // Normal: remaining time = off timeout - dim timeout
            offDelayMs = m_offTimeoutMs - m_dimTimeoutMs;
        }

        if (offDelayMs <= 0)
        {
            // If off timeout <= dim timeout, turn off immediately
            onOffTimeout();
        }
        else
        {
            m_offTimer.start(offDelayMs);
        }

        qCInfo(mediaApp) << "ScreenTimeoutController: dimming complete, brightness =" << m_currentStepBrightness
                         << "%, off timer =" << offDelayMs << "ms";
        return;
    }

    if (m_displayControl)
    {
        m_displayControl->setBrightness(m_currentStepBrightness);
    }
}

void ScreenTimeoutController::onPlaybackModeChanged(PlaybackMode mode)
{
    if (mode == PlaybackMode::Playing)
    {
        // Suppress all timers - screen stays on during playback
        m_playbackSuppressing = true;
        m_dimTimer.stop();
        m_offTimer.stop();
        m_dimStepTimer.stop();
        qCInfo(mediaApp) << "ScreenTimeoutController: playback started, suppressing timers";
    }
    else
    {
        if (m_playbackSuppressing)
        {
            m_playbackSuppressing = false;
            qCInfo(mediaApp) << "ScreenTimeoutController: playback stopped, restarting timers";

            // Restart timers from current state
            if (m_timeoutEnabled && m_state == ScreenState::Active)
            {
                m_dimTimer.start(m_dimTimeoutMs);
            }
        }
    }
}

void ScreenTimeoutController::onDoorOpenChanged(bool open)
{
    if (open)
    {
        // Door opened - someone is present, wake up
        qCInfo(mediaApp) << "ScreenTimeoutController: door opened, triggering activity";
        activityDetected();
    }
    else
    {
        // Door closed - user is leaving, immediate dim then off
        qCInfo(mediaApp) << "ScreenTimeoutController: door closed, immediate dim sequence";

        if (!m_timeoutEnabled)
            return;

        // Stop normal timers
        m_dimTimer.stop();
        m_offTimer.stop();

        // Start immediate dimming
        setState(ScreenState::Dimming);
        m_currentStepBrightness = 100;
        m_targetBrightness = m_dimBrightness;

        int range = 100 - m_dimBrightness;
        m_brightnessStep = qMax(1, range / kDimSteps);

        // Set door close mode: onDimStep will use short off delay
        m_doorCloseMode = true;
        m_dimStepTimer.start();
    }
}

void ScreenTimeoutController::setState(ScreenState newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    emit stateChanged(m_state);
}
