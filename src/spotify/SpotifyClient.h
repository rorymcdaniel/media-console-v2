#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>

#include <functional>

class SpotifyClient : public QObject
{
    Q_OBJECT

public:
    explicit SpotifyClient(QObject* parent = nullptr);

    /// Set the access token for API calls. Called by SpotifyController
    /// whenever SpotifyAuth provides a new token.
    void setAccessToken(const QString& token);

    // --- Search ---
    void search(const QString& query, int limit = 10);

    // --- Devices ---
    void getDevices();

    // --- Playback state ---
    void getCurrentPlayback();

    // --- Playback control ---
    void transferPlayback(const QString& deviceId, bool startPlaying = true);
    void play(const QString& deviceId = QString());
    void playUri(const QString& uri, const QString& deviceId = QString());
    void playContext(const QString& contextUri, int offsetIndex = 0, const QString& deviceId = QString());
    void pause(const QString& deviceId = QString());
    void next(const QString& deviceId = QString());
    void previous(const QString& deviceId = QString());

    // --- Queue ---
    void addToQueue(const QString& trackUri, const QString& deviceId = QString());

    // --- Browse ---
    void getUserPlaylists(int limit = 20);
    void getFeaturedPlaylists(int limit = 20);
    void getPlaylistTracks(const QString& playlistId);
    void getArtistTopTracks(const QString& artistId, const QString& market = "US");
    void getAlbumTracks(const QString& albumId);

    /// Build a full API URL (exposed for testing).
    static QUrl buildUrl(const QString& endpoint, const QList<QPair<QString, QString>>& params = {});

signals:
    // Search results
    void searchResultsReady(const QJsonObject& results);
    void searchFailed(const QString& error, int statusCode);

    // Device list
    void devicesReady(const QJsonArray& devices);
    void devicesFailed(const QString& error, int statusCode);

    // Current playback
    void currentPlaybackReady(const QJsonObject& playback);
    void noActivePlayback(); // 204 response -- nothing playing
    void currentPlaybackFailed(const QString& error, int statusCode);

    // Playback control responses
    void playbackCommandSucceeded(const QString& command);
    void playbackCommandFailed(const QString& command, const QString& error, int statusCode);

    // Transfer
    void transferSucceeded();
    void transferFailed(const QString& error, int statusCode);

    // Queue
    void addToQueueSucceeded();
    void addToQueueFailed(const QString& error, int statusCode);

    // Browse/playlists
    void userPlaylistsReady(const QJsonObject& playlists);
    void featuredPlaylistsReady(const QJsonObject& playlists);
    void playlistTracksReady(const QJsonArray& tracks);
    void artistTopTracksReady(const QJsonArray& tracks);
    void albumTracksReady(const QJsonArray& tracks);
    void browseFailed(const QString& error, int statusCode);

private:
    QNetworkRequest createRequest(const QUrl& url) const;

    /// Handle a reply that returns JSON object (search, playback, playlists).
    void handleJsonObjectReply(QNetworkReply* reply, const std::function<void(const QJsonObject&)>& onSuccess,
                               const std::function<void(const QString&, int)>& onError);

    /// Handle a reply that returns JSON array (tracks, devices).
    void handleJsonArrayReply(QNetworkReply* reply, const QString& arrayKey,
                              const std::function<void(const QJsonArray&)>& onSuccess,
                              const std::function<void(const QString&, int)>& onError);

    /// Handle a reply that returns no content (204) on success (play, pause, etc).
    void handleCommandReply(QNetworkReply* reply, const QString& commandName);

    /// Handle transfer reply (204 success, specific signals).
    void handleTransferReply(QNetworkReply* reply);

    /// Handle add-to-queue reply (204 success, specific signals).
    void handleQueueReply(QNetworkReply* reply);

    /// Handle current playback reply (204 = no active playback, 200 = JSON).
    void handleCurrentPlaybackReply(QNetworkReply* reply);

    QNetworkAccessManager m_networkManager;
    QString m_accessToken;

    static constexpr const char* kBaseUrl = "https://api.spotify.com/v1/";
};
