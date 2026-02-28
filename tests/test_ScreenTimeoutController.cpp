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
    }

    void TearDown() override
    {
        delete m_displayControl;
        delete m_playbackState;
        delete m_uiState;
    }

    /// Create a DisplayConfig with short timeouts for testing.
    /// Values are in seconds (controller multiplies by 1000 for ms).
    /// Minimum meaningful test value is 1 second.
    DisplayConfig makeTestConfig(int dimSec = 1, int offSec = 2, int dimBrightness = 25, bool enabled = true)
    {
        DisplayConfig config;
        config.dimTimeoutSeconds = dimSec;
        config.offTimeoutSeconds = offSec;
        config.dimBrightness = dimBrightness;
        config.timeoutEnabled = enabled;
        return config;
    }

    StubDisplayControl* m_displayControl = nullptr;
    PlaybackState* m_playbackState = nullptr;
    UIState* m_uiState = nullptr;
};

TEST_F(ScreenTimeoutControllerTest, InitialStateIsActive)
{
    auto config = makeTestConfig();
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, StartBeginsTimerWhenEnabled)
{
    auto config = makeTestConfig();
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, DimTimeoutTransitionsToDimmingOrDimmed)
{
    auto config = makeTestConfig(1, 5); // 1s dim, 5s off
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);

    QSignalSpy spy(&controller, &ScreenTimeoutController::stateChanged);
    controller.start();

    // Wait for dim timeout (1s) + dimming animation (~1s) + margin
    QTest::qWait(2500);

    // Should have transitioned away from Active
    EXPECT_NE(controller.state(), ScreenState::Active);
    EXPECT_TRUE(controller.state() == ScreenState::Dimming || controller.state() == ScreenState::Dimmed);
}

TEST_F(ScreenTimeoutControllerTest, ActivityDetectedResetsToActive)
{
    auto config = makeTestConfig(1, 5); // 1s dim, 5s off
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for dim timeout + animation
    QTest::qWait(2500);
    EXPECT_NE(controller.state(), ScreenState::Active);

    // Activity detected should reset to active
    controller.activityDetected();
    EXPECT_EQ(controller.state(), ScreenState::Active);
    EXPECT_EQ(m_displayControl->brightness(), 100);
}

TEST_F(ScreenTimeoutControllerTest, PlaybackSuppressesTimeout)
{
    auto config = makeTestConfig(1, 3);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);

    // Start playing before starting controller
    m_playbackState->setPlaybackMode(PlaybackMode::Playing);
    controller.start();

    // Wait well past dim timeout (1s * 1000 = 1000ms, wait 2500ms)
    QTest::qWait(2500);

    // Should still be Active because playback is suppressing
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, PlaybackStopRestartsTimers)
{
    auto config = makeTestConfig(1, 5);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);

    m_playbackState->setPlaybackMode(PlaybackMode::Playing);
    controller.start();

    // Wait past dim timeout -- still active because playing
    QTest::qWait(1500);
    EXPECT_EQ(controller.state(), ScreenState::Active);

    // Stop playback
    m_playbackState->setPlaybackMode(PlaybackMode::Stopped);

    // Wait for dim timeout (1s) + animation (~1s)
    QTest::qWait(2500);

    // Should have transitioned
    EXPECT_NE(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, DoorOpenTriggersActivity)
{
    auto config = makeTestConfig(1, 5);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for dim
    QTest::qWait(2500);
    EXPECT_NE(controller.state(), ScreenState::Active);

    // Door opens (UIState default is doorOpen=true, so set false first, then true)
    m_uiState->setDoorOpen(false);
    QTest::qWait(100);
    m_uiState->setDoorOpen(true);

    // Should reset to active
    EXPECT_EQ(controller.state(), ScreenState::Active);
    EXPECT_EQ(m_displayControl->brightness(), 100);
}

TEST_F(ScreenTimeoutControllerTest, DoorCloseTriggersImmediateDim)
{
    auto config = makeTestConfig(30, 60); // Long timeouts -- door close should override
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    EXPECT_EQ(controller.state(), ScreenState::Active);

    // Close door (set true first since default is already true)
    m_uiState->setDoorOpen(false);

    // Wait for immediate dim animation (~1s) + off delay (2s) + margin
    QTest::qWait(4000);

    // Should be dimmed or off (door close triggers immediate dim->off)
    EXPECT_NE(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, DisabledTimeoutStaysActive)
{
    auto config = makeTestConfig(1, 2, 25, false);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait well past timeout
    QTest::qWait(3000);

    // Should still be Active because timeout is disabled
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, OffTimeoutTransitionsToOff)
{
    auto config = makeTestConfig(1, 2); // 1s dim, 2s off
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for: dim timeout (1s) + animation (~1s) + off delay (2s-1s=1s) + margin
    QTest::qWait(4000);

    // Should be off
    EXPECT_EQ(controller.state(), ScreenState::Off);
    EXPECT_FALSE(m_displayControl->isPowered());
}

TEST_F(ScreenTimeoutControllerTest, ActivityFromOffRestoresPower)
{
    auto config = makeTestConfig(1, 2);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for off
    QTest::qWait(4000);
    EXPECT_EQ(controller.state(), ScreenState::Off);

    // Activity should restore
    controller.activityDetected();
    EXPECT_EQ(controller.state(), ScreenState::Active);
    EXPECT_TRUE(m_displayControl->isPowered());
    EXPECT_EQ(m_displayControl->brightness(), 100);
}

TEST_F(ScreenTimeoutControllerTest, StopCancelsAllTimers)
{
    auto config = makeTestConfig(1, 2);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();
    controller.stop();

    // Wait past timeout
    QTest::qWait(3000);

    // Should still be Active because stop was called
    EXPECT_EQ(controller.state(), ScreenState::Active);
}

TEST_F(ScreenTimeoutControllerTest, ScreenDimmedPropertySetOnOff)
{
    auto config = makeTestConfig(1, 2);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    EXPECT_FALSE(m_uiState->screenDimmed());

    // Wait for off
    QTest::qWait(4000);

    EXPECT_TRUE(m_uiState->screenDimmed());
}

TEST_F(ScreenTimeoutControllerTest, ScreenDimmedClearsOnActivity)
{
    auto config = makeTestConfig(1, 2);
    ScreenTimeoutController controller(m_displayControl, m_playbackState, m_uiState, config);
    controller.start();

    // Wait for off
    QTest::qWait(4000);
    EXPECT_TRUE(m_uiState->screenDimmed());

    // Activity
    controller.activityDetected();
    EXPECT_FALSE(m_uiState->screenDimmed());
}
