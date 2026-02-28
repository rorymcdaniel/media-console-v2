#include "SpotifyClient.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrlQuery>

#include "utils/Logging.h"

SpotifyClient::SpotifyClient(QObject* parent)
    : QObject(parent)
{
}

void SpotifyClient::setAccessToken(const QString& token)
{
    m_accessToken = token;
}

QUrl SpotifyClient::buildUrl(const QString& endpoint, const QList<QPair<QString, QString>>& params)
{
    QUrl url(QString::fromLatin1(kBaseUrl) + endpoint);

    if (!params.isEmpty())
    {
        QUrlQuery query;
        for (const auto& param : params)
        {
            query.addQueryItem(param.first, param.second);
        }
        url.setQuery(query);
    }

    return url;
}

QNetworkRequest SpotifyClient::createRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

// --- Search ---

void SpotifyClient::search(const QString& query, int limit)
{
    const QUrl url
        = buildUrl(QStringLiteral("search"),
                   { { "q", query }, { "type", "track,artist,album" }, { "limit", QString::number(limit) } });

    qCDebug(mediaSpotify) << "Search:" << url.toString();

    auto* reply = m_networkManager.get(createRequest(url));
    handleJsonObjectReply(
        reply, [this](const QJsonObject& json) { emit searchResultsReady(json); },
        [this](const QString& error, int code) { emit searchFailed(error, code); });
}

// --- Devices ---

void SpotifyClient::getDevices()
{
    const QUrl url = buildUrl(QStringLiteral("me/player/devices"));

    qCDebug(mediaSpotify) << "GetDevices:" << url.toString();

    auto* reply = m_networkManager.get(createRequest(url));
    handleJsonArrayReply(
        reply, QStringLiteral("devices"), [this](const QJsonArray& devices) { emit devicesReady(devices); },
        [this](const QString& error, int code) { emit devicesFailed(error, code); });
}

// --- Playback state ---

void SpotifyClient::getCurrentPlayback()
{
    const QUrl url = buildUrl(QStringLiteral("me/player"));

    qCDebug(mediaSpotify) << "GetCurrentPlayback:" << url.toString();

    auto* reply = m_networkManager.get(createRequest(url));
    handleCurrentPlaybackReply(reply);
}

// --- Playback control ---

void SpotifyClient::transferPlayback(const QString& deviceId, bool startPlaying)
{
    const QUrl url = buildUrl(QStringLiteral("me/player"));

    QJsonObject body;
    body[QStringLiteral("device_ids")] = QJsonArray { deviceId };
    body[QStringLiteral("play")] = startPlaying;

    qCDebug(mediaSpotify) << "TransferPlayback to device:" << deviceId;

    auto* reply = m_networkManager.put(createRequest(url), QJsonDocument(body).toJson());
    handleTransferReply(reply);
}

void SpotifyClient::play(const QString& deviceId)
{
    QList<QPair<QString, QString>> params;
    if (!deviceId.isEmpty())
    {
        params.append({ "device_id", deviceId });
    }
    const QUrl url = buildUrl(QStringLiteral("me/player/play"), params);

    qCDebug(mediaSpotify) << "Play:" << url.toString();

    auto* reply = m_networkManager.put(createRequest(url), QByteArray());
    handleCommandReply(reply, QStringLiteral("play"));
}

void SpotifyClient::playUri(const QString& uri, const QString& deviceId)
{
    QList<QPair<QString, QString>> params;
    if (!deviceId.isEmpty())
    {
        params.append({ "device_id", deviceId });
    }
    const QUrl url = buildUrl(QStringLiteral("me/player/play"), params);

    QJsonObject body;
    body[QStringLiteral("uris")] = QJsonArray { uri };

    qCDebug(mediaSpotify) << "PlayUri:" << uri;

    auto* reply = m_networkManager.put(createRequest(url), QJsonDocument(body).toJson());
    handleCommandReply(reply, QStringLiteral("playUri"));
}

void SpotifyClient::playContext(const QString& contextUri, int offsetIndex, const QString& deviceId)
{
    QList<QPair<QString, QString>> params;
    if (!deviceId.isEmpty())
    {
        params.append({ "device_id", deviceId });
    }
    const QUrl url = buildUrl(QStringLiteral("me/player/play"), params);

    QJsonObject offset;
    offset[QStringLiteral("position")] = offsetIndex;

    QJsonObject body;
    body[QStringLiteral("context_uri")] = contextUri;
    body[QStringLiteral("offset")] = offset;

    qCDebug(mediaSpotify) << "PlayContext:" << contextUri << "offset:" << offsetIndex;

    auto* reply = m_networkManager.put(createRequest(url), QJsonDocument(body).toJson());
    handleCommandReply(reply, QStringLiteral("playContext"));
}

