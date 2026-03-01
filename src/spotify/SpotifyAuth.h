#pragma once

#include <QAbstractOAuthReplyHandler>
#include <QOAuth2AuthorizationCodeFlow>
#include <QObject>

class CliOAuthReplyHandler;
struct SpotifyConfig;

class SpotifyAuth : public QObject
{
    Q_OBJECT

public:
    explicit SpotifyAuth(const SpotifyConfig& config, QObject* parent = nullptr);

    bool isAuthenticated() const;
    QString accessToken() const;
    void startAuthFlow();
    CliOAuthReplyHandler* useCliReplyHandler(const QString& redirectUri);
    bool restoreTokens();
    void saveTokens();
    void clearTokens();

signals:
    void authStateChanged(bool authenticated);
    void authorizationUrlReady(const QUrl& url);
    void authFlowComplete();
    void authError(const QString& error);

private slots:
    void onGranted();
    void onRefreshTokenChanged(const QString& refreshToken);

private:
    void setupOAuth(const SpotifyConfig& config);

    QOAuth2AuthorizationCodeFlow m_oauth;
    QAbstractOAuthReplyHandler* m_replyHandler = nullptr;
    bool m_authenticated = false;
};
