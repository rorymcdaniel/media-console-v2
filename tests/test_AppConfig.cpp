#include <gtest/gtest.h>

#include "app/AppConfig.h"

// Test default struct values (no QSettings needed for these)

TEST(AppConfig, DefaultReceiverHost)
{
    AppConfig config;
    EXPECT_EQ(config.receiver.host, "192.168.68.63");
}

TEST(AppConfig, DefaultReceiverPort)
{
    AppConfig config;
    EXPECT_EQ(config.receiver.port, 60128);
}

TEST(AppConfig, DefaultApiPort)
{
    AppConfig config;
    EXPECT_EQ(config.api.port, 8080);
}

TEST(AppConfig, DefaultDisplayDimTimeout)
{
    AppConfig config;
    EXPECT_EQ(config.display.dimTimeoutSeconds, 300);
}

TEST(AppConfig, DefaultDisplayOffTimeout)
{
    AppConfig config;
    EXPECT_EQ(config.display.offTimeoutSeconds, 1200);
}

TEST(AppConfig, DefaultDisplayDimBrightness)
{
    AppConfig config;
    EXPECT_EQ(config.display.dimBrightness, 25);
}

TEST(AppConfig, DefaultDisplayTimeoutEnabled)
{
    AppConfig config;
    EXPECT_TRUE(config.display.timeoutEnabled);
}

TEST(AppConfig, DefaultCdDevicePath)
{
    AppConfig config;
    EXPECT_EQ(config.cd.devicePath, "/dev/cdrom");
}

TEST(AppConfig, DefaultSpotifyDesiredDeviceName)
{
    AppConfig config;
    EXPECT_EQ(config.spotify.desiredDeviceName, "Voice of Music");
}

TEST(AppConfig, DefaultLibraryRootPath)
{
    AppConfig config;
    EXPECT_EQ(config.library.rootPath, "/data/media/music/");
}