void SpotifyClient::pause(const QString& deviceId)
{
    QList<QPair<QString, QString>> params;
    if (!deviceId.isEmpty())
    {
        params.append({ "device_id", deviceId });
    }
    const QUrl url = buildUrl(QStringLiteral("me/player/pause"), params);

    qCDebug(mediaSpotify) << "Pause:" << url.toString();

    auto* reply = m_networkManager.put(createRequest(url), QByteArray());
    handleCommandReply(reply, QStringLiteral("pause"));
}

void SpotifyClient::next(const QString& deviceId)
{
    QList<QPair<QString, QString>> params;
    if (!deviceId.isEmpty())
    {
        params.append({ "device_id", deviceId });
    }
    const QUrl url = buildUrl(QStringLiteral("me/player/next"), params);

    qCDebug(mediaSpotify) << "Next:" << url.toString();

    auto* reply = m_networkManager.sendCustomRequest(createRequest(url), "POST");
    handleCommandReply(reply, QStringLiteral("next"));
}

void SpotifyClient::previous(const QString& deviceId)
{
    QList<QPair<QString, QString>> params;
    if (!deviceId.isEmpty())
    {
        params.append({ "device_id", deviceId });
    }
    const QUrl url = buildUrl(QStringLiteral("me/player/previous"), params);

    qCDebug(mediaSpotify) << "Previous:" << url.toString();

    auto* reply = m_networkManager.sendCustomRequest(createRequest(url), "POST");
    handleCommandReply(reply, QStringLiteral("previous"));
}

// --- Queue ---

void SpotifyClient::addToQueue(const QString& trackUri, const QString& deviceId)
{
    QList<QPair<QString, QString>> params = { { "uri", trackUri } };
    if (!deviceId.isEmpty())
    {
        params.append({ "device_id", deviceId });
    }
    const QUrl url = buildUrl(QStringLiteral("me/player/queue"), params);

    qCDebug(mediaSpotify) << "AddToQueue:" << trackUri;

    auto* reply = m_networkManager.sendCustomRequest(createRequest(url), "POST");
    handleQueueReply(reply);
}

// --- Browse ---

void SpotifyClient::getUserPlaylists(int limit)
{
    const QUrl url = buildUrl(QStringLiteral("me/playlists"), { { "limit", QString::number(limit) } });

    qCDebug(mediaSpotify) << "GetUserPlaylists:" << url.toString();

    auto* reply = m_networkManager.get(createRequest(url));
    handleJsonObjectReply(
        reply, [this](const QJsonObject& json) { emit userPlaylistsReady(json); },
        [this](const QString& error, int code) { emit browseFailed(error, code); });
}

void SpotifyClient::getFeaturedPlaylists(int limit)
{
    const QUrl url = buildUrl(QStringLiteral("browse/featured-playlists"), { { "limit", QString::number(limit) } });

    qCDebug(mediaSpotify) << "GetFeaturedPlaylists:" << url.toString();

    auto* reply = m_networkManager.get(createRequest(url));
    handleJsonObjectReply(
        reply, [this](const QJsonObject& json) { emit featuredPlaylistsReady(json); },
        [this](const QString& error, int code) { emit browseFailed(error, code); });
}

void SpotifyClient::getPlaylistTracks(const QString& playlistId)
{
    const QUrl url = buildUrl(QStringLiteral("playlists/") + playlistId + QStringLiteral("/tracks"));

    qCDebug(mediaSpotify) << "GetPlaylistTracks:" << playlistId;

    auto* reply = m_networkManager.get(createRequest(url));
    handleJsonArrayReply(
        reply, QStringLiteral("items"), [this](const QJsonArray& tracks) { emit playlistTracksReady(tracks); },
        [this](const QString& error, int code) { emit browseFailed(error, code); });
}

void SpotifyClient::getArtistTopTracks(const QString& artistId, const QString& market)
{
    const QUrl url
        = buildUrl(QStringLiteral("artists/") + artistId + QStringLiteral("/top-tracks"), { { "market", market } });

    qCDebug(mediaSpotify) << "GetArtistTopTracks:" << artistId;

    auto* reply = m_networkManager.get(createRequest(url));
    handleJsonArrayReply(
        reply, QStringLiteral("tracks"), [this](const QJsonArray& tracks) { emit artistTopTracksReady(tracks); },
        [this](const QString& error, int code) { emit browseFailed(error, code); });
}

