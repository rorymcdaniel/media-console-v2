#pragma once

#include <QAbstractOAuthReplyHandler>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

/// Out-of-band OAuth reply handler for CLI use.
/// The user pastes the full redirect URL from their browser into stdin;
/// this handler parses the OAuth params and emits callbackReceived().
class CliOAuthReplyHandler : public QAbstractOAuthReplyHandler
{
public:
    explicit CliOAuthReplyHandler(const QString& redirectUri, QObject* parent = nullptr)
        : QAbstractOAuthReplyHandler(parent)
        , m_redirectUri(redirectUri)
    {
    }

    QString callback() const override { return m_redirectUri; }

    // Token exchange replies are handled internally by QOAuth2AuthorizationCodeFlow
    void networkReplyFinished(QNetworkReply* reply) override { reply->deleteLater(); }

    void handleRedirectUrl(const QUrl& url)
    {
        QVariantMap params;
        const QUrlQuery query(url.query());
        for (const auto& item : query.queryItems(QUrl::FullyDecoded))
            params.insert(item.first, item.second);
        emit callbackReceived(params);
    }

private:
    QString m_redirectUri;
};
