#include "CdController.h"

#include <QNetworkAccessManager>
#include <QStandardPaths>

#include "app/AppConfig.h"
#include "audio/LocalPlaybackController.h"
#include "cd/CdAlbumArtProvider.h"
#include "cd/CdAudioStream.h"
#include "cd/CdMetadataProvider.h"
#include "platform/ICdDrive.h"
#include "state/MediaSource.h"
#include "state/PlaybackState.h"
#include "utils/Logging.h"

CdController::CdController(ICdDrive* drive, LocalPlaybackController* playbackController, PlaybackState* playbackState,
                           const CdConfig& config, QObject* parent)
    : QObject(parent)
    , m_drive(drive)
    , m_playbackController(playbackController)
    , m_playbackState(playbackState)
    , m_devicePath(config.devicePath)
    , m_pollIntervalMs(config.pollIntervalMs)
    , m_audioOnly(config.audioOnly)
    , m_idleTimeoutSeconds(config.idleTimeoutSeconds)
{
    // Create network manager for metadata/art lookups
    m_nam = std::make_unique<QNetworkAccessManager>(this);

    // Create metadata cache
    m_cache = std::make_unique<CdMetadataCache>(this);
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_cache->open(cacheDir + "/cd_metadata.db");

    // Create metadata provider
    m_metadataProvider = std::make_unique<CdMetadataProvider>(m_nam.get(), this);

    // Create album art provider
    m_albumArtProvider = std::make_unique<CdAlbumArtProvider>(m_nam.get(), cacheDir + "/albumart", this);

    // Wire metadata signals
    connect(m_metadataProvider.get(), &CdMetadataProvider::metadataReady, this, &CdController::onMetadataReady);
    connect(m_metadataProvider.get(), &CdMetadataProvider::lookupFailed, this, &CdController::onMetadataFailed);

    // Wire album art signals
    connect(m_albumArtProvider.get(), &CdAlbumArtProvider::albumArtReady, this, &CdController::onAlbumArtReady);
    connect(m_albumArtProvider.get(), &CdAlbumArtProvider::albumArtFailed, this, &CdController::onAlbumArtFailed);

    // Wire playback finished
    if (m_playbackController)
    {
        connect(m_playbackController, &LocalPlaybackController::trackFinished, this, &CdController::onTrackFinished);
    }

    // Wire timers
    connect(&m_pollTimer, &QTimer::timeout, this, &CdController::onPollTimer);
    connect(&m_idleTimer, &QTimer::timeout, this, &CdController::onIdleTimeout);

    // Configure idle timer as single-shot
    m_idleTimer.setSingleShot(true);
    m_idleTimer.setInterval(m_idleTimeoutSeconds * 1000);
}

CdController::~CdController()
{
    stop();
}

void CdController::start()
{
    m_drive->openDevice(m_devicePath);
    m_pollTimer.start(m_pollIntervalMs);
    qCInfo(mediaCd) << "CdController: started polling at" << m_pollIntervalMs << "ms";
}

void CdController::stop()
{
    m_pollTimer.stop();
    m_idleTimer.stop();
    m_metadataProvider->cancel();
    m_albumArtProvider->cancel();
    qCInfo(mediaCd) << "CdController: stopped";
}

void CdController::onPollTimer()
{
    bool discNow = m_drive->isDiscPresent();

    if (discNow && !m_discPresent)
    {
        handleDiscInserted();
    }
    else if (!discNow && m_discPresent)
    {
        handleDiscRemoved();
    }
}

void CdController::handleDiscInserted()
{
    m_discPresent = true;
    emit discDetected();
    qCInfo(mediaCd) << "CdController: disc detected";

    // Check if audio disc
    if (m_audioOnly && !m_drive->isAudioDisc())
    {
        qCInfo(mediaCd) << "CdController: non-audio disc detected";
        emit nonAudioDiscDetected();
        return;
    }

    // Read TOC
    m_currentToc = m_drive->readToc();
    emit tocReady(m_currentToc);
    qCInfo(mediaCd) << "CdController: TOC ready," << m_currentToc.size() << "tracks";

    // Get disc ID
    m_currentDiscId = m_drive->getDiscId();

    // Check cache for known disc
    auto cached = m_cache->lookup(m_currentDiscId);
    if (cached.has_value())
    {
        m_currentMetadata = cached.value();
        emit metadataReady(m_currentMetadata);
        qCInfo(mediaCd) << "CdController: cache hit for" << m_currentDiscId;

        // Check art cache
        auto art = m_cache->getAlbumArt(m_currentDiscId);
        if (art.has_value())
        {
            emit albumArtReady(art->frontPath, art->backPath);
        }
        else if (!m_currentMetadata.musicbrainzReleaseId.isEmpty())
        {
            m_albumArtProvider->download(m_currentMetadata.musicbrainzReleaseId, m_currentDiscId);
        }
    }
    else
    {
        // Cache miss -- start async metadata lookup
        qCInfo(mediaCd) << "CdController: cache miss, starting lookup for" << m_currentDiscId;
        m_metadataProvider->lookup(m_currentDiscId, m_currentToc);
    }

    startIdleTimer();
}

