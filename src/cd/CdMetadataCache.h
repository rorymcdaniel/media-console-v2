#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include <optional>

struct CdTrackInfo
{
    int trackNumber = 0;
    QString title;
    QString artist; // per-track artist, may differ from album artist
    int durationSeconds = 0;
};

struct CdMetadata
{
    QString discId;
    QString artist;
    QString album;
    QString genre;
    int year = 0;
    QString source; // "musicbrainz", "gnudb", "discogs", "fallback"
    QString musicbrainzReleaseId; // for CoverArtArchive lookup
    QVector<CdTrackInfo> tracks;
};

struct CdAlbumArtInfo
{
    QString frontPath;
    QString backPath;
    QString source; // "coverartarchive", "discogs"
};

/// SQLite-backed metadata cache for CD disc lookups. Provides instant metadata
/// retrieval for previously-seen discs without network requests.
class CdMetadataCache : public QObject
{
    Q_OBJECT
public:
    explicit CdMetadataCache(QObject* parent = nullptr);
    ~CdMetadataCache() override;

    bool open(const QString& dbPath);
    void close();

    std::optional<CdMetadata> lookup(const QString& discId);
    bool store(const CdMetadata& metadata);

    std::optional<CdAlbumArtInfo> getAlbumArt(const QString& discId);
    bool storeAlbumArt(const QString& discId, const QString& frontPath, const QString& backPath, const QString& source);

private:
    bool createTables();
    QString m_connectionName;
};
