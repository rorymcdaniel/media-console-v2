#include "api/HttpApiServer.h"

#include <QDebug>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>

#include "app/AppConfig.h"
#include "platform/IDisplayControl.h"
#include "receiver/ReceiverController.h"
#include "spotify/SpotifyAuth.h"
#include "spotify/SpotifyController.h"
#include "state/PlaybackMode.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"

#ifdef HAS_QT_HTTPSERVER
#include <QDir>
#include <QFile>
#include <QNetworkInterface>
#include <QProcess>
#include <QStandardPaths>
#include <QUrlQuery>

#ifdef QT_SSL_AVAILABLE
#include <QSslConfiguration>
#include <QSslServer>
#include <QSslSocket>
#endif
#endif

HttpApiServer::HttpApiServer(ReceiverController* receiverController, ReceiverState* receiverState,
                             PlaybackState* playbackState, IDisplayControl* displayControl, SpotifyAuth* spotifyAuth,
                             SpotifyController* spotifyController, const ApiConfig& config, QObject* parent)
    : QObject(parent)
#ifdef HAS_QT_HTTPSERVER
    , m_server(new QHttpServer(this))
#endif
    , m_receiverController(receiverController)
    , m_receiverState(receiverState)
    , m_playbackState(playbackState)
    , m_displayControl(displayControl)
    , m_spotifyAuth(spotifyAuth)
    , m_spotifyController(spotifyController)
    , m_port(config.port)
{
#ifdef HAS_QT_HTTPSERVER
#ifdef QT_SSL_AVAILABLE
    m_tcpServer = new QSslServer(this);
#else
    m_tcpServer = new QTcpServer(this);
#endif
    setupRoutes();
#else
    m_tcpServer = new QTcpServer(this);
    qWarning() << "[API] Qt6::HttpServer not available -- HTTP API disabled";
#endif
}

bool HttpApiServer::start()
{
#ifdef HAS_QT_HTTPSERVER

#ifdef QT_SSL_AVAILABLE
    if (!setupSSL())
    {
        qWarning() << "[API] Failed to setup SSL";
        return false;
    }

    QSslServer* sslServer = qobject_cast<QSslServer*>(m_tcpServer);
    if (sslServer)
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setLocalCertificate(m_certificate);
        sslConfig.setPrivateKey(m_privateKey);
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfig.setProtocol(QSsl::TlsV1_2OrLater);

        sslServer->setSslConfiguration(sslConfig);
        m_sslEnabled = true;
        qInfo() << "[API] SSL configuration applied";
    }
#else
    qWarning() << "[API] SSL not available (Qt6 Ssl not found) - falling back to HTTP";
#endif

    if (!m_tcpServer->listen(QHostAddress::Any, m_port))
    {
        qWarning() << "[API] Failed to start TCP server on port" << m_port << ":" << m_tcpServer->errorString();
        return false;
    }

    if (!m_server->bind(m_tcpServer))
    {
        qWarning() << "[API] Failed to bind HTTP server to TCP server";
        m_tcpServer->close();
        return false;
    }

#ifdef QT_SSL_AVAILABLE
    if (m_sslEnabled)
    {
        qInfo() << "[API] HTTPS server listening on port" << m_tcpServer->serverPort();
    }
    else
    {
        qInfo() << "[API] HTTP server listening on port" << m_tcpServer->serverPort() << "(SSL setup failed)";
    }
#else
    qInfo() << "[API] HTTP server listening on port" << m_tcpServer->serverPort() << "(SSL not available)";
#endif
    return true;

#else
    // No HttpServer support -- cannot start
    qWarning() << "[API] Cannot start: Qt6::HttpServer not available";
    return false;
#endif
}

void HttpApiServer::stop()
{
    if (m_tcpServer && m_tcpServer->isListening())
    {
        m_tcpServer->close();
        qInfo() << "[API] HTTP server stopped";
    }
}

QString HttpApiServer::serverUrl() const
{
    if (!m_tcpServer || !m_tcpServer->isListening())
    {
        return QString();
    }

#ifdef QT_SSL_AVAILABLE
    return QString("https://127.0.0.1:%1").arg(m_tcpServer->serverPort());
#else
    return QString("http://127.0.0.1:%1").arg(m_tcpServer->serverPort());
#endif
}

MediaSource HttpApiServer::inputStringToMediaSource(const QString& input)
{
    const QString lower = input.toLower();
    if (lower == "streaming" || lower == "net")
    {
        return MediaSource::Streaming;
    }
    if (lower == "phono")
    {
        return MediaSource::Phono;
    }
    if (lower == "cd")
    {
        return MediaSource::CD;
    }
    if (lower == "computer")
    {
        return MediaSource::Computer;
    }
    if (lower == "bluetooth")
    {
        return MediaSource::Bluetooth;
    }
    if (lower == "library")
    {
        return MediaSource::Library;
    }
    return MediaSource::None;
}

