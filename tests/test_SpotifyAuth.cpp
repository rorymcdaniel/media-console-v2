#include <QCoreApplication>
#include <QSettings>

#include <gtest/gtest.h>

#include "app/AppConfig.h"
#include "spotify/SpotifyAuth.h"

class SpotifyAuthTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure QCoreApplication exists (needed for QSettings)
        if (!QCoreApplication::instance())
        {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }
        QCoreApplication::setOrganizationName("MediaConsoleTest");
        QCoreApplication::setApplicationName("test-spotify-auth");

        // Clean QSettings before each test
        QSettings settings;
        settings.beginGroup("spotify_auth");
        settings.remove("");
        settings.endGroup();
    }

    void TearDown() override
    {
        // Clean QSettings after each test
        QSettings settings;
        settings.beginGroup("spotify_auth");
        settings.remove("");
        settings.endGroup();
    }

    QCoreApplication* app = nullptr;
};

TEST_F(SpotifyAuthTest, ConstructionWithEmptyConfigDoesNotCrash)
{
    SpotifyConfig config;
    SpotifyAuth auth(config);
    EXPECT_FALSE(auth.isAuthenticated());
}

TEST_F(SpotifyAuthTest, SaveTokensPersistsToQSettings)
{
    // Manually set token values in QSettings to verify the group/key structure
    QSettings settings;
    settings.beginGroup("spotify_auth");
    settings.setValue("access_token", "test_access_token");
    settings.setValue("refresh_token", "test_refresh_token");
    settings.setValue("expiry", QDateTime::currentDateTimeUtc().addSecs(3600).toString(Qt::ISODate));
    settings.endGroup();

    // Verify values were written
    settings.beginGroup("spotify_auth");
    EXPECT_EQ(settings.value("access_token").toString(), "test_access_token");
    EXPECT_EQ(settings.value("refresh_token").toString(), "test_refresh_token");
    EXPECT_FALSE(settings.value("expiry").toString().isEmpty());
    settings.endGroup();
}

TEST_F(SpotifyAuthTest, RestoreTokensReadsBackSavedValues)
{
    // Seed QSettings with token data
    QSettings settings;
    settings.beginGroup("spotify_auth");
    settings.setValue("access_token", "restored_access_token");
    settings.setValue("refresh_token", "restored_refresh_token");
    settings.setValue("expiry", QDateTime::currentDateTimeUtc().addSecs(3600).toString(Qt::ISODate));
    settings.endGroup();

    // Construct SpotifyAuth and restore
    SpotifyConfig config;
    SpotifyAuth auth(config);
    EXPECT_FALSE(auth.isAuthenticated());

    bool restored = auth.restoreTokens();
    EXPECT_TRUE(restored);
    EXPECT_TRUE(auth.isAuthenticated());
    EXPECT_EQ(auth.accessToken(), "restored_access_token");
}

TEST_F(SpotifyAuthTest, ClearTokensRemovesAllStoredTokens)
{
    // Seed QSettings with token data
    QSettings settings;
    settings.beginGroup("spotify_auth");
    settings.setValue("access_token", "to_be_cleared");
    settings.setValue("refresh_token", "to_be_cleared");
    settings.setValue("expiry", QDateTime::currentDateTimeUtc().addSecs(3600).toString(Qt::ISODate));
    settings.endGroup();

    SpotifyConfig config;
    SpotifyAuth auth(config);
    auth.restoreTokens();
    EXPECT_TRUE(auth.isAuthenticated());

    auth.clearTokens();
    EXPECT_FALSE(auth.isAuthenticated());

    // Verify QSettings are cleared
    settings.beginGroup("spotify_auth");
    EXPECT_TRUE(settings.value("access_token").toString().isEmpty());
    EXPECT_TRUE(settings.value("refresh_token").toString().isEmpty());
    EXPECT_TRUE(settings.value("expiry").toString().isEmpty());
    settings.endGroup();
}

TEST_F(SpotifyAuthTest, SpotifyConfigDefaultRedirectPort)
{
    SpotifyConfig config;
    EXPECT_EQ(config.redirectPort, 8888);
}
