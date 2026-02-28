#include "SpotifyController.h"

#include <QJsonArray>
#include <QJsonObject>

#include "app/AppConfig.h"
#include "spotify/SpotifyAuth.h"
#include "spotify/SpotifyClient.h"
#include "state/PlaybackState.h"
#include "state/UIState.h"
#include "utils/Logging.h"

SpotifyController::SpotifyController(SpotifyAuth* auth, SpotifyClient* client, PlaybackState* playbackState,
                                     UIState* uiState, const SpotifyConfig& config, QObject* parent)
    : QObject(parent)
    , m_auth(auth)
    , m_client(client)
    , m_playbackState(playbackState)
    , m_uiState(uiState)
    , m_desiredDeviceName(config.desiredDeviceName)
{
    // Configure search debounce timer: single-shot, fires after 300ms
    m_searchDebounceTimer.setSingleShot(true);
    m_searchDebounceTimer.setInterval(kSearchDebounceMs);
    connect(&m_searchDebounceTimer, &QTimer::timeout, this, &SpotifyController::onSearchDebounceTimeout);

    // Configure playback poll timer
    m_playbackPollTimer.setInterval(kPlaybackPollMs);
    connect(&m_playbackPollTimer, &QTimer::timeout, this, &SpotifyController::onPlaybackPollTimeout);

    // Wire SpotifyClient signals (only if client is non-null)
    if (m_client)
    {
        connect(m_client, &SpotifyClient::searchResultsReady, this, &SpotifyController::onSearchResultsReady);
        connect(m_client, &SpotifyClient::devicesReady, this, &SpotifyController::onDevicesReady);
        connect(m_client, &SpotifyClient::currentPlaybackReady, this, &SpotifyController::onCurrentPlaybackReady);
        connect(m_client, &SpotifyClient::noActivePlayback, this, &SpotifyController::onNoActivePlayback);
        connect(m_client, &SpotifyClient::transferSucceeded, this, &SpotifyController::onTransferSucceeded);
        connect(m_client, &SpotifyClient::transferFailed, this, &SpotifyController::onTransferFailed);
        connect(m_client, &SpotifyClient::playbackCommandFailed, this, &SpotifyController::onPlaybackCommandFailed);
        connect(m_client, &SpotifyClient::userPlaylistsReady, this, &SpotifyController::onUserPlaylistsReady);
        connect(m_client, &SpotifyClient::featuredPlaylistsReady, this, &SpotifyController::onFeaturedPlaylistsReady);

        // Drill-down: artist top tracks, album tracks, playlist tracks
        connect(m_client, &SpotifyClient::artistTopTracksReady, this,
                [this](const QJsonArray& tracks) { emit drillDownTracksReady(tracks, QStringLiteral("Top Tracks")); });
        connect(m_client, &SpotifyClient::albumTracksReady, this, [this](const QJsonArray& tracks)
                { emit drillDownTracksReady(tracks, QStringLiteral("Album Tracks")); });
        connect(m_client, &SpotifyClient::playlistTracksReady, this, [this](const QJsonArray& tracks)
                { emit drillDownTracksReady(tracks, QStringLiteral("Playlist Tracks")); });
    }

    // Wire SpotifyAuth signals (only if auth is non-null)
    if (m_auth)
    {
        connect(m_auth, &SpotifyAuth::authStateChanged, this, &SpotifyController::onAuthStateChanged);

        // Set initial token if already authenticated
        if (m_auth->isAuthenticated() && m_client)
        {
            m_client->setAccessToken(m_auth->accessToken());
        }
    }

    qCInfo(mediaSpotify) << "SpotifyController initialized, desired device:" << m_desiredDeviceName;
}