#ifdef HAS_QT_HTTPSERVER

void HttpApiServer::setupRoutes()
{
    // ========================================================================
    // Core Control Endpoints
    // ========================================================================

    // POST /api/volume
    m_server->route("/api/volume", QHttpServerRequest::Method::Post,
                    [this](const QHttpServerRequest& request)
                    {
                        emit activityDetected();

                        auto json = QJsonDocument::fromJson(request.body()).object();
                        if (!json.contains("volume") || !json["volume"].isDouble())
                        {
                            return QHttpServerResponse(
                                QJsonObject { { "error", "Missing or invalid 'volume' parameter" } },
                                QHttpServerResponder::StatusCode::BadRequest);
                        }

                        int volume = json["volume"].toInt();
                        qDebug() << "[API] Set volume:" << volume;

                        if (m_receiverController)
                        {
                            m_receiverController->setVolume(volume);
                        }

                        return QHttpServerResponse(QJsonObject { { "status", "ok" } });
                    });

    // POST /api/input
    m_server->route(
        "/api/input", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request)
        {
            emit activityDetected();

            auto json = QJsonDocument::fromJson(request.body()).object();
            if (!json.contains("input") || !json["input"].isString())
            {
                return QHttpServerResponse(QJsonObject { { "error", "Missing or invalid 'input' parameter" } },
                                           QHttpServerResponder::StatusCode::BadRequest);
            }

            QString inputCode = json["input"].toString();
            MediaSource source = inputStringToMediaSource(inputCode);

            if (source == MediaSource::None)
            {
                return QHttpServerResponse(QJsonObject { { "error", QString("Unknown input: %1").arg(inputCode) } },
                                           QHttpServerResponder::StatusCode::BadRequest);
            }

            qDebug() << "[API] Set input:" << inputCode;

            if (m_receiverController)
            {
                m_receiverController->selectInput(source);
            }

            return QHttpServerResponse(QJsonObject { { "status", "ok" } });
        });

    // POST /api/display
    m_server->route("/api/display", QHttpServerRequest::Method::Post,
                    [this](const QHttpServerRequest& request)
                    {
                        emit activityDetected();

                        auto json = QJsonDocument::fromJson(request.body()).object();
                        if (!json.contains("power") || !json["power"].isBool())
                        {
                            return QHttpServerResponse(
                                QJsonObject { { "error", "Missing or invalid 'power' parameter" } },
                                QHttpServerResponder::StatusCode::BadRequest);
                        }

                        bool power = json["power"].toBool();
                        qDebug() << "[API] Set display power:" << power;

                        if (m_displayControl)
                        {
                            m_displayControl->setPower(power);
                        }

                        return QHttpServerResponse(QJsonObject { { "status", "ok" } });
                    });

    // GET /api/status
    m_server->route(
        "/api/status", QHttpServerRequest::Method::Get,
        [this]()
        {
            emit activityDetected();

            // Build playback mode string
            QString playbackModeStr = "stopped";
            if (m_playbackState)
            {
                switch (m_playbackState->playbackMode())
                {
                case PlaybackMode::Playing:
                    playbackModeStr = "playing";
                    break;
                case PlaybackMode::Paused:
                    playbackModeStr = "paused";
                    break;
                case PlaybackMode::Stopped:
                default:
                    playbackModeStr = "stopped";
                    break;
                }
            }

            // Build active source string
            QString activeSourceStr = "none";
            if (m_playbackState)
            {
                switch (m_playbackState->activeSource())
                {
                case MediaSource::Streaming:
                    activeSourceStr = "streaming";
                    break;
                case MediaSource::Phono:
                    activeSourceStr = "phono";
                    break;
                case MediaSource::CD:
                    activeSourceStr = "cd";
                    break;
                case MediaSource::Computer:
                    activeSourceStr = "computer";
                    break;
                case MediaSource::Bluetooth:
                    activeSourceStr = "bluetooth";
                    break;
                case MediaSource::Library:
                    activeSourceStr = "library";
                    break;
                default:
                    activeSourceStr = "none";
                    break;
                }
            }

            // Build input string
            QString inputStr = "none";
            if (m_receiverState)
            {
                switch (m_receiverState->currentInput())
                {
                case MediaSource::Streaming:
                    inputStr = "streaming";
                    break;
                case MediaSource::Phono:
                    inputStr = "phono";
                    break;
                case MediaSource::CD:
                    inputStr = "cd";
                    break;
                case MediaSource::Computer:
                    inputStr = "computer";
                    break;
                case MediaSource::Bluetooth:
                    inputStr = "bluetooth";
                    break;
                case MediaSource::Library:
                    inputStr = "library";
                    break;
                default:
                    inputStr = "none";
                    break;
                }
            }

            return QJsonObject { { "volume", m_receiverState ? m_receiverState->volume() : 0 },
                                 { "powered", m_receiverState ? m_receiverState->powered() : false },
                                 { "muted", m_receiverState ? m_receiverState->muted() : false },
                                 { "input", inputStr },
                                 { "displayPowered", m_displayControl ? m_displayControl->isPowered() : false },
                                 { "displayBrightness", m_displayControl ? m_displayControl->brightness() : 0 },
                                 { "playbackMode", playbackModeStr },
                                 { "activeSource", activeSourceStr } };
        });

    // ========================================================================
    // Spotify OAuth Endpoints
    // ========================================================================

    // GET /setup/spotify - Initiates Spotify OAuth flow
    m_server->route(
        "/setup/spotify", QHttpServerRequest::Method::Get,
        [this]()
        {
            emit activityDetected();

            if (!m_spotifyAuth)
            {
                return QString("<html><body><h1>Error</h1><p>Spotify authentication not configured</p></body></html>");
            }

            m_spotifyAuth->startAuthFlow();

            // The auth URL will be emitted via signal -- for the HTML page we provide
            // a button that triggers the standard browser OAuth redirect flow.
            // Since startAuthFlow() triggers the OAuth browser redirect directly,
            // we show a waiting page.
            QString html = QString(
                "<!DOCTYPE html>"
                "<html><head><title>Spotify Setup</title></head>"
                "<body style='font-family: sans-serif; max-width: 600px; margin: 50px auto; text-align: center;'>"
                "<h1>Spotify Authentication</h1>"
                "<p>The Spotify authorization page should open automatically.</p>"
                "<p>If it does not, check the console output for the authorization URL.</p>"
                "<p style='margin-top: 30px; color: #666; font-size: 0.9em;'>You will be redirected to Spotify to "
                "grant permissions</p>"
                "</body></html>");

            return html;
        });

    // GET /auth/spotify/callback - OAuth callback endpoint
    m_server->route(
        "/auth/spotify/callback", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest& request)
        {
            emit activityDetected();

            if (!m_spotifyAuth)
            {
                return QString("<html><body><h1>Error</h1><p>Spotify authentication not configured</p></body></html>");
            }

            QUrl url = request.url();
            QUrlQuery query(url);

            // Check for errors
            if (query.hasQueryItem("error"))
            {
                QString error = query.queryItemValue("error");
                qWarning() << "[API] Spotify OAuth error:" << error;

                return QString("<!DOCTYPE html>"
                               "<html><head><title>Authentication Failed</title></head>"
                               "<body style='font-family: sans-serif; max-width: 600px; margin: 50px auto; "
                               "text-align: center;'>"
                               "<h1>Authentication Failed</h1>"
                               "<p>Error: %1</p>"
                               "<p><a href='/setup/spotify'>Try Again</a></p>"
                               "</body></html>")
                    .arg(error);
            }

            // Extract authorization code
            if (!query.hasQueryItem("code"))
            {
                return QString("<!DOCTYPE html>"
                               "<html><head><title>Authentication Failed</title></head>"
                               "<body style='font-family: sans-serif; max-width: 600px; margin: 50px auto; "
                               "text-align: center;'>"
                               "<h1>Authentication Failed</h1>"
                               "<p>No authorization code received</p>"
                               "</body></html>");
            }

            qDebug() << "[API] Received Spotify authorization code";

            // Token exchange is handled by SpotifyAuth's QOAuthHttpServerReplyHandler on port 8888
            // This callback page just confirms to the user.

            return QString(
                "<!DOCTYPE html>"
                "<html><head><title>Authentication Successful</title></head>"
                "<body style='font-family: sans-serif; max-width: 600px; margin: 50px auto; text-align: center;'>"
                "<h1>Successfully Connected!</h1>"
                "<p>Your Voice of Music system is now connected to Spotify</p>"
                "<p style='margin-top: 30px; color: #666;'>You can close this window</p>"
                "<script>setTimeout(function(){ window.close(); }, 3000);</script>"
                "</body></html>");
        });

    // GET /spotify/status - Check Spotify authentication status
    m_server->route("/spotify/status", QHttpServerRequest::Method::Get,
                    [this]()
                    {
                        emit activityDetected();

                        if (!m_spotifyAuth)
                        {
                            return QJsonObject { { "error", "Spotify not configured" } };
                        }

                        return QJsonObject { { "authenticated", m_spotifyAuth->isAuthenticated() } };
                    });

    // GET /spotify/search - Search Spotify catalog
    m_server->route("/spotify/search", QHttpServerRequest::Method::Get,
                    [this](const QHttpServerRequest& request)
                    {
                        emit activityDetected();

                        if (!m_spotifyAuth || !m_spotifyAuth->isAuthenticated())
                        {
                            return QHttpServerResponse(QJsonObject { { "error", "Not authenticated" } },
                                                       QHttpServerResponder::StatusCode::Unauthorized);
                        }

                        QUrlQuery query(request.url());
                        if (!query.hasQueryItem("q"))
                        {
                            return QHttpServerResponse(QJsonObject { { "error", "Missing 'q' parameter" } },
                                                       QHttpServerResponder::StatusCode::BadRequest);
                        }

                        QString searchQuery = query.queryItemValue("q");
                        if (m_spotifyController)
                        {
                            m_spotifyController->search(searchQuery);
                        }

                        return QHttpServerResponse(QJsonObject { { "status", "ok" }, { "query", searchQuery } });
                    });

    // POST /spotify/play - Resume playback on receiver
    m_server->route("/spotify/play", QHttpServerRequest::Method::Post,
                    [this]()
                    {
                        emit activityDetected();

                        if (!m_spotifyAuth || !m_spotifyAuth->isAuthenticated())
                        {
                            return QJsonObject { { "error", "Not authenticated" } };
                        }

                        if (m_spotifyController)
                        {
                            m_spotifyController->play();
                        }

                        return QJsonObject { { "status", "ok" } };
                    });

    qDebug() << "[API] Routes configured (including Spotify OAuth)";
}

