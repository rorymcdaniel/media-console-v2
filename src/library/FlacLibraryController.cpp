#include "FlacLibraryController.h"

#include <QSet>
#include <QStandardPaths>

#include "app/AppConfig.h"
#include "audio/LocalPlaybackController.h"
#include "library/FlacAudioStream.h"
#include "library/LibraryAlbumArtProvider.h"
#include "library/LibraryAlbumModel.h"
#include "library/LibraryArtistModel.h"
#include "library/LibraryScanner.h"
#include "library/LibraryTrackModel.h"
#include "state/PlaybackState.h"
#include "utils/Logging.h"

FlacLibraryController::FlacLibraryController(LocalPlaybackController* playbackController, PlaybackState* playbackState,
                                             const LibraryConfig& config, QObject* parent)
    : QObject(parent)
    , m_playbackController(playbackController)
    , m_playbackState(playbackState)
    , m_rootPath(config.rootPath)
{
    // Create owned components
    m_database = std::make_unique<LibraryDatabase>(this);
    m_scanner = std::make_unique<LibraryScanner>(this);

    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/album-art";
    m_artProvider = std::make_unique<LibraryAlbumArtProvider>(cacheDir, this);

    m_artistModel = std::make_unique<LibraryArtistModel>(m_database.get(), this);
    m_albumModel = std::make_unique<LibraryAlbumModel>(m_database.get(), m_artProvider.get(), this);
    m_trackModel = std::make_unique<LibraryTrackModel>(m_database.get(), this);

    // Wire scanner signals
    connect(m_scanner.get(), &LibraryScanner::batchReady, this, &FlacLibraryController::onBatchReady);
    connect(m_scanner.get(), &LibraryScanner::scanComplete, this, &FlacLibraryController::onScanComplete);
    connect(m_scanner.get(), &LibraryScanner::scanProgress, this, &FlacLibraryController::scanProgress);

    // Wire playback finished
    if (m_playbackController)
    {
        connect(m_playbackController, &LocalPlaybackController::trackFinished, this,
                &FlacLibraryController::onTrackFinished);
    }
}

FlacLibraryController::~FlacLibraryController()
{
    stop();
}

void FlacLibraryController::start()
{
    // Open database
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/library.db";
    if (!m_database->open(dbPath))
    {
        qCWarning(mediaLibrary) << "FlacLibraryController: failed to open database:" << dbPath;
        return;
    }

    qCInfo(mediaLibrary) << "FlacLibraryController: started with root" << m_rootPath;

    // Refresh models with existing data (library browsable immediately)
    m_artistModel->refresh();

    // Start incremental scan
    startScan();
}

void FlacLibraryController::stop()
{
    if (m_scanner)
    {
        m_scanner->cancel();
    }

    // Clear playlist
    m_playlist.clear();
    m_currentIndex = -1;
}

void FlacLibraryController::startScan()
{
    QMap<QString, qint64> existingMtimes = m_database->getAllMtimes();
    m_scanner->startScan(m_rootPath, existingMtimes);
}

void FlacLibraryController::onBatchReady(const QVector<LibraryTrack>& tracks)
{
    // Extract art for each unique album in the batch
    QSet<QString> processedAlbums;
    for (const auto& track : tracks)
    {
        QString key = track.albumArtist + "\x1F" + track.album;
        if (!processedAlbums.contains(key))
        {
            processedAlbums.insert(key);
            if (!m_artProvider->hasCachedArt(track.albumArtist, track.album))
            {
                m_artProvider->extractArt(track.filePath, track.albumArtist, track.album);
            }
        }
    }

    // Insert tracks into database
    m_database->upsertTrackBatch(tracks);

    // Refresh artist model (albums and tracks refresh on filter change)
    m_artistModel->refresh();
}

void FlacLibraryController::onScanComplete(int totalProcessed, int totalSkipped)
{
    qCInfo(mediaLibrary) << "FlacLibraryController: scan complete. Processed:" << totalProcessed
                         << "Skipped:" << totalSkipped;

    // Final model refresh
    m_artistModel->refresh();

    emit libraryReady();
    emit scanComplete(totalProcessed, totalSkipped);
}