bool SpotifyController::isSpotifyAvailable() const
{
    return m_auth && m_auth->isAuthenticated();
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

// --- Source activation ---

void SpotifyController::activateSpotify()
{
    if (!m_auth || !m_auth->isAuthenticated())
    {
        qCWarning(mediaSpotify) << "Spotify not authenticated, cannot activate";
        if (m_uiState)
        {
            emit m_uiState->showToast(QStringLiteral("Spotify not configured \u2014 run media-console --spotify-auth"),
                                      QStringLiteral("error"));
        }
        return;
    }

    m_spotifyActive = true;
    emit spotifyActiveChanged(true);

    // Update client access token from auth
    if (m_client)
    {
        m_client->setAccessToken(m_auth->accessToken());

        // Check for active session on another device
        m_client->getCurrentPlayback();
    }

    // Load suggested content for the search view
    loadSuggestedContent();

    qCInfo(mediaSpotify) << "Spotify activated";
}

void SpotifyController::deactivateSpotify()
{
    m_spotifyActive = false;
    emit spotifyActiveChanged(false);
    stopPlaybackPolling();

    qCInfo(mediaSpotify) << "Spotify deactivated";
}

// --- Search ---

void SpotifyController::search(const QString& query)
{
    m_pendingSearchQuery = query;
    // Restart debounce timer -- any pending timeout is canceled, fresh 300ms starts
    m_searchDebounceTimer.start();
}

void SpotifyController::clearSearch()
{
    m_searchDebounceTimer.stop();
    m_pendingSearchQuery.clear();
    m_searchResults = QJsonObject();
    emit searchResultsChanged(m_searchResults);
}

void SpotifyController::onSearchDebounceTimeout()
{
    if (m_pendingSearchQuery.isEmpty())
    {
        return;
    }

    if (m_client)
    {
        qCDebug(mediaSpotify) << "Search debounce fired:" << m_pendingSearchQuery;
        m_client->search(m_pendingSearchQuery, 10);
    }
}

void SpotifyController::onSearchResultsReady(const QJsonObject& results)
{
    m_searchResults = results;
    emit searchResultsChanged(m_searchResults);
}

// --- Playback control ---

void SpotifyController::play()
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Play requested";
        m_client->play();
    }
}

void SpotifyController::pause()
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Pause requested";
        m_client->pause();
    }
}

void SpotifyController::next()
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Next track requested";
        m_client->next();
    }
}

void SpotifyController::previous()
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Previous track requested";
        m_client->previous();
    }
}

void SpotifyController::playTrackUri(const QString& uri)
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Play track URI:" << uri;
        m_client->playUri(uri);
    }
}

void SpotifyController::playAlbumUri(const QString& albumUri, int trackIndex)
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Play album URI:" << albumUri << "track:" << trackIndex;
        m_client->playContext(albumUri, trackIndex);
    }
}

void SpotifyController::addToQueue(const QString& trackUri)
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Add to queue:" << trackUri;
        m_client->addToQueue(trackUri);
    }
}

// --- Session takeover ---

void SpotifyController::onCurrentPlaybackReady(const QJsonObject& playback)
{
    const QJsonObject device = playback.value(QStringLiteral("device")).toObject();
    const QString deviceName = device.value(QStringLiteral("name")).toString();
    const bool isPlaying = playback.value(QStringLiteral("is_playing")).toBool();

    if (isPlaying && deviceName != m_desiredDeviceName)
    {
        // Active session on another device -- prompt user for takeover
        const QJsonObject item = playback.value(QStringLiteral("item")).toObject();
        const QString trackTitle = item.value(QStringLiteral("name")).toString();

        const QJsonArray artists = item.value(QStringLiteral("artists")).toArray();
        const QString artistName = artists.isEmpty()
            ? QStringLiteral("Unknown Artist")
            : artists.first().toObject().value(QStringLiteral("name")).toString();

        // Store the device ID for potential transfer
        m_pendingTransferDeviceId.clear(); // Transfer TO our device, not FROM theirs

        qCInfo(mediaSpotify) << "Active session detected on" << deviceName << "playing" << trackTitle << "by"
                             << artistName;

        emit activeSessionDetected(deviceName, trackTitle, artistName);
    }
    else if (deviceName == m_desiredDeviceName)
    {
        // Already playing on our device -- start polling
        updatePlaybackStateFromJson(playback);
        startPlaybackPolling();
    }
    else
    {
        // Playing on our device or no specific mismatch -- proceed normally
        updatePlaybackStateFromJson(playback);
        startPlaybackPolling();
    }
}

