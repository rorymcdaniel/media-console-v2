#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "platform/stubs/StubDisplayControl.h"

class StubDisplayControlTest : public ::testing::Test
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

TEST_F(StubDisplayControlTest, DefaultsPoweredAndFullBrightness)
{
    StubDisplayControl display;
    EXPECT_TRUE(display.isPowered());
    EXPECT_EQ(display.brightness(), 100);
}

TEST_F(StubDisplayControlTest, SetPowerStoresState)
{
    StubDisplayControl display;
    EXPECT_TRUE(display.isPowered());

    display.setPower(false);
    EXPECT_FALSE(display.isPowered());

    display.setPower(true);
    EXPECT_TRUE(display.isPowered());
}

TEST_F(StubDisplayControlTest, SetBrightnessStoresAndEmits)
{
    StubDisplayControl display;
    QSignalSpy spy(&display, &IDisplayControl::brightnessChanged);

    display.setBrightness(50);
    EXPECT_EQ(display.brightness(), 50);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 50);
}

TEST_F(StubDisplayControlTest, SetPowerEmitsSignal)
{
    StubDisplayControl display;
    QSignalSpy spy(&display, &IDisplayControl::powerChanged);

    display.setPower(false);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toBool(), false);
}

TEST_F(StubDisplayControlTest, AutoDetectSucceeds)
{
    StubDisplayControl display;
    EXPECT_TRUE(display.autoDetectDisplay());
}

TEST_F(StubDisplayControlTest, AutoDetectEmitsDisplayDetected)
{
    StubDisplayControl display;
    QSignalSpy spy(&display, &IDisplayControl::displayDetected);

    display.autoDetectDisplay();
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 1);
}
