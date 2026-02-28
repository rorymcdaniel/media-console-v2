#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>

#include <gtest/gtest.h>

#include "api/HttpApiServer.h"
#include "app/AppConfig.h"
#include "platform/stubs/StubDisplayControl.h"
#include "spotify/SpotifyAuth.h"
#include "spotify/SpotifyClient.h"
#include "spotify/SpotifyController.h"
#include "state/MediaSource.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"
#include "state/UIState.h"

#ifdef HAS_QT_HTTPSERVER
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#endif

// ============================================================================
// InputStringMapping tests (always available -- static method, no HttpServer)
// ============================================================================

class HttpApiServerInputMappingTest : public ::testing::Test
{
};

TEST_F(HttpApiServerInputMappingTest, StreamingMapsCorrectly)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("streaming"), MediaSource::Streaming);
}

TEST_F(HttpApiServerInputMappingTest, NetMapsToStreaming)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("net"), MediaSource::Streaming);
}

TEST_F(HttpApiServerInputMappingTest, PhonoMapsCorrectly)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("phono"), MediaSource::Phono);
}

TEST_F(HttpApiServerInputMappingTest, CdMapsCorrectly)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("cd"), MediaSource::CD);
}

TEST_F(HttpApiServerInputMappingTest, ComputerMapsCorrectly)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("computer"), MediaSource::Computer);
}

TEST_F(HttpApiServerInputMappingTest, BluetoothMapsCorrectly)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("bluetooth"), MediaSource::Bluetooth);
}

TEST_F(HttpApiServerInputMappingTest, LibraryMapsCorrectly)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("library"), MediaSource::Library);
}

TEST_F(HttpApiServerInputMappingTest, UnknownReturnsNone)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("invalid"), MediaSource::None);
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource(""), MediaSource::None);
}

TEST_F(HttpApiServerInputMappingTest, CaseInsensitive)
{
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("STREAMING"), MediaSource::Streaming);
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("Phono"), MediaSource::Phono);
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("CD"), MediaSource::CD);
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("Bluetooth"), MediaSource::Bluetooth);
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("LIBRARY"), MediaSource::Library);
    EXPECT_EQ(HttpApiServer::inputStringToMediaSource("NET"), MediaSource::Streaming);
}

// ============================================================================
// Construction and lifecycle tests
// ============================================================================

class HttpApiServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }

        spotifyConfig.clientId = "test_client_id";
        spotifyConfig.clientSecret = "test_secret";
        spotifyConfig.desiredDeviceName = "Voice of Music";
        spotifyConfig.redirectPort = 8888;

        apiConfig.port = 0; // Will be overridden to a free port for testing

        receiverState = new ReceiverState();
        playbackState = new PlaybackState();
        displayControl = new StubDisplayControl();
        auth = new SpotifyAuth(spotifyConfig);
        client = new SpotifyClient();
        uiState = new UIState();
        spotifyController = new SpotifyController(auth, client, playbackState, uiState, spotifyConfig);
    }

    void TearDown() override
    {
        delete spotifyController;
        delete uiState;
        delete client;
        delete auth;
        delete displayControl;
        delete playbackState;
        delete receiverState;
    }

    QCoreApplication* app = nullptr;
    SpotifyConfig spotifyConfig;
    ApiConfig apiConfig;
    ReceiverState* receiverState = nullptr;
    PlaybackState* playbackState = nullptr;
    StubDisplayControl* displayControl = nullptr;
    SpotifyAuth* auth = nullptr;
    SpotifyClient* client = nullptr;
    UIState* uiState = nullptr;
    SpotifyController* spotifyController = nullptr;
};

TEST_F(HttpApiServerTest, ConstructionSucceeds)
{
    HttpApiServer server(nullptr, receiverState, playbackState, displayControl, auth, spotifyController, apiConfig);

    EXPECT_EQ(server.port(), apiConfig.port);
}

TEST_F(HttpApiServerTest, ConstructionWithAllNullptrsDoesNotCrash)
{
    HttpApiServer server(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, apiConfig);

    EXPECT_EQ(server.port(), apiConfig.port);
}

