#pragma once

#include <QObject>
#include <QVector>

#include <memory>

#include "library/LibraryDatabase.h"

struct LibraryConfig;
class LibraryScanner;
class LibraryAlbumArtProvider;
class LibraryArtistModel;
class LibraryAlbumModel;
class LibraryTrackModel;
class LocalPlaybackController;
class PlaybackState;

/// Lifecycle orchestrator for the FLAC library subsystem.
/// Coordinates scanning, database, art extraction, browse models, and playlist playback.
/// Follows CdController pattern: owns sub-components, wires signals, manages lifecycle.
class FlacLibraryController : public QObject
{
    Q_OBJECT
public:
    FlacLibraryController(LocalPlaybackController* playbackController, PlaybackState* playbackState,
                          const LibraryConfig& config, QObject* parent = nullptr);
    ~FlacLibraryController() override;

    void start();
    void stop();

    // User-initiated playback (FLAC-08)
    void playTrack(int trackModelIndex);
    void next();
    void previous();

    // Accessors for models (QML binding in Phase 10)
    LibraryArtistModel* artistModel() const;
    LibraryAlbumModel* albumModel() const;
    LibraryTrackModel* trackModel() const;

    bool isScanning() const;

    // Accessor for database (used by AppBuilder for wiring)
    LibraryDatabase* database() const;

signals:
    void scanProgress(int processed, int total);
    void scanComplete(int totalProcessed, int totalSkipped);
    void libraryReady();

private slots:
    void onBatchReady(const QVector<LibraryTrack>& tracks);
    void onScanComplete(int totalProcessed, int totalSkipped);
    void onTrackFinished();

private:
    void startScan();
    void playCurrentTrack();
    void updatePlaybackState();

    LocalPlaybackController* m_playbackController; // Non-owning
    PlaybackState* m_playbackState; // Non-owning
    QString m_rootPath;

    std::unique_ptr<LibraryDatabase> m_database;
    std::unique_ptr<LibraryScanner> m_scanner;
    std::unique_ptr<LibraryAlbumArtProvider> m_artProvider;
    std::unique_ptr<LibraryArtistModel> m_artistModel;
    std::unique_ptr<LibraryAlbumModel> m_albumModel;
    std::unique_ptr<LibraryTrackModel> m_trackModel;

    // Playlist state
    QVector<LibraryTrack> m_playlist;
    int m_currentIndex = -1;

    static constexpr int kPreviousRestartThresholdMs = 3000;
};
