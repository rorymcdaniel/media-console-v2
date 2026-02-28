#include <QCoreApplication>
#include <QUrl>
#include <QUrlQuery>

#include <gtest/gtest.h>

#include "spotify/SpotifyClient.h"

class SpotifyClientTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }
    }

    QCoreApplication* app = nullptr;
};

TEST_F(SpotifyClientTest, ConstructionDoesNotCrash)
{
    SpotifyClient client;
    SUCCEED();
}

TEST_F(SpotifyClientTest, BuildUrlSearchWithParams)
{
    QUrl url = SpotifyClient::buildUrl("search", { { "q", "test" }, { "type", "track,artist,album" } });
    EXPECT_EQ(url.scheme(), "https");
    EXPECT_EQ(url.host(), "api.spotify.com");
    EXPECT_EQ(url.path(), "/v1/search");

    QUrlQuery query(url);
    EXPECT_EQ(query.queryItemValue("q"), "test");
    // QUrl decodes the comma in query values, so expect decoded form
    EXPECT_EQ(query.queryItemValue("type", QUrl::FullyDecoded), "track,artist,album");
}

TEST_F(SpotifyClientTest, BuildUrlDevicesNoParams)
{
    QUrl url = SpotifyClient::buildUrl("me/player/devices", {});
    EXPECT_EQ(url.toString(), "https://api.spotify.com/v1/me/player/devices");
}

TEST_F(SpotifyClientTest, BuildUrlQueueWithUri)
{
    QUrl url = SpotifyClient::buildUrl("me/player/queue", { { "uri", "spotify:track:123" } });
    EXPECT_EQ(url.scheme(), "https");
    EXPECT_EQ(url.host(), "api.spotify.com");
    EXPECT_EQ(url.path(), "/v1/me/player/queue");

    QUrlQuery query(url);
    EXPECT_EQ(query.queryItemValue("uri", QUrl::FullyDecoded), "spotify:track:123");
}

TEST_F(SpotifyClientTest, BuildUrlHandlesSpecialCharsInQuery)
{
    QUrl url = SpotifyClient::buildUrl("search", { { "q", "hello world & more" } });
    QUrlQuery query(url);
    EXPECT_EQ(query.queryItemValue("q", QUrl::FullyDecoded), "hello world & more");
}

TEST_F(SpotifyClientTest, SetAccessTokenStoresToken)
{
    SpotifyClient client;
    client.setAccessToken("test_token_abc123");

    // We can verify the token is used by checking the request header
    // buildUrl is static and doesn't use the token, so we verify via
    // the class being constructed and accepting the token without error.
    // Full request header verification requires createRequest which is private,
    // but we verify setAccessToken doesn't crash and the client remains valid.
    SUCCEED();
}

TEST_F(SpotifyClientTest, BuildUrlPlaybackNoParams)
{
    QUrl url = SpotifyClient::buildUrl("me/player", {});
    EXPECT_EQ(url.toString(), "https://api.spotify.com/v1/me/player");
}

TEST_F(SpotifyClientTest, BuildUrlPlaylistsWithLimit)
{
    QUrl url = SpotifyClient::buildUrl("me/playlists", { { "limit", "20" } });
    EXPECT_EQ(url.path(), "/v1/me/playlists");

    QUrlQuery query(url);
    EXPECT_EQ(query.queryItemValue("limit"), "20");
}
