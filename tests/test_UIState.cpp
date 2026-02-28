#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "state/UIState.h"

class UIStateTest : public ::testing::Test
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
    }
};

TEST_F(UIStateTest, DefaultValues)
{
    UIState state;
    EXPECT_EQ(state.activeView(), ActiveView::NowPlaying);
    EXPECT_FALSE(state.volumeOverlayVisible());
    EXPECT_FALSE(state.errorBannerVisible());
    EXPECT_FALSE(state.toastVisible());
    EXPECT_TRUE(state.toastMessage().isEmpty());
    EXPECT_TRUE(state.toastType().isEmpty());
    EXPECT_FALSE(state.receiverConnected());
    EXPECT_TRUE(state.audioError().isEmpty());
    EXPECT_TRUE(state.doorOpen()); // default: door open = display on
}

TEST_F(UIStateTest, ActiveViewEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::activeViewChanged);

    state.setActiveView(ActiveView::LibraryBrowser);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(state.activeView(), ActiveView::LibraryBrowser);
}

TEST_F(UIStateTest, ActiveViewDoesNotEmitWhenUnchanged)
{
    UIState state;
    state.setActiveView(ActiveView::LibraryBrowser);

    QSignalSpy spy(&state, &UIState::activeViewChanged);
    state.setActiveView(ActiveView::LibraryBrowser);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UIStateTest, VolumeOverlayVisibleEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::volumeOverlayVisibleChanged);

    state.setVolumeOverlayVisible(true);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(UIStateTest, ErrorBannerVisibleEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::errorBannerVisibleChanged);

    state.setErrorBannerVisible(true);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(UIStateTest, ToastVisibleEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::toastVisibleChanged);

    state.setToastVisible(true);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(UIStateTest, ToastMessageEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::toastMessageChanged);

    state.setToastMessage("Connection lost");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Connection lost");
}

TEST_F(UIStateTest, ToastTypeEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::toastTypeChanged);

    state.setToastType("error");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "error");
}

TEST_F(UIStateTest, ReceiverConnectedEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::receiverConnectedChanged);

    state.setReceiverConnected(true);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(UIStateTest, AudioErrorEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::audioErrorChanged);

    state.setAudioError("ALSA device error");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "ALSA device error");
}

TEST_F(UIStateTest, ShowToastSignalEmits)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::showToast);

    emit state.showToast("Test message", "info");

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "Test message");
    EXPECT_EQ(spy.at(0).at(1).toString(), "info");
}

TEST_F(UIStateTest, DoorOpenEmitsChanged)
{
    UIState state;
    QSignalSpy spy(&state, &UIState::doorOpenChanged);

    state.setDoorOpen(false);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.at(0).at(0).toBool());
    EXPECT_FALSE(state.doorOpen());
}

TEST_F(UIStateTest, DoorOpenDoesNotEmitWhenUnchanged)
{
    UIState state;
    // doorOpen defaults to true
    QSignalSpy spy(&state, &UIState::doorOpenChanged);

    state.setDoorOpen(true); // same as default
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UIStateTest, DoorOpenChangeGuardConsecutiveSame)
{
    UIState state;
    state.setDoorOpen(false); // first change: true -> false

    QSignalSpy spy(&state, &UIState::doorOpenChanged);
    state.setDoorOpen(false); // duplicate, should not emit

    EXPECT_EQ(spy.count(), 0);
}
