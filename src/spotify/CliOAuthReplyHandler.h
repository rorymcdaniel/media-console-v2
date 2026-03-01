#pragma once

#include <QAbstractOAuthReplyHandler>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRestReply>
#include <QUrl>
#include <QUrlQuery>

using namespace Qt::StringLiterals;

/// Out-of-band OAuth reply handler for CLI use.
/// The user pastes the full redirect URL from their browser into stdin;
/// this handler parses the OAuth params and emits callbackReceived().
///
/// networkReplyFinished() is modeled on QOAuthOobReplyHandler — it reads
/// the token response body, parses JSON or form-encoded data, and emits
/// tokensReceived() so the OAuth flow can complete.
class CliOAuthReplyHandler : public QAbstractOAuthReplyHandler
{
public:
    explicit CliOAuthReplyHandler(const QString& redirectUri, QObject* parent = nullptr)
        : QAbstractOAuthReplyHandler(parent)
        , m_redirectUri(redirectUri)
    {
    }

    QString callback() const override { return m_redirectUri; }

    void networkReplyFinished(QNetworkReply* reply) override
    {
        QRestReply restReply(reply);

        if (restReply.hasError())
        {
            emit tokenRequestErrorOccurred(QAbstractOAuth::Error::NetworkError, reply->errorString());
            return;
        }
        if (!restReply.isHttpStatusSuccess())
        {
            const QByteArray body = reply->readAll();
            emit tokenRequestErrorOccurred(QAbstractOAuth::Error::ServerError, QString::fromUtf8(body));
            return;
        }

        const QByteArray data = reply->readAll();
        emit replyDataReceived(data);

        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();

        QVariantMap tokens;
        if (contentType.startsWith("application/json"_L1) || contentType.startsWith("text/javascript"_L1))
        {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
            {
                emit tokenRequestErrorOccurred(QAbstractOAuth::Error::ServerError, u"Response is not a JSON object"_s);
                return;
            }
            tokens = doc.object().toVariantMap();
        }
        else
        {
            // form-encoded fallback
            const QUrlQuery query(QString::fromUtf8(data));
            for (const auto& item : query.queryItems(QUrl::FullyDecoded))
                tokens.insert(item.first, item.second);
        }

        emit tokensReceived(tokens);
    }

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