void FlacLibraryController::playTrack(int trackModelIndex)
{
    if (!m_playbackController || !m_playbackState)
    {
        qCWarning(mediaLibrary) << "FlacLibraryController: no playback controller or state";
        return;
    }

    // Load album track list from track model
    m_playlist = m_trackModel->allTracks();

    if (trackModelIndex < 0 || trackModelIndex >= m_playlist.size())
    {
        qCWarning(mediaLibrary) << "FlacLibraryController: invalid track index:" << trackModelIndex;
        return;
    }

    m_currentIndex = trackModelIndex;
    playCurrentTrack();
}

void FlacLibraryController::next()
{
    if (m_playlist.isEmpty() || m_currentIndex < 0)
    {
        return;
    }

    if (m_currentIndex + 1 < m_playlist.size())
    {
        ++m_currentIndex;
        playCurrentTrack();
    }
    else
    {
        // Album finished -- stop playback (no looping, no continuation)
        qCInfo(mediaLibrary) << "FlacLibraryController: album finished, stopping";
        if (m_playbackController)
        {
            m_playbackController->stop();
        }
        m_playlist.clear();
        m_currentIndex = -1;
    }
}

void FlacLibraryController::previous()
{
    if (m_playlist.isEmpty() || m_currentIndex < 0)
    {
        return;
    }

    // If more than 3 seconds in, restart current track
    if (m_playbackState && m_playbackState->positionMs() > kPreviousRestartThresholdMs)
    {
        if (m_playbackController)
        {
            m_playbackController->seek(0);
        }
        return;
    }

    if (m_currentIndex > 0)
    {
        --m_currentIndex;
        playCurrentTrack();
    }
    else
    {
        // At first track, restart it
        if (m_playbackController)
        {
            m_playbackController->seek(0);
        }
    }
}

void FlacLibraryController::onTrackFinished()
{
    // Only handle if Library is the active source
    if (m_playbackState && m_playbackState->activeSource() == MediaSource::Library)
    {
        next();
    }
}

void FlacLibraryController::playCurrentTrack()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size())
    {
        return;
    }

    const auto& track = m_playlist[m_currentIndex];

    // Create FlacAudioStream
    auto stream = std::make_shared<FlacAudioStream>(track.filePath);

    // Update PlaybackState
    updatePlaybackState();

    // Play
    m_playbackController->play(stream);

    qCInfo(mediaLibrary) << "FlacLibraryController: playing" << track.title << "by" << track.artist;
}

void FlacLibraryController::updatePlaybackState()
{
    if (!m_playbackState || m_currentIndex < 0 || m_currentIndex >= m_playlist.size())
    {
        return;
    }

    const auto& track = m_playlist[m_currentIndex];

    m_playbackState->setActiveSource(MediaSource::Library);
    m_playbackState->setTitle(track.title);
    m_playbackState->setArtist(track.artist);
    m_playbackState->setAlbum(track.album);
    m_playbackState->setTrackNumber(m_currentIndex + 1);
    m_playbackState->setTrackCount(static_cast<int>(m_playlist.size()));
    m_playbackState->setDurationMs(static_cast<qint64>(track.durationSeconds) * 1000LL);

    // Set album art
    auto art = m_artProvider->getCachedArt(track.albumArtist, track.album);
    m_playbackState->setAlbumArtUrl(art.frontPath.isEmpty() ? QString() : "file://" + art.frontPath);
}

LibraryArtistModel* FlacLibraryController::artistModel() const
{
    return m_artistModel.get();
}

LibraryAlbumModel* FlacLibraryController::albumModel() const
{
    return m_albumModel.get();
}

LibraryTrackModel* FlacLibraryController::trackModel() const
{
    return m_trackModel.get();
}

bool FlacLibraryController::isScanning() const
{
    return m_scanner && m_scanner->isScanning();
}

LibraryDatabase* FlacLibraryController::database() const
{
    return m_database.get();
}