TEST_F(HttpApiServerTest, ServerUrlEmptyWhenNotStarted)
{
    HttpApiServer server(nullptr, receiverState, playbackState, displayControl, auth, spotifyController, apiConfig);

    EXPECT_TRUE(server.serverUrl().isEmpty());
}

TEST_F(HttpApiServerTest, StopWithoutStartDoesNotCrash)
{
    HttpApiServer server(nullptr, receiverState, playbackState, displayControl, auth, spotifyController, apiConfig);

    server.stop(); // Should not crash
}

// ============================================================================
// HttpServer integration tests (only when Qt6::HttpServer is available)
// ============================================================================

#ifdef HAS_QT_HTTPSERVER

/// Find a free port by binding to port 0 and reading the assigned port.
static int findFreePort()
{
    QTcpServer tmp;
    tmp.listen(QHostAddress::LocalHost, 0);
    int port = tmp.serverPort();
    tmp.close();
    return port;
}

/// Helper: send a GET request and wait for reply.
static QJsonObject httpGet(QNetworkAccessManager& nam, const QString& url)
{
    QUrl qurl(url);
    QNetworkRequest req { qurl };
    QNetworkReply* reply = nam.get(req);

    // Spin event loop until reply finishes
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit); // timeout safety
    loop.exec();

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return QJsonDocument::fromJson(data).object();
}

/// Helper: send a POST request with JSON body and wait for reply.
static QJsonObject httpPost(QNetworkAccessManager& nam, const QString& url, const QJsonObject& body)
{
    QUrl qurl(url);
    QNetworkRequest req { qurl };
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = nam.post(req, bodyData);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return QJsonDocument::fromJson(data).object();
}

class HttpApiServerIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }

        spotifyConfig.clientId = "test_client_id";
        spotifyConfig.clientSecret = "test_secret";
        spotifyConfig.desiredDeviceName = "Voice of Music";
        spotifyConfig.redirectPort = 8888;

        freePort = findFreePort();
        apiConfig.port = freePort;

        receiverState = new ReceiverState();
        playbackState = new PlaybackState();
        displayControl = new StubDisplayControl();
        auth = new SpotifyAuth(spotifyConfig);
        client = new SpotifyClient();
        uiState = new UIState();
        spotifyController = new SpotifyController(auth, client, playbackState, uiState, spotifyConfig);

        server = new HttpApiServer(nullptr, receiverState, playbackState, displayControl, auth, spotifyController,
                                   apiConfig);
    }

    void TearDown() override
    {
        if (server)
        {
            server->stop();
            delete server;
        }
        delete spotifyController;
        delete uiState;
        delete client;
        delete auth;
        delete displayControl;
        delete playbackState;
        delete receiverState;
    }

    QString baseUrl() const { return QString("http://127.0.0.1:%1").arg(freePort); }

    QCoreApplication* app = nullptr;
    SpotifyConfig spotifyConfig;
    ApiConfig apiConfig;
    int freePort = 0;
    ReceiverState* receiverState = nullptr;
    PlaybackState* playbackState = nullptr;
    StubDisplayControl* displayControl = nullptr;
    SpotifyAuth* auth = nullptr;
    SpotifyClient* client = nullptr;
    UIState* uiState = nullptr;
    SpotifyController* spotifyController = nullptr;
    HttpApiServer* server = nullptr;
};

TEST_F(HttpApiServerIntegrationTest, StartSucceeds)
{
    EXPECT_TRUE(server->start());
    EXPECT_FALSE(server->serverUrl().isEmpty());
}

TEST_F(HttpApiServerIntegrationTest, StartAndStopLifecycle)
{
    EXPECT_TRUE(server->start());
    EXPECT_FALSE(server->serverUrl().isEmpty());

    server->stop();
    EXPECT_TRUE(server->serverUrl().isEmpty());
}

TEST_F(HttpApiServerIntegrationTest, GetStatusReturnsJson)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject status = httpGet(nam, baseUrl() + "/api/status");

    EXPECT_TRUE(status.contains("volume"));
    EXPECT_TRUE(status.contains("powered"));
    EXPECT_TRUE(status.contains("muted"));
    EXPECT_TRUE(status.contains("input"));
    EXPECT_TRUE(status.contains("displayPowered"));
    EXPECT_TRUE(status.contains("displayBrightness"));
    EXPECT_TRUE(status.contains("playbackMode"));
    EXPECT_TRUE(status.contains("activeSource"));
}