void SpotifyController::onNoActivePlayback()
{
    qCInfo(mediaSpotify) << "No active playback, finding and transferring to device";
    findAndTransferToDevice();
}

void SpotifyController::confirmTransfer()
{
    // User confirmed takeover -- transfer playback to our desired device
    qCInfo(mediaSpotify) << "User confirmed transfer, finding target device";
    findAndTransferToDevice();
}

void SpotifyController::cancelTransfer()
{
    m_pendingTransferDeviceId.clear();
    qCInfo(mediaSpotify) << "User canceled transfer";
}

void SpotifyController::findAndTransferToDevice()
{
    if (m_client)
    {
        m_client->getDevices();
    }
}

void SpotifyController::onDevicesReady(const QJsonArray& devices)
{
    // Find the device matching m_desiredDeviceName
    for (const QJsonValue& val : devices)
    {
        const QJsonObject device = val.toObject();
        const QString name = device.value(QStringLiteral("name")).toString();

        if (name == m_desiredDeviceName)
        {
            const QString deviceId = device.value(QStringLiteral("id")).toString();
            qCInfo(mediaSpotify) << "Found target device:" << name << "id:" << deviceId;

            if (m_client)
            {
                m_client->transferPlayback(deviceId, false);
            }
            return;
        }
    }

    // Device not found
    const QString errorMsg
        = QStringLiteral("Device '%1' not found. Ensure the receiver is on and connected.").arg(m_desiredDeviceName);
    qCWarning(mediaSpotify) << errorMsg;
    emit spotifyError(errorMsg);
}

void SpotifyController::onTransferSucceeded()
{
    qCInfo(mediaSpotify) << "Playback transferred successfully";
    startPlaybackPolling();
}

void SpotifyController::onTransferFailed(const QString& error, int statusCode)
{
    if (statusCode == 403)
    {
        emit spotifyError(QStringLiteral("Spotify Premium required for playback control"));
    }
    else
    {
        emit spotifyError(QStringLiteral("Transfer failed: %1").arg(error));
    }
    qCWarning(mediaSpotify) << "Transfer failed:" << error << "status:" << statusCode;
}

// --- Playback polling ---

void SpotifyController::startPlaybackPolling()
{
    if (!m_playbackPollTimer.isActive())
    {
        m_playbackPollTimer.start();
        qCDebug(mediaSpotify) << "Playback polling started (every" << kPlaybackPollMs << "ms)";
    }
}

void SpotifyController::stopPlaybackPolling()
{
    if (m_playbackPollTimer.isActive())
    {
        m_playbackPollTimer.stop();
        qCDebug(mediaSpotify) << "Playback polling stopped";
    }
}

void SpotifyController::onPlaybackPollTimeout()
{
    if (!m_spotifyActive)
    {
        stopPlaybackPolling();
        return;
    }

    if (m_client)
    {
        m_client->getCurrentPlayback();
    }
}

void SpotifyController::updatePlaybackStateFromJson(const QJsonObject& playback)
{
    if (!m_playbackState)
    {
        return;
    }

    const QJsonObject item = playback.value(QStringLiteral("item")).toObject();
    if (item.isEmpty())
    {
        return;
    }

    const QString title = item.value(QStringLiteral("name")).toString();

    const QJsonArray artists = item.value(QStringLiteral("artists")).toArray();
    const QString artist
        = artists.isEmpty() ? QString() : artists.first().toObject().value(QStringLiteral("name")).toString();

    const QJsonObject album = item.value(QStringLiteral("album")).toObject();
    const QString albumName = album.value(QStringLiteral("name")).toString();

    const qint64 durationMs = item.value(QStringLiteral("duration_ms")).toInteger();
    const qint64 progressMs = playback.value(QStringLiteral("progress_ms")).toInteger();
    const bool isPlaying = playback.value(QStringLiteral("is_playing")).toBool();

    m_playbackState->setTitle(title);
    m_playbackState->setArtist(artist);
    m_playbackState->setAlbum(albumName);
    m_playbackState->setDurationMs(durationMs);
    m_playbackState->setPositionMs(progressMs);
    m_playbackState->setPlaybackMode(isPlaying ? PlaybackMode::Playing : PlaybackMode::Paused);

    // NOTE: Do NOT set albumArtUrl here. For active playback, the receiver provides
    // album art via its CGI endpoint (NJA2 parsing in ReceiverController). Only
    // Spotify API image URLs are used for search result thumbnails (SPOT-08).
}

