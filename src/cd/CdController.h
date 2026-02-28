#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>

#include <memory>

#include "cd/CdMetadataCache.h" // for CdMetadata, CdTrackInfo, CdAlbumArtInfo

struct TocEntry;
struct CdConfig;
class ICdDrive;
class CdMetadataProvider;
class CdAlbumArtProvider;
class LocalPlaybackController;
class PlaybackState;
class QNetworkAccessManager;

/// Lifecycle orchestrator for the CD subsystem.
/// Coordinates disc detection, progressive metadata loading (TOC -> metadata -> art),
/// idle spindle management, and user-initiated playback.
/// Never auto-plays on disc insertion (CD-04).
class CdController : public QObject
{
    Q_OBJECT

public:
    CdController(ICdDrive* drive, LocalPlaybackController* playbackController, PlaybackState* playbackState,
                 const CdConfig& config, QObject* parent = nullptr);
    ~CdController() override;

    void start();
    void stop();

    // User-initiated playback (CD-04: ALWAYS user-initiated, never auto-play)
    void playTrack(int trackNumber);
    void eject();

    // Accessors for UI binding
    QVector<TocEntry> currentToc() const;
    CdMetadata currentMetadata() const;
    bool isDiscPresent() const;

signals:
    // Progressive display signals (CD-06)
    void discDetected();
    void tocReady(const QVector<TocEntry>& toc);
    void metadataReady(const CdMetadata& metadata);
    void albumArtReady(const QString& frontPath, const QString& backPath);

    void discRemoved();
    void nonAudioDiscDetected();

    void spinUpStarted();
    void spinUpComplete();

private slots:
    void onPollTimer();
    void onMetadataReady(const CdMetadata& metadata);
    void onMetadataFailed(const QString& discId);
    void onAlbumArtReady(const QString& discId, const QString& frontPath, const QString& backPath);
    void onAlbumArtFailed(const QString& discId);
    void onTrackFinished();
    void onIdleTimeout();

private:
    void handleDiscInserted();
    void handleDiscRemoved();
    void startIdleTimer();
    void resetIdleTimer();
    void stopIdleTimer();

    ICdDrive* m_drive; // Non-owning
    LocalPlaybackController* m_playbackController; // Non-owning
    PlaybackState* m_playbackState; // Non-owning

    QString m_devicePath;
    int m_pollIntervalMs;
    bool m_audioOnly;
    int m_idleTimeoutSeconds;

    std::unique_ptr<QNetworkAccessManager> m_nam;
    std::unique_ptr<CdMetadataCache> m_cache;
    std::unique_ptr<CdMetadataProvider> m_metadataProvider;
    std::unique_ptr<CdAlbumArtProvider> m_albumArtProvider;

    QTimer m_pollTimer;
    QTimer m_idleTimer;

    bool m_discPresent = false;

    QVector<TocEntry> m_currentToc;
    CdMetadata m_currentMetadata;
    QString m_currentDiscId;
};
