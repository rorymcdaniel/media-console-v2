#pragma once

#include <QObject>
#include <QString>

class ReceiverController;
class ReceiverState;
class PlaybackState;
class IDisplayControl;
class SpotifyAuth;
class SpotifyController;
class QTcpServer;
struct ApiConfig;

#ifdef HAS_QT_HTTPSERVER
#include <QHttpServer>
#ifdef QT_SSL_AVAILABLE
#include <QSslCertificate>
#include <QSslKey>
#include <QSslServer>
#endif
#endif

#include "state/MediaSource.h"

/**
 * @brief HTTP API server for remote control and Spotify OAuth.
 *
 * Provides REST endpoints for controlling volume, input, display power,
 * and system status. Also serves Spotify OAuth HTML pages for authentication.
 *
 * Requires Qt6::HttpServer at compile time (HAS_QT_HTTPSERVER define).
 * Optional HTTPS via self-signed certificate when Qt SSL is available.
 */
class HttpApiServer : public QObject
{
    Q_OBJECT

public:
    HttpApiServer(ReceiverController* receiverController, ReceiverState* receiverState, PlaybackState* playbackState,
                  IDisplayControl* displayControl, SpotifyAuth* spotifyAuth, SpotifyController* spotifyController,
                  const ApiConfig& config, QObject* parent = nullptr);

    ~HttpApiServer() override = default;

    bool start();
    void stop();
    QString serverUrl() const;
    int port() const { return m_port; }

    /// Map a user-facing input string to MediaSource enum.
    /// Returns MediaSource::None for unrecognized strings.
    static MediaSource inputStringToMediaSource(const QString& input);

signals:
    /// Emitted on every API request -- connect to ScreenTimeoutController.
    void activityDetected();

private:
#ifdef HAS_QT_HTTPSERVER
    void setupRoutes();
#ifdef QT_SSL_AVAILABLE
    bool setupSSL();
    bool generateSelfSignedCertificate();
    QString getCertificatePath() const;
    QString getKeyPath() const;
#endif
    QHttpServer* m_server = nullptr;
#endif

    QTcpServer* m_tcpServer = nullptr;
    ReceiverController* m_receiverController = nullptr;
    ReceiverState* m_receiverState = nullptr;
    PlaybackState* m_playbackState = nullptr;
    IDisplayControl* m_displayControl = nullptr;
    SpotifyAuth* m_spotifyAuth = nullptr;
    SpotifyController* m_spotifyController = nullptr;
    int m_port = 8080;

#ifdef QT_SSL_AVAILABLE
    QSslCertificate m_certificate;
    QSslKey m_privateKey;
    bool m_sslEnabled = false;
#endif
};
