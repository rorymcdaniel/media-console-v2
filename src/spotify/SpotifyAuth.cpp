#include "SpotifyAuth.h"

#include <QDateTime>
#include <QOAuthHttpServerReplyHandler>
#include <QSettings>

#include "app/AppConfig.h"
#include "spotify/CliOAuthReplyHandler.h"
#include "utils/Logging.h"

SpotifyAuth::SpotifyAuth(const SpotifyConfig& config, QObject* parent)
    : QObject(parent)
{
    setupOAuth(config);
}

void SpotifyAuth::setupOAuth(const SpotifyConfig& config)
{
    m_oauth.setAuthorizationUrl(QUrl(QStringLiteral("https://accounts.spotify.com/authorize")));
    m_oauth.setAccessTokenUrl(QUrl(QStringLiteral("https://accounts.spotify.com/api/token")));
    m_oauth.setClientIdentifier(config.clientId);

    // PKCE S256 -- no client secret needed for public clients
    m_oauth.setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256);

    // Scopes needed for playback control, device management, search, user playlists
    m_oauth.setRequestedScopeTokens({
        QByteArrayLiteral("user-read-playback-state"),
        QByteArrayLiteral("user-modify-playback-state"),
        QByteArrayLiteral("user-read-currently-playing"),
        QByteArrayLiteral("playlist-read-private"),
        QByteArrayLiteral("playlist-read-collaborative"),
    });

    // Auto-refresh 5 minutes (300s) before expiry per SPOT-01
    m_oauth.setAutoRefresh(true);
    m_oauth.setRefreshLeadTime(std::chrono::seconds(300));

    // Localhost redirect handler for OAuth callback
    auto* httpHandler = new QOAuthHttpServerReplyHandler(static_cast<quint16>(config.redirectPort), this);
    httpHandler->setCallbackText(
        QStringLiteral("Authorization complete. You can close this tab and return to your terminal."));
    m_replyHandler = httpHandler;
    m_oauth.setReplyHandler(httpHandler);

    // Connect signals
    connect(&m_oauth, &QAbstractOAuth::authorizeWithBrowser, this, &SpotifyAuth::authorizationUrlReady);
    connect(&m_oauth, &QAbstractOAuth::granted, this, &SpotifyAuth::onGranted);
    connect(&m_oauth, &QAbstractOAuth2::refreshTokenChanged, this, &SpotifyAuth::onRefreshTokenChanged);
    connect(&m_oauth, &QAbstractOAuth::requestFailed, this,
            [this](QAbstractOAuth::Error error)
            {
                const QString msg
                    = QStringLiteral("Token request failed (OAuth error %1)").arg(static_cast<int>(error));
                qCWarning(mediaSpotify) << msg;
                emit authError(msg);
            });

    qCInfo(mediaSpotify) << "SpotifyAuth configured: PKCE S256, auto-refresh 300s lead time, redirect port"
                         << config.redirectPort;
}

bool SpotifyAuth::isAuthenticated() const
{
    return m_authenticated && !m_oauth.token().isEmpty();
}

QString SpotifyAuth::accessToken() const
{
    return m_oauth.token();
}

CliOAuthReplyHandler* SpotifyAuth::useCliReplyHandler(const QString& redirectUri)
{
    if (auto* httpHandler = qobject_cast<QOAuthHttpServerReplyHandler*>(m_replyHandler))
        httpHandler->close();

    m_replyHandler->deleteLater();
    auto* cliHandler = new CliOAuthReplyHandler(redirectUri, this);
    m_replyHandler = cliHandler;
    m_oauth.setReplyHandler(cliHandler);
    qCInfo(mediaSpotify) << "Switched to CLI reply handler, redirect URI:" << redirectUri;
    return cliHandler;
}

void SpotifyAuth::startAuthFlow()
{
    if (auto* httpHandler = qobject_cast<QOAuthHttpServerReplyHandler*>(m_replyHandler))
    {
        if (!httpHandler->isListening())
            qCWarning(mediaSpotify) << "Reply handler not listening, auth flow may fail";
    }

    qCInfo(mediaSpotify) << "Starting OAuth authorization flow";
    m_oauth.grant();
}

bool SpotifyAuth::restoreTokens()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("spotify_auth"));

    const QString accessToken = settings.value(QStringLiteral("access_token")).toString();
    const QString refreshToken = settings.value(QStringLiteral("refresh_token")).toString();
    const QString expiryStr = settings.value(QStringLiteral("expiry")).toString();

    settings.endGroup();

    if (accessToken.isEmpty() && refreshToken.isEmpty())
    {
        qCInfo(mediaSpotify) << "No stored tokens found";
        return false;
    }

    m_oauth.setToken(accessToken);
    m_oauth.setRefreshToken(refreshToken);

    if (!expiryStr.isEmpty())
    {
        const QDateTime expiry = QDateTime::fromString(expiryStr, Qt::ISODate);
        if (expiry.isValid() && expiry <= QDateTime::currentDateTimeUtc() && !refreshToken.isEmpty())
        {
            qCInfo(mediaSpotify) << "Access token expired, attempting refresh";
            m_authenticated = true; // set temporarily so refreshTokens can proceed
            m_oauth.refreshTokens();
            return true;
        }
    }

    if (!accessToken.isEmpty())
    {
        m_authenticated = true;
        qCInfo(mediaSpotify) << "Tokens restored successfully";
        emit authStateChanged(true);
        return true;
    }

    return false;
}

void SpotifyAuth::saveTokens()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("spotify_auth"));
    settings.setValue(QStringLiteral("access_token"), m_oauth.token());
    settings.setValue(QStringLiteral("refresh_token"), m_oauth.refreshToken());
    settings.setValue(QStringLiteral("expiry"), m_oauth.expirationAt().toString(Qt::ISODate));
    settings.endGroup();

    qCDebug(mediaSpotify) << "Tokens saved to QSettings";
}

void SpotifyAuth::clearTokens()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("spotify_auth"));
    settings.remove(QString());
    settings.endGroup();

    m_authenticated = false;
    m_oauth.setToken(QString());
    m_oauth.setRefreshToken(QString());

    qCInfo(mediaSpotify) << "Tokens cleared";
    emit authStateChanged(false);
}

void SpotifyAuth::onGranted()
{
    m_authenticated = true;
    saveTokens();
    qCInfo(mediaSpotify) << "Authorization granted, tokens saved";
    emit authStateChanged(true);
    emit authFlowComplete();

    // Close the HTTP server reply handler after successful auth (no-op for CLI handler)
    if (auto* httpHandler = qobject_cast<QOAuthHttpServerReplyHandler*>(m_replyHandler))
        httpHandler->close();
}

void SpotifyAuth::onRefreshTokenChanged(const QString& refreshToken)
{
    Q_UNUSED(refreshToken)
    // Spotify may rotate refresh tokens; persist the new one immediately
    saveTokens();
    qCInfo(mediaSpotify) << "Refresh token rotated, saved to QSettings";
}
