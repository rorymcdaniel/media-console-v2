#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QTimer>

class SpotifyAuth;
class SpotifyClient;
class PlaybackState;
class UIState;

struct SpotifyConfig;

class SpotifyController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool spotifyAvailable READ isSpotifyAvailable NOTIFY spotifyAvailableChanged)
    Q_PROPERTY(bool spotifyActive READ isSpotifyActive NOTIFY spotifyActiveChanged)
    Q_PROPERTY(QJsonObject searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QJsonArray suggestedPlaylists READ suggestedPlaylists NOTIFY suggestedPlaylistsChanged)

public:
    SpotifyController(SpotifyAuth* auth, SpotifyClient* client, PlaybackState* playbackState, UIState* uiState,
                      const SpotifyConfig& config, QObject* parent = nullptr);

    bool isSpotifyAvailable() const;
    bool isSpotifyActive() const;
    QJsonObject searchResults() const;
    QJsonArray suggestedPlaylists() const;

public slots:
    void activateSpotify();
    void deactivateSpotify();
    void search(const QString& query);
    void clearSearch();
    void play();
    void pause();
    void next();
    void previous();
    void playTrackUri(const QString& uri);
    void playAlbumUri(const QString& albumUri, int trackIndex = 0);
    void addToQueue(const QString& trackUri);
    void confirmTransfer();
    void cancelTransfer();
    void loadArtistTopTracks(const QString& artistId);
    void loadAlbumTracks(const QString& albumId);
    void loadPlaylistTracks(const QString& playlistId);

signals:
    void spotifyAvailableChanged(bool available);
    void spotifyActiveChanged(bool active);
    void searchResultsChanged(const QJsonObject& results);
    void suggestedPlaylistsChanged(const QJsonArray& playlists);
    void activeSessionDetected(const QString& deviceName, const QString& trackTitle, const QString& artistName);
    void drillDownTracksReady(const QJsonArray& tracks, const QString& title);
    void spotifyError(const QString& message);

private slots:
    void onSearchDebounceTimeout();
    void onSearchResultsReady(const QJsonObject& results);
    void onDevicesReady(const QJsonArray& devices);
    void onCurrentPlaybackReady(const QJsonObject& playback);
    void onNoActivePlayback();
    void onTransferSucceeded();
    void onTransferFailed(const QString& error, int statusCode);
    void onPlaybackPollTimeout();
    void onPlaybackCommandFailed(const QString& command, const QString& error, int statusCode);
    void onAuthStateChanged(bool authenticated);
    void onUserPlaylistsReady(const QJsonObject& playlists);
    void onFeaturedPlaylistsReady(const QJsonObject& playlists);

private:
    void startPlaybackPolling();
    void stopPlaybackPolling();
    void updatePlaybackStateFromJson(const QJsonObject& playback);
    void findAndTransferToDevice();
    void loadSuggestedContent();

    SpotifyAuth* m_auth;
    SpotifyClient* m_client;
    PlaybackState* m_playbackState;
    UIState* m_uiState;
    QString m_desiredDeviceName;

    QTimer m_searchDebounceTimer;
    QString m_pendingSearchQuery;

    QTimer m_playbackPollTimer;
    bool m_spotifyActive = false;

    QString m_pendingTransferDeviceId;

    QJsonObject m_searchResults;
    QJsonArray m_suggestedPlaylists;

    static constexpr int kSearchDebounceMs = 300;
    static constexpr int kPlaybackPollMs = 3000;
};
