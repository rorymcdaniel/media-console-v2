#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "platform/stubs/StubGpioMonitor.h"

class StubGpioMonitorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // QSignalSpy requires a QCoreApplication
        if (!QCoreApplication::instance())
        {
            static int argc = 1;
            static char appName[] = "test";
            static char* argv[] = { appName, nullptr };
            new QCoreApplication(argc, argv);
        }
    }
};

TEST_F(StubGpioMonitorTest, StartSucceeds)
{
    StubGpioMonitor gpio;
    EXPECT_TRUE(gpio.start());
}

TEST_F(StubGpioMonitorTest, SimulateVolumeChangePositiveEmitsSignal)
{
    StubGpioMonitor gpio;
    QSignalSpy spy(&gpio, &IGpioMonitor::volumeChanged);

    gpio.simulateVolumeChange(+2);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 2);
}

TEST_F(StubGpioMonitorTest, SimulateVolumeChangeNegativeEmitsSignal)
{
    StubGpioMonitor gpio;
    QSignalSpy spy(&gpio, &IGpioMonitor::volumeChanged);

    gpio.simulateVolumeChange(-2);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), -2);
}

TEST_F(StubGpioMonitorTest, SimulateMuteToggleEmitsSignal)
{
    StubGpioMonitor gpio;
    QSignalSpy spy(&gpio, &IGpioMonitor::muteToggled);

    gpio.simulateMuteToggle();
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(StubGpioMonitorTest, SimulateReedSwitchEmitsSignal)
{
    StubGpioMonitor gpio;
    QSignalSpy spy(&gpio, &IGpioMonitor::reedSwitchChanged);

    gpio.simulateReedSwitch(true);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toBool(), true);

    gpio.simulateReedSwitch(false);
    ASSERT_EQ(spy.count(), 2);
    EXPECT_EQ(spy.at(1).at(0).toBool(), false);
}

TEST_F(StubGpioMonitorTest, SimulateInputNextEmitsSignal)
{
    StubGpioMonitor gpio;
    QSignalSpy spy(&gpio, &IGpioMonitor::inputNext);

    gpio.simulateInputNext();
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(StubGpioMonitorTest, SimulateInputPreviousEmitsSignal)
{
    StubGpioMonitor gpio;
    QSignalSpy spy(&gpio, &IGpioMonitor::inputPrevious);

    gpio.simulateInputPrevious();
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(StubGpioMonitorTest, SimulateInputSelectEmitsSignal)
{
    StubGpioMonitor gpio;
    QSignalSpy spy(&gpio, &IGpioMonitor::inputSelect);

    gpio.simulateInputSelect();
    EXPECT_EQ(spy.count(), 1);
}