#ifdef QT_SSL_AVAILABLE

bool HttpApiServer::setupSSL()
{
    QString certPath = getCertificatePath();
    QString keyPath = getKeyPath();

    if (QFile::exists(certPath) && QFile::exists(keyPath))
    {
        QFile certFile(certPath);
        if (!certFile.open(QIODevice::ReadOnly))
        {
            qWarning() << "[API] Failed to open certificate file:" << certPath;
            return false;
        }
        m_certificate = QSslCertificate(certFile.readAll());
        certFile.close();

        QFile keyFile(keyPath);
        if (!keyFile.open(QIODevice::ReadOnly))
        {
            qWarning() << "[API] Failed to open key file:" << keyPath;
            return false;
        }
        m_privateKey = QSslKey(keyFile.readAll(), QSsl::Rsa);
        keyFile.close();

        qInfo() << "[API] Loaded existing SSL certificate from" << certPath;
        return true;
    }

    qInfo() << "[API] Generating self-signed certificate...";
    if (!generateSelfSignedCertificate())
    {
        qWarning() << "[API] Failed to generate self-signed certificate";
        return false;
    }

    QFile certFile(certPath);
    if (!certFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "[API] Failed to write certificate file:" << certPath;
        return false;
    }
    certFile.write(m_certificate.toPem());
    certFile.close();
    qInfo() << "[API] Saved certificate to" << certPath;

    QFile keyFile(keyPath);
    if (!keyFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "[API] Failed to write key file:" << keyPath;
        return false;
    }
    keyFile.write(m_privateKey.toPem());
    keyFile.close();
    qInfo() << "[API] Saved private key to" << keyPath;

    return true;
}