TEST_F(HttpApiServerIntegrationTest, GetStatusReflectsState)
{
    receiverState->setVolume(42);
    receiverState->setPowered(true);
    receiverState->setMuted(false);
    receiverState->setCurrentInput(MediaSource::Phono);
    playbackState->setPlaybackMode(PlaybackMode::Playing);
    playbackState->setActiveSource(MediaSource::Phono);

    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject status = httpGet(nam, baseUrl() + "/api/status");

    EXPECT_EQ(status["volume"].toInt(), 42);
    EXPECT_EQ(status["powered"].toBool(), true);
    EXPECT_EQ(status["muted"].toBool(), false);
    EXPECT_EQ(status["input"].toString(), "phono");
    EXPECT_EQ(status["playbackMode"].toString(), "playing");
    EXPECT_EQ(status["activeSource"].toString(), "phono");
}

TEST_F(HttpApiServerIntegrationTest, PostVolumeReturnsOk)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject body { { "volume", 30 } };
    QJsonObject response = httpPost(nam, baseUrl() + "/api/volume", body);

    EXPECT_EQ(response["status"].toString(), "ok");
}

TEST_F(HttpApiServerIntegrationTest, PostVolumeWithoutParamReturnsBadRequest)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject body { { "not_volume", 30 } };
    QJsonObject response = httpPost(nam, baseUrl() + "/api/volume", body);

    EXPECT_TRUE(response.contains("error"));
}

TEST_F(HttpApiServerIntegrationTest, PostInputReturnsOk)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject body { { "input", "phono" } };
    QJsonObject response = httpPost(nam, baseUrl() + "/api/input", body);

    EXPECT_EQ(response["status"].toString(), "ok");
}

TEST_F(HttpApiServerIntegrationTest, PostInputUnknownReturnsBadRequest)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject body { { "input", "unknown_input" } };
    QJsonObject response = httpPost(nam, baseUrl() + "/api/input", body);

    EXPECT_TRUE(response.contains("error"));
}

TEST_F(HttpApiServerIntegrationTest, PostDisplayReturnsOk)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject body { { "power", true } };
    QJsonObject response = httpPost(nam, baseUrl() + "/api/display", body);

    EXPECT_EQ(response["status"].toString(), "ok");
}

TEST_F(HttpApiServerIntegrationTest, SpotifyStatusReturnsJson)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject status = httpGet(nam, baseUrl() + "/spotify/status");

    EXPECT_TRUE(status.contains("authenticated"));
    EXPECT_EQ(status["authenticated"].toBool(), false); // Not authenticated in test
}

TEST_F(HttpApiServerIntegrationTest, SpotifyPlayWhenNotAuthenticatedReturnsError)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    QJsonObject response = httpPost(nam, baseUrl() + "/spotify/play", QJsonObject {});

    EXPECT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"].toString(), "Not authenticated");
}

TEST_F(HttpApiServerIntegrationTest, ActivityDetectedSignalEmitted)
{
    ASSERT_TRUE(server->start());

    QSignalSpy activitySpy(server, &HttpApiServer::activityDetected);

    QNetworkAccessManager nam;
    httpGet(nam, baseUrl() + "/api/status");

    EXPECT_GE(activitySpy.count(), 1);
}

TEST_F(HttpApiServerIntegrationTest, ActivityDetectedOnPostRequests)
{
    ASSERT_TRUE(server->start());

    QSignalSpy activitySpy(server, &HttpApiServer::activityDetected);

    QNetworkAccessManager nam;
    httpPost(nam, baseUrl() + "/api/volume", QJsonObject { { "volume", 10 } });

    EXPECT_GE(activitySpy.count(), 1);
}

TEST_F(HttpApiServerIntegrationTest, SpotifySearchWhenNotAuthenticatedReturnsError)
{
    ASSERT_TRUE(server->start());

    QNetworkAccessManager nam;
    auto json = httpGet(nam, baseUrl() + "/spotify/search?q=test");

    EXPECT_TRUE(json.contains("error"));
}

#endif // HAS_QT_HTTPSERVER