// --- Error handling ---

void SpotifyController::onPlaybackCommandFailed(const QString& command, const QString& error, int statusCode)
{
    Q_UNUSED(command)

    if (statusCode == 403)
    {
        if (m_uiState)
        {
            emit m_uiState->showToast(QStringLiteral("Spotify Premium required"), QStringLiteral("error"));
        }
    }
    else if (statusCode == 429)
    {
        if (m_uiState)
        {
            emit m_uiState->showToast(QStringLiteral("Too many requests, try again"), QStringLiteral("warning"));
        }
    }
    else
    {
        if (m_uiState)
        {
            emit m_uiState->showToast(error, QStringLiteral("error"));
        }
    }

    qCWarning(mediaSpotify) << "Playback command failed:" << command << error << statusCode;
}

// --- Auth state changes ---

void SpotifyController::onAuthStateChanged(bool authenticated)
{
    emit spotifyAvailableChanged(authenticated);

    if (authenticated && m_client && m_auth)
    {
        m_client->setAccessToken(m_auth->accessToken());
        qCInfo(mediaSpotify) << "Auth state changed: authenticated, token updated";
    }
    else if (!authenticated && m_spotifyActive)
    {
        deactivateSpotify();
        if (m_uiState)
        {
            emit m_uiState->showToast(QStringLiteral("Spotify session expired"), QStringLiteral("error"));
        }
        qCWarning(mediaSpotify) << "Auth state changed: not authenticated, Spotify deactivated";
    }
}

// --- Browse / suggested content ---

void SpotifyController::loadSuggestedContent()
{
    if (m_client)
    {
        // Try featured playlists first; fallback to user playlists on 403
        m_client->getFeaturedPlaylists();
    }
}

void SpotifyController::onFeaturedPlaylistsReady(const QJsonObject& playlists)
{
    const QJsonObject playlistsObj = playlists.value(QStringLiteral("playlists")).toObject();
    const QJsonArray items = playlistsObj.value(QStringLiteral("items")).toArray();

    m_suggestedPlaylists = items;
    emit suggestedPlaylistsChanged(m_suggestedPlaylists);

    qCDebug(mediaSpotify) << "Featured playlists loaded:" << items.size();
}

void SpotifyController::onUserPlaylistsReady(const QJsonObject& playlists)
{
    const QJsonArray items = playlists.value(QStringLiteral("items")).toArray();
    m_suggestedPlaylists = items;
    emit suggestedPlaylistsChanged(m_suggestedPlaylists);

    qCDebug(mediaSpotify) << "User playlists loaded:" << items.size();
}

// --- Browse drill-down ---

void SpotifyController::loadArtistTopTracks(const QString& artistId)
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Loading artist top tracks:" << artistId;
        m_client->getArtistTopTracks(artistId);
    }
}

void SpotifyController::loadAlbumTracks(const QString& albumId)
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Loading album tracks:" << albumId;
        m_client->getAlbumTracks(albumId);
    }
}

void SpotifyController::loadPlaylistTracks(const QString& playlistId)
{
    if (m_client)
    {
        qCDebug(mediaSpotify) << "Loading playlist tracks:" << playlistId;
        m_client->getPlaylistTracks(playlistId);
    }
}
