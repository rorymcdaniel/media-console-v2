#include "SpotifyAuth.h"

#include "app/AppConfig.h"
#include "utils/Logging.h"

SpotifyAuth::SpotifyAuth(const SpotifyConfig& config, QObject* parent)
    : QObject(parent)
{
    // Stub: no setup yet
}

bool SpotifyAuth::isAuthenticated() const
{
    return false;
}

QString SpotifyAuth::accessToken() const
{
    return {};
}

void SpotifyAuth::startAuthFlow() { }

bool SpotifyAuth::restoreTokens()
{
    return false;
}

void SpotifyAuth::saveTokens() { }

void SpotifyAuth::clearTokens() { }

void SpotifyAuth::onGranted() { }

void SpotifyAuth::onRefreshTokenChanged(const QString& refreshToken)
{
    Q_UNUSED(refreshToken)
}

void SpotifyAuth::setupOAuth(const SpotifyConfig& config)
{
    Q_UNUSED(config)
}
