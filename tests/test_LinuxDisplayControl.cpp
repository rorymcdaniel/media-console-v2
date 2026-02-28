#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "display/LinuxDisplayControl.h"

class LinuxDisplayControlTest : public ::testing::Test
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

TEST_F(LinuxDisplayControlTest, DefaultState)
{
    LinuxDisplayControl display;
    EXPECT_EQ(display.busNumber(), -1);
    EXPECT_EQ(display.brightness(), 100);
    EXPECT_TRUE(display.isPowered());
}

TEST_F(LinuxDisplayControlTest, SetBrightnessFailsWithoutDetection)
{
    LinuxDisplayControl display;
    // No bus detected, should return false
    EXPECT_FALSE(display.setBrightness(50));
    // Brightness should remain at initial value
    EXPECT_EQ(display.brightness(), 100);
}

TEST_F(LinuxDisplayControlTest, SetPowerFailsWithoutDetection)
{
    LinuxDisplayControl display;
    // No bus detected, should return false
    EXPECT_FALSE(display.setPower(false));
    // Power state should remain at initial value
    EXPECT_TRUE(display.isPowered());
}

TEST_F(LinuxDisplayControlTest, AutoDetectFailsWithoutDdcutil)
{
    LinuxDisplayControl display;
    // On macOS (dev), ddcutil is not installed, should return false
    bool result = display.autoDetectDisplay();
    // We can't guarantee ddcutil is installed on dev machine
    // But bus number should stay -1 if detection fails
    if (!result)
    {
        EXPECT_EQ(display.busNumber(), -1);
    }
}

TEST_F(LinuxDisplayControlTest, SetBrightnessNoSignalWithoutDetection)
{
    LinuxDisplayControl display;
    QSignalSpy spy(&display, &IDisplayControl::brightnessChanged);

    display.setBrightness(50);
    // Should not emit signal when no bus detected
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(LinuxDisplayControlTest, SetPowerNoSignalWithoutDetection)
{
    LinuxDisplayControl display;
    QSignalSpy spy(&display, &IDisplayControl::powerChanged);

    display.setPower(false);
    // Should not emit signal when no bus detected
    EXPECT_EQ(spy.count(), 0);
}