void SpotifyClient::getAlbumTracks(const QString& albumId)
{
    const QUrl url = buildUrl(QStringLiteral("albums/") + albumId + QStringLiteral("/tracks"));

    qCDebug(mediaSpotify) << "GetAlbumTracks:" << albumId;

    auto* reply = m_networkManager.get(createRequest(url));
    handleJsonArrayReply(
        reply, QStringLiteral("items"), [this](const QJsonArray& tracks) { emit albumTracksReady(tracks); },
        [this](const QString& error, int code) { emit browseFailed(error, code); });
}

// --- Reply handlers ---

void SpotifyClient::handleJsonObjectReply(QNetworkReply* reply,
                                          const std::function<void(const QJsonObject&)>& onSuccess,
                                          const std::function<void(const QString&, int)>& onError)
{
    connect(reply, &QNetworkReply::finished, this,
            [reply, onSuccess, onError]()
            {
                reply->deleteLater();

                const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qCWarning(mediaSpotify) << "API error:" << statusCode << reply->errorString();
                    onError(reply->errorString(), statusCode);
                    return;
                }

                const QByteArray data = reply->readAll();
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError)
                {
                    qCWarning(mediaSpotify) << "JSON parse error:" << parseError.errorString();
                    onError(QStringLiteral("JSON parse error: ") + parseError.errorString(), statusCode);
                    return;
                }

                onSuccess(doc.object());
            });
}

void SpotifyClient::handleJsonArrayReply(QNetworkReply* reply, const QString& arrayKey,
                                         const std::function<void(const QJsonArray&)>& onSuccess,
                                         const std::function<void(const QString&, int)>& onError)
{
    connect(reply, &QNetworkReply::finished, this,
            [reply, arrayKey, onSuccess, onError]()
            {
                reply->deleteLater();

                const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qCWarning(mediaSpotify) << "API error:" << statusCode << reply->errorString();
                    onError(reply->errorString(), statusCode);
                    return;
                }

                const QByteArray data = reply->readAll();
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError)
                {
                    qCWarning(mediaSpotify) << "JSON parse error:" << parseError.errorString();
                    onError(QStringLiteral("JSON parse error: ") + parseError.errorString(), statusCode);
                    return;
                }

                const QJsonObject obj = doc.object();
                onSuccess(obj.value(arrayKey).toArray());
            });
}

void SpotifyClient::handleCommandReply(QNetworkReply* reply, const QString& commandName)
{
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, commandName]()
            {
                reply->deleteLater();

                const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qCWarning(mediaSpotify)
                        << "Command" << commandName << "failed:" << statusCode << reply->errorString();
                    emit playbackCommandFailed(commandName, reply->errorString(), statusCode);
                    return;
                }

                qCDebug(mediaSpotify) << "Command" << commandName << "succeeded";
                emit playbackCommandSucceeded(commandName);
            });
}

void SpotifyClient::handleTransferReply(QNetworkReply* reply)
{
    connect(reply, &QNetworkReply::finished, this,
            [this, reply]()
            {
                reply->deleteLater();

                const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qCWarning(mediaSpotify) << "Transfer failed:" << statusCode << reply->errorString();
                    emit transferFailed(reply->errorString(), statusCode);
                    return;
                }

                qCDebug(mediaSpotify) << "Transfer succeeded";
                emit transferSucceeded();
            });
}

void SpotifyClient::handleQueueReply(QNetworkReply* reply)
{
    connect(reply, &QNetworkReply::finished, this,
            [this, reply]()
            {
                reply->deleteLater();

                const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qCWarning(mediaSpotify) << "Add to queue failed:" << statusCode << reply->errorString();
                    emit addToQueueFailed(reply->errorString(), statusCode);
                    return;
                }

                qCDebug(mediaSpotify) << "Add to queue succeeded";
                emit addToQueueSucceeded();
            });
}

void SpotifyClient::handleCurrentPlaybackReply(QNetworkReply* reply)
{
    connect(reply, &QNetworkReply::finished, this,
            [this, reply]()
            {
                reply->deleteLater();

                const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                // 204 = no active playback
                if (statusCode == 204)
                {
                    qCDebug(mediaSpotify) << "No active playback (204)";
                    emit noActivePlayback();
                    return;
                }

                if (reply->error() != QNetworkReply::NoError)
                {
                    qCWarning(mediaSpotify) << "GetCurrentPlayback failed:" << statusCode << reply->errorString();
                    emit currentPlaybackFailed(reply->errorString(), statusCode);
                    return;
                }

                const QByteArray data = reply->readAll();
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError)
                {
                    qCWarning(mediaSpotify) << "JSON parse error:" << parseError.errorString();
                    emit currentPlaybackFailed(QStringLiteral("JSON parse error: ") + parseError.errorString(),
                                               statusCode);
                    return;
                }

                emit currentPlaybackReady(doc.object());
            });
}
