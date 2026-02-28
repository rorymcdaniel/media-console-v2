#include "SpotifyController.h"

#include "app/AppConfig.h"

SpotifyController::SpotifyController(SpotifyAuth* auth, SpotifyClient* client, PlaybackState* playbackState,
                                     UIState* uiState, const SpotifyConfig& config, QObject* parent)
    : QObject(parent)
    , m_auth(auth)
    , m_client(client)
    , m_playbackState(playbackState)
    , m_uiState(uiState)
    , m_desiredDeviceName(config.desiredDeviceName)
{
    // Stub: no signal wiring yet
}

bool SpotifyController::isSpotifyAvailable() const
{
    return false; // Stub
}

bool SpotifyController::isSpotifyActive() const
{
    return m_spotifyActive;
}

QJsonObject SpotifyController::searchResults() const
{
    return m_searchResults;
}

QJsonArray SpotifyController::suggestedPlaylists() const
{
    return m_suggestedPlaylists;
}

void SpotifyController::activateSpotify() { }
void SpotifyController::deactivateSpotify() { }
void SpotifyController::search(const QString& query)
{
    Q_UNUSED(query)
}
void SpotifyController::clearSearch() { } // Stub: does NOT emit signal -- test should fail
void SpotifyController::play() { }
void SpotifyController::pause() { }
void SpotifyController::next() { }
void SpotifyController::previous() { }
void SpotifyController::playTrackUri(const QString& uri)
{
    Q_UNUSED(uri)
}
void SpotifyController::playAlbumUri(const QString& albumUri, int trackIndex)
{
    Q_UNUSED(albumUri)
    Q_UNUSED(trackIndex)
}
void SpotifyController::addToQueue(const QString& trackUri)
{
    Q_UNUSED(trackUri)
}
void SpotifyController::confirmTransfer() { }
void SpotifyController::cancelTransfer() { }
void SpotifyController::loadArtistTopTracks(const QString& artistId)
{
    Q_UNUSED(artistId)
}
void SpotifyController::loadAlbumTracks(const QString& albumId)
{
    Q_UNUSED(albumId)
}
void SpotifyController::loadPlaylistTracks(const QString& playlistId)
{
    Q_UNUSED(playlistId)
}
void SpotifyController::onSearchDebounceTimeout() { }
void SpotifyController::onSearchResultsReady(const QJsonObject& results)
{
    Q_UNUSED(results)
}
void SpotifyController::onDevicesReady(const QJsonArray& devices)
{
    Q_UNUSED(devices)
}
void SpotifyController::onCurrentPlaybackReady(const QJsonObject& playback)
{
    Q_UNUSED(playback)
}
void SpotifyController::onNoActivePlayback() { }
void SpotifyController::onTransferSucceeded() { }
void SpotifyController::onTransferFailed(const QString& error, int statusCode)
{
    Q_UNUSED(error)
    Q_UNUSED(statusCode)
}
void SpotifyController::onPlaybackPollTimeout() { }
void SpotifyController::onPlaybackCommandFailed(const QString& command, const QString& error, int statusCode)
{
    Q_UNUSED(command)
    Q_UNUSED(error)
    Q_UNUSED(statusCode)
}
void SpotifyController::onAuthStateChanged(bool authenticated)
{
    Q_UNUSED(authenticated)
}
void SpotifyController::onUserPlaylistsReady(const QJsonObject& playlists)
{
    Q_UNUSED(playlists)
}
void SpotifyController::onFeaturedPlaylistsReady(const QJsonObject& playlists)
{
    Q_UNUSED(playlists)
}
void SpotifyController::startPlaybackPolling() { }
void SpotifyController::stopPlaybackPolling() { }
void SpotifyController::updatePlaybackStateFromJson(const QJsonObject& playback)
{
    Q_UNUSED(playback)
}
void SpotifyController::findAndTransferToDevice() { }
void SpotifyController::loadSuggestedContent() { }
