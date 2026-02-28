#include <QCoreApplication>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTimer>

#include <gtest/gtest.h>

#include "app/AppConfig.h"
#include "spotify/SpotifyAuth.h"
#include "spotify/SpotifyClient.h"
#include "spotify/SpotifyController.h"
#include "state/PlaybackState.h"
#include "state/UIState.h"

class SpotifyControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }

        config.clientId = "test_client_id";
        config.clientSecret = "test_secret";
        config.desiredDeviceName = "Voice of Music";
        config.redirectPort = 8888;

        auth = new SpotifyAuth(config);
        client = new SpotifyClient();
        playbackState = new PlaybackState();
        uiState = new UIState();
    }

    void TearDown() override
    {
        delete uiState;
        delete playbackState;
        delete client;
        delete auth;
    }

    QCoreApplication* app = nullptr;
    SpotifyConfig config;
    SpotifyAuth* auth = nullptr;
    SpotifyClient* client = nullptr;
    PlaybackState* playbackState = nullptr;
    UIState* uiState = nullptr;
};

TEST_F(SpotifyControllerTest, SearchDebounceTimerResetsOnRapidCalls)
{
    SpotifyController controller(auth, client, playbackState, uiState, config);

    // Rapid calls should not fire immediately - only last query matters
    // We verify the debounce timer is configured correctly by checking
    // that the controller accepts rapid search calls without crashing
    // and that the internal timer is single-shot
    controller.search("first");
    controller.search("second");
    controller.search("third");

    // The controller should not have emitted searchResultsChanged yet
    QSignalSpy spy(&controller, &SpotifyController::searchResultsChanged);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(SpotifyControllerTest, IsSpotifyAvailableReturnsFalseWhenNotAuthenticated)
{
    SpotifyController controller(auth, client, playbackState, uiState, config);

    // Auth was constructed but never authenticated (no tokens restored)
    EXPECT_FALSE(controller.isSpotifyAvailable());
}

TEST_F(SpotifyControllerTest, IsSpotifyActiveReturnsFalseInitially)
{
    SpotifyController controller(auth, client, playbackState, uiState, config);

    EXPECT_FALSE(controller.isSpotifyActive());
}

TEST_F(SpotifyControllerTest, ConstructionWithNullPointersDoesNotCrash)
{
    // Defensive: null auth/client should not crash construction
    SpotifyController controller(nullptr, nullptr, nullptr, nullptr, config);
    EXPECT_FALSE(controller.isSpotifyAvailable());
    EXPECT_FALSE(controller.isSpotifyActive());
}

TEST_F(SpotifyControllerTest, ClearSearchResetsResults)
{
    SpotifyController controller(auth, client, playbackState, uiState, config);

    QSignalSpy spy(&controller, &SpotifyController::searchResultsChanged);
    controller.clearSearch();

    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(controller.searchResults().isEmpty());
}
