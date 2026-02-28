#include <gtest/gtest.h>

#include "app/AppConfig.h"
#include "platform/IAudioOutput.h"
#include "platform/ICdDrive.h"
#include "platform/IDisplayControl.h"
#include "platform/IGpioMonitor.h"
#include "platform/PlatformFactory.h"

TEST(PlatformFactory, CreatesNonNullAudioOutput)
{
    auto audio = PlatformFactory::createAudioOutput();
    ASSERT_NE(audio, nullptr);
}

TEST(PlatformFactory, CreatesNonNullCdDrive)
{
    auto cd = PlatformFactory::createCdDrive();
    ASSERT_NE(cd, nullptr);
}

TEST(PlatformFactory, CreatesNonNullGpioMonitor)
{
    GpioConfig config;
    auto gpio = PlatformFactory::createGpioMonitor(config);
    ASSERT_NE(gpio, nullptr);
}

TEST(PlatformFactory, CreatesNonNullDisplayControl)
{
    auto display = PlatformFactory::createDisplayControl();
    ASSERT_NE(display, nullptr);
}