void CdController::handleDiscRemoved()
{
    m_discPresent = false;

    // Cancel pending lookups
    m_metadataProvider->cancel();
    m_albumArtProvider->cancel();

    // Stop active playback
    if (m_playbackController && m_playbackController->isActive())
    {
        m_playbackController->stop();
    }

    // Clear state
    m_currentToc.clear();
    m_currentMetadata = CdMetadata();
    m_currentDiscId.clear();

    emit discRemoved();
    stopIdleTimer();

    qCInfo(mediaCd) << "CdController: disc removed";
}

void CdController::onMetadataReady(const CdMetadata& metadata)
{
    m_currentMetadata = metadata;
    m_cache->store(metadata);
    emit metadataReady(metadata);
    qCInfo(mediaCd) << "CdController: metadata ready -" << metadata.artist << "/" << metadata.album;

    // Start album art download if MusicBrainz release ID available
    if (!metadata.musicbrainzReleaseId.isEmpty())
    {
        m_albumArtProvider->download(metadata.musicbrainzReleaseId, metadata.discId);
    }
}

void CdController::onMetadataFailed(const QString& discId)
{
    // Silent fallback to "Audio CD" per user decision
    m_currentMetadata.discId = discId;
    m_currentMetadata.artist = "Audio CD";
    m_currentMetadata.album = "Audio CD";
    m_currentMetadata.source = "fallback";
    emit metadataReady(m_currentMetadata);
    qCInfo(mediaCd) << "CdController: metadata lookup failed, using fallback for" << discId;
}

void CdController::onAlbumArtReady(const QString& discId, const QString& frontPath, const QString& backPath)
{
    m_cache->storeAlbumArt(discId, frontPath, backPath, "coverartarchive");
    emit albumArtReady(frontPath, backPath);
    qCInfo(mediaCd) << "CdController: album art ready for" << discId;
}

void CdController::onAlbumArtFailed(const QString& discId)
{
    // Silent failure -- art area stays empty
    qCInfo(mediaCd) << "CdController: album art failed for" << discId << "(silent)";
}

void CdController::playTrack(int trackNumber)
{
    if (trackNumber < 1 || trackNumber > m_currentToc.size())
    {
        qCWarning(mediaCd) << "CdController: invalid track number" << trackNumber;
        return;
    }

    resetIdleTimer();

    const auto& track = m_currentToc[trackNumber - 1];
    auto stream = std::make_shared<CdAudioStream>(m_devicePath, track.startSector, track.endSector);

    // Update PlaybackState
    if (m_playbackState)
    {
        m_playbackState->setActiveSource(MediaSource::CD);
        m_playbackState->setTrackNumber(trackNumber);
        m_playbackState->setTrackCount(m_currentToc.size());

        if (trackNumber <= m_currentMetadata.tracks.size())
        {
            const auto& trackInfo = m_currentMetadata.tracks[trackNumber - 1];
            m_playbackState->setTitle(trackInfo.title);
            m_playbackState->setArtist(trackInfo.artist);
        }
        m_playbackState->setAlbum(m_currentMetadata.album);
        m_playbackState->setDurationMs(track.durationSeconds * 1000LL);
    }

    // Start playback (spin-up handled on background thread)
    emit spinUpStarted();
    if (m_playbackController)
    {
        m_playbackController->play(stream);
    }
    emit spinUpComplete();

    qCInfo(mediaCd) << "CdController: playing track" << trackNumber;
}

void CdController::eject()
{
    if (m_playbackController && m_playbackController->isActive())
    {
        m_playbackController->stop();
    }
    m_drive->eject();
    handleDiscRemoved();
}

QVector<TocEntry> CdController::currentToc() const
{
    return m_currentToc;
}

CdMetadata CdController::currentMetadata() const
{
    return m_currentMetadata;
}

bool CdController::isDiscPresent() const
{
    return m_discPresent;
}

void CdController::onTrackFinished()
{
    startIdleTimer();
}

void CdController::onIdleTimeout()
{
    qCInfo(mediaCd) << "CdController: idle timeout, stopping spindle";
    m_drive->stopSpindle();
}

void CdController::startIdleTimer()
{
    m_idleTimer.start(m_idleTimeoutSeconds * 1000);
}

void CdController::resetIdleTimer()
{
    if (m_idleTimer.isActive())
    {
        m_idleTimer.start(m_idleTimeoutSeconds * 1000);
    }
}

void CdController::stopIdleTimer()
{
    m_idleTimer.stop();
}
