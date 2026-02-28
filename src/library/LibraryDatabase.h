#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

struct LibraryTrack
{
    int id = 0;
    QString filePath;
    QString title;
    QString artist;
    QString albumArtist;
    QString album;
    int trackNumber = 0;
    int discNumber = 1;
    int year = 0;
    QString genre;
    int durationSeconds = 0;
    int sampleRate = 0;
    int bitDepth = 0;
    qint64 mtime = 0;
};

struct LibraryArtistEntry
{
    QString albumArtist;
    int albumCount = 0;
};

struct LibraryAlbumEntry
{
    QString album;
    QString albumArtist;
    int year = 0;
    int trackCount = 0;
};

/// SQLite-backed library database for FLAC file metadata.
/// Follows CdMetadataCache pattern: unique connection name, prepared statements.
class LibraryDatabase : public QObject
{
    Q_OBJECT
public:
    explicit LibraryDatabase(QObject* parent = nullptr);
    ~LibraryDatabase() override;

    bool open(const QString& dbPath);
    void close();

    // Write operations (main thread only per Qt SQL threading rules)
    bool upsertTrack(const LibraryTrack& track);
    bool upsertTrackBatch(const QVector<LibraryTrack>& tracks);
    int removeStaleEntries(const QSet<QString>& validPaths);

    // Read operations for browse models
    QVector<LibraryArtistEntry> getArtists();
    QVector<LibraryAlbumEntry> getAlbumsByArtist(const QString& albumArtist);
    QVector<LibraryTrack> getTracksByAlbum(const QString& albumArtist, const QString& album);

    // Incremental scan support
    qint64 getTrackMtime(const QString& filePath);
    QMap<QString, qint64> getAllMtimes();
    int trackCount() const;

private:
    bool createTables();
    QString m_connectionName;
};
