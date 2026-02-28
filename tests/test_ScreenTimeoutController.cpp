#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include "display/ScreenTimeoutController.h"
#include "platform/stubs/StubDisplayControl.h"
#include "state/PlaybackState.h"
#include "state/UIState.h"

class ScreenTimeoutControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            static int argc = 1;
            static char appName[] = "test";
            static char* argv[] = { appName, nullptr };
            new QCoreApplication(argc, argv);
        }

        m_displayControl = new StubDisplayControl();
        m_playbackState = new PlaybackState();
        m_uiState = new UIState();

        // Use short timeouts for testing
        m_config.dimTimeoutSeconds = 0; // 100ms (will be set in ms)
        m_config.offTimeoutSeconds = 0; // 200ms (will be set in ms)
        m_config.dimBrightness = 25;
        m_config.timeoutEnabled = true;
    }

    void TearDown() override
    {
        delete m_displayControl;
        delete m_playbackState;
        delete m_uiState;
    }

    DisplayConfig makeTestConfig(int dimMs = 100, int offMs = 300, int dimBrightness = 25, bool enabled = true)
    {
        DisplayConfig config;
        // Convert ms to seconds for config (controller will convert back)
        // Actually, we'll use dimTimeoutSeconds directly and controller multiplies by 1000
        // For fast tests, we'll set very small values and rely on milliseconds
        config.dimTimeoutSeconds = dimMs; // We'll treat these as ms in test config
        config.offTimeoutSeconds = offMs;
        config.dimBrightness = dimBrightness;
        config.timeoutEnabled = enabled;
        return config;
    }

    StubDisplayControl* m_displayControl = nullptr;
    PlaybackState* m_playbackState = nullptr;
    UIState* m_uiState = nullptr;
    DisplayConfig m_config;
};

TEST_F(ScreenTimeoutControllerTest, InitialStateIsActive)
{
    auto config = makeTestConfig();
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, StartBeginsTimerWhenEnabled)
{
    auto config = makeTestConfig(100, 300);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, DimTimeoutTransitionsToDimmingOrDimmed)
{
    auto config = makeTestConfig(50, 500);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);

    QSignalSpy spy(&controller, &ScreenTimeoutController::stateChanged);
    controller.start();

    // Wait for dim timeout + dimming animation to complete
    // Dimming takes ~20 steps * 50ms = 1000ms, but with 50ms dim timeout
    QTest::qWait(2000);

    // Should have transitioned away from Active
    EXPECT_NE(controller.state(), ScreenState::Active);
    // Should be either Dimming or Dimmed
    EXPECT_TRUE(controller.state() == ScreenState::Dimming || controller.state() == ScreenState::Dimmed);
}

TEST_F(ScreenTimeoutControllerTest, ActivityDetectedResetsToActive)
{
    auto config = makeTestConfig(50, 500);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for dim timeout
    QTest::qWait(2000);

    // Should be dimmed
    EXPECT_NE(controller.state(), ScreenState::Active);

    // Activity detected should reset to active
    controller.activityDetected();
    EXPECT_EQ(controller.state(), ScreenState::Active);
    EXPECT_EQ(m_displayControl->brightness(), 100);
}

TEST_F(ScreenTimeoutControllerTest, PlaybackSuppressesTimeout)
{
    auto config = makeTestConfig(50, 200);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);

    // Start playing before starting controller
    m_playbackState->setPlaybackMode(PlaybackMode::Playing);
    controller.start();

    // Wait well past dim timeout
    QTest::qWait(300);

    // Should still be Active because playback is suppressing
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, PlaybackStopRestartsTimers)
{
    auto config = makeTestConfig(50, 500);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);

    m_playbackState->setPlaybackMode(PlaybackMode::Playing);
    controller.start();

    // Wait past dim timeout -- still active because playing
    QTest::qWait(200);
    EXPECT_EQ(controller.state(), ScreenState::Active);

    // Stop playback
    m_playbackState->setPlaybackMode(PlaybackMode::Stopped);

    // Wait for dim timeout
    QTest::qWait(2000);

    // Should have transitioned
    EXPECT_NE(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, DoorOpenTriggersActivity)
{
    auto config = makeTestConfig(50, 500);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for dim
    QTest::qWait(2000);
    EXPECT_NE(controller.state(), ScreenState::Active);

    // Door opens
    m_uiState->setDoorOpen(true);

    // Should reset to active
    EXPECT_EQ(controller.state(), ScreenState::Active);
    EXPECT_EQ(m_displayControl->brightness(), 100);
}

TEST_F(ScreenTimeoutControllerTest, DoorCloseTriggersImmediateDim)
{
    auto config = makeTestConfig(5000, 10000); // Long timeouts -- door close should override
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Should be active
    EXPECT_EQ(controller.state(), ScreenState::Active);

    // Close door
    m_uiState->setDoorOpen(false);

    // Wait for immediate dim sequence
    QTest::qWait(2500);

    // Should be dimmed or off (door close triggers immediate dim->off)
    EXPECT_NE(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, DisabledTimeoutStaysActive)
{
    auto config = makeTestConfig(50, 200, 25, false);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait well past timeout
    QTest::qWait(500);

    // Should still be Active because timeout is disabled
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, OffTimeoutTransitionsToOff)
{
    auto config = makeTestConfig(50, 100);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for both dim and off timeouts + animation
    QTest::qWait(3000);

    // Should be off
    EXPECT_EQ(controller.state(), ScreenState::Off);
    EXPECT_FALSE(m_displayControl->isPowered());
}

TEST_F(ScreenTimeoutControllerTest, ActivityFromOffRestoresPower)
{
    auto config = makeTestConfig(50, 100);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for off
    QTest::qWait(3000);
    EXPECT_EQ(controller.state(), ScreenState::Off);

    // Activity should restore
    controller.activityDetected();
    EXPECT_EQ(controller.state(), ScreenState::Active);
    EXPECT_TRUE(m_displayControl->isPowered());
    EXPECT_EQ(m_displayControl->brightness(), 100);
}

TEST_F(ScreenTimeoutControllerTest, StopCancelsAllTimers)
{
    auto config = makeTestConfig(50, 200);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();
    controller.stop();

    // Wait past timeout
    QTest::qWait(500);

    // Should still be Active because stop was called
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, ScreenDimmedPropertySetOnOff)
{
    auto config = makeTestConfig(50, 100);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    EXPECT_FALSE(m_uiState->screenDimmed());

    // Wait for off
    QTest::qWait(3000);

    EXPECT_TRUE(m_uiState->screenDimmed());
}

TEST_F(ScreenTimeoutControllerTest, ScreenDimmedClearsOnActivity)
{
    auto config = makeTestConfig(50, 100);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for off
    QTest::qWait(3000);
    EXPECT_TRUE(m_uiState->screenDimmed());

    // Activity
    controller.activityDetected();
    EXPECT_FALSE(m_uiState->screenDimmed());
}