bool HttpApiServer::generateSelfSignedCertificate()
{
    QString certPath = getCertificatePath();
    QString keyPath = getKeyPath();

    QDir configDir(QFileInfo(certPath).dir());
    if (!configDir.exists())
    {
        configDir.mkpath(".");
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    QString command = QString("openssl req -x509 -newkey rsa:2048 -keyout %1 -out %2 "
                              "-days 365 -nodes -subj '/CN=127.0.0.1/O=Media Console/C=US'")
                          .arg(keyPath, certPath);

    process.start("/bin/sh", QStringList() << "-c" << command);
    if (!process.waitForFinished(10000))
    {
        qWarning() << "[API] Certificate generation timed out";
        return false;
    }

    if (process.exitCode() != 0)
    {
        qWarning() << "[API] Certificate generation failed:" << process.readAll();
        return false;
    }

    qInfo() << "[API] Successfully generated self-signed certificate";

    QFile certFile(certPath);
    if (!certFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "[API] Failed to open generated certificate";
        return false;
    }
    m_certificate = QSslCertificate(certFile.readAll());
    certFile.close();

    QFile keyFile(keyPath);
    if (!keyFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "[API] Failed to open generated key";
        return false;
    }
    m_privateKey = QSslKey(keyFile.readAll(), QSsl::Rsa);
    keyFile.close();

    return true;
}

QString HttpApiServer::getCertificatePath() const
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return configPath + "/server-cert.pem";
}

QString HttpApiServer::getKeyPath() const
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return configPath + "/server-key.pem";
}

#endif // QT_SSL_AVAILABLE
#endif // HAS_QT_HTTPSERVER
