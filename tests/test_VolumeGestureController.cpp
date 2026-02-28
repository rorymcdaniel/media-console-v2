#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include "receiver/VolumeGestureController.h"
#include "state/ReceiverState.h"
#include "state/UIState.h"

class VolumeGestureControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (QCoreApplication::instance() == nullptr)
        {
            static int argc = 1;
            static char appName[] = "test";
            static char* argv[] = { appName };
            m_app = new QCoreApplication(argc, argv);
        }

        m_receiverState = new ReceiverState();
        m_uiState = new UIState();
        m_controller = new VolumeGestureController(m_receiverState, m_uiState);
    }

    void TearDown() override
    {
        delete m_controller;
        delete m_uiState;
        delete m_receiverState;
    }

    static QCoreApplication* m_app;
    ReceiverState* m_receiverState = nullptr;
    UIState* m_uiState = nullptr;
    VolumeGestureController* m_controller = nullptr;
};

QCoreApplication* VolumeGestureControllerTest::m_app = nullptr;

// --- Optimistic Update Tests ---

TEST_F(VolumeGestureControllerTest, EncoderTickIncreasesVolume)
{
    m_receiverState->setVolume(50);
    m_controller->onEncoderTick(1);
    EXPECT_EQ(m_receiverState->volume(), 51);
}

TEST_F(VolumeGestureControllerTest, EncoderTickDecreasesVolume)
{
    m_receiverState->setVolume(50);
    m_controller->onEncoderTick(-1);
    EXPECT_EQ(m_receiverState->volume(), 49);
}

TEST_F(VolumeGestureControllerTest, MultipleTicks)
{
    m_receiverState->setVolume(50);
    m_controller->onEncoderTick(1);
    m_controller->onEncoderTick(1);
    m_controller->onEncoderTick(1);
    EXPECT_EQ(m_receiverState->volume(), 53);
}

// --- Clamping Tests ---

TEST_F(VolumeGestureControllerTest, VolumeClampedAtZero)
{
    m_receiverState->setVolume(0);
    m_controller->onEncoderTick(-1);
    EXPECT_EQ(m_receiverState->volume(), 0);
}

TEST_F(VolumeGestureControllerTest, VolumeClampedAt200)
{
    m_receiverState->setVolume(200);
    m_controller->onEncoderTick(1);
    EXPECT_EQ(m_receiverState->volume(), 200);
}

TEST_F(VolumeGestureControllerTest, LargeDeltaClamped)
{
    m_receiverState->setVolume(195);
    m_controller->onEncoderTick(10);
    EXPECT_EQ(m_receiverState->volume(), 200);
}

// --- Gesture State Tests ---

TEST_F(VolumeGestureControllerTest, GestureActiveAfterTick)
{
    m_controller->onEncoderTick(1);
    EXPECT_TRUE(m_controller->isGestureActive());
}

TEST_F(VolumeGestureControllerTest, GestureInactiveInitially)
{
    EXPECT_FALSE(m_controller->isGestureActive());
}

TEST_F(VolumeGestureControllerTest, GestureInactiveAfterTimeout)
{
    m_receiverState->setVolume(50);
    m_controller->onEncoderTick(1);
    EXPECT_TRUE(m_controller->isGestureActive());

    // Wait for gesture timeout (300ms + some margin)
    QTest::qWait(400);

    EXPECT_FALSE(m_controller->isGestureActive());
}

TEST_F(VolumeGestureControllerTest, GestureEndedSignalEmitted)
{
    QSignalSpy spy(m_controller, &VolumeGestureController::gestureEnded);

    m_receiverState->setVolume(50);
    m_controller->onEncoderTick(5);

    // Wait for gesture timeout
    QTest::qWait(400);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 55);
}

// --- Volume Overlay Tests ---

TEST_F(VolumeGestureControllerTest, EncoderTickShowsOverlay)
{
    EXPECT_FALSE(m_uiState->volumeOverlayVisible());
    m_controller->onEncoderTick(1);
    EXPECT_TRUE(m_uiState->volumeOverlayVisible());
}

// --- External Volume Update Tests ---

TEST_F(VolumeGestureControllerTest, ExternalUpdateWhenIdle)
{
    m_receiverState->setVolume(50);
    m_controller->onExternalVolumeUpdate(75);
    EXPECT_EQ(m_receiverState->volume(), 75);
}

TEST_F(VolumeGestureControllerTest, ExternalUpdateDoesNotShowOverlay)
{
    m_controller->onExternalVolumeUpdate(75);
    EXPECT_FALSE(m_uiState->volumeOverlayVisible());
}

TEST_F(VolumeGestureControllerTest, ExternalUpdateSuppressedDuringGesture)
{
    m_receiverState->setVolume(50);
    m_controller->onEncoderTick(5); // volume = 55, gesture active
    EXPECT_EQ(m_receiverState->volume(), 55);

    m_controller->onExternalVolumeUpdate(30); // Should be suppressed
    EXPECT_EQ(m_receiverState->volume(), 55); // Unchanged
}

TEST_F(VolumeGestureControllerTest, ExternalUpdateAfterGestureEnds)
{
    m_receiverState->setVolume(50);
    m_controller->onEncoderTick(5); // volume = 55

    // Wait for gesture to end
    QTest::qWait(400);

    // Now external update should work (reconciliation)
    m_controller->onExternalVolumeUpdate(53);
    EXPECT_EQ(m_receiverState->volume(), 53);
}
