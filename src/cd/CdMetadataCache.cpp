#include "CdMetadataCache.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "utils/Logging.h"

CdMetadataCache::CdMetadataCache(QObject* parent)
    : QObject(parent)
    , m_connectionName("cd_metadata_cache_" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

CdMetadataCache::~CdMetadataCache()
{
    close();
}

bool CdMetadataCache::open(const QString& dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);

    if (!db.open())
    {
        qCWarning(mediaCd) << "CdMetadataCache::open: failed to open database:" << db.lastError().text();
        return false;
    }

    if (!createTables())
    {
        qCWarning(mediaCd) << "CdMetadataCache::open: failed to create tables";
        db.close();
        return false;
    }

    qCInfo(mediaCd) << "CdMetadataCache: opened" << dbPath;
    return true;
}

void CdMetadataCache::close()
{
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen())
        {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool CdMetadataCache::createTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    bool ok = query.exec("CREATE TABLE IF NOT EXISTS disc_metadata ("
                         "  disc_id TEXT PRIMARY KEY,"
                         "  artist TEXT,"
                         "  album TEXT,"
                         "  genre TEXT,"
                         "  year INTEGER,"
                         "  source TEXT,"
                         "  musicbrainz_release_id TEXT,"
                         "  created_at TEXT DEFAULT (datetime('now'))"
                         ")");

    if (!ok)
    {
        qCWarning(mediaCd) << "CdMetadataCache: failed to create disc_metadata:" << query.lastError().text();
        return false;
    }

    ok = query.exec("CREATE TABLE IF NOT EXISTS track_metadata ("
                    "  disc_id TEXT,"
                    "  track_number INTEGER,"
                    "  title TEXT,"
                    "  artist TEXT,"
                    "  duration_seconds INTEGER,"
                    "  PRIMARY KEY (disc_id, track_number),"
                    "  FOREIGN KEY (disc_id) REFERENCES disc_metadata(disc_id)"
                    ")");

    if (!ok)
    {
        qCWarning(mediaCd) << "CdMetadataCache: failed to create track_metadata:" << query.lastError().text();
        return false;
    }

    ok = query.exec("CREATE TABLE IF NOT EXISTS album_art ("
                    "  disc_id TEXT PRIMARY KEY,"
                    "  front_path TEXT,"
                    "  back_path TEXT,"
                    "  source TEXT,"
                    "  created_at TEXT DEFAULT (datetime('now')),"
                    "  FOREIGN KEY (disc_id) REFERENCES disc_metadata(disc_id)"
                    ")");

    if (!ok)
    {
        qCWarning(mediaCd) << "CdMetadataCache: failed to create album_art:" << query.lastError().text();
        return false;
    }

    return true;
}

std::optional<CdMetadata> CdMetadataCache::lookup(const QString& discId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT artist, album, genre, year, source, musicbrainz_release_id "
                  "FROM disc_metadata WHERE disc_id = ?");
    query.addBindValue(discId);

    if (!query.exec() || !query.next())
    {
        return std::nullopt;
    }

    CdMetadata metadata;
    metadata.discId = discId;
    metadata.artist = query.value(0).toString();
    metadata.album = query.value(1).toString();
    metadata.genre = query.value(2).toString();
    metadata.year = query.value(3).toInt();
    metadata.source = query.value(4).toString();
    metadata.musicbrainzReleaseId = query.value(5).toString();

    // Load tracks
    QSqlQuery trackQuery(db);
    trackQuery.prepare("SELECT track_number, title, artist, duration_seconds "
                       "FROM track_metadata WHERE disc_id = ? ORDER BY track_number");
    trackQuery.addBindValue(discId);

    if (trackQuery.exec())
    {
        while (trackQuery.next())
        {
            CdTrackInfo track;
            track.trackNumber = trackQuery.value(0).toInt();
            track.title = trackQuery.value(1).toString();
            track.artist = trackQuery.value(2).toString();
            track.durationSeconds = trackQuery.value(3).toInt();
            metadata.tracks.append(track);
        }
    }

    return metadata;
}

bool CdMetadataCache::store(const CdMetadata& metadata)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    if (!db.transaction())
    {
        qCWarning(mediaCd) << "CdMetadataCache::store: failed to begin transaction";
        return false;
    }

    QSqlQuery query(db);

    // Upsert disc metadata
    query.prepare("INSERT OR REPLACE INTO disc_metadata "
                  "(disc_id, artist, album, genre, year, source, musicbrainz_release_id) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(metadata.discId);
    query.addBindValue(metadata.artist);
    query.addBindValue(metadata.album);
    query.addBindValue(metadata.genre);
    query.addBindValue(metadata.year);
    query.addBindValue(metadata.source);
    query.addBindValue(metadata.musicbrainzReleaseId);

    if (!query.exec())
    {
        qCWarning(mediaCd) << "CdMetadataCache::store: disc insert failed:" << query.lastError().text();
        db.rollback();
        return false;
    }

    // Delete existing tracks for this disc
    query.prepare("DELETE FROM track_metadata WHERE disc_id = ?");
    query.addBindValue(metadata.discId);
    query.exec();

    // Insert tracks
    query.prepare("INSERT INTO track_metadata (disc_id, track_number, title, artist, duration_seconds) "
                  "VALUES (?, ?, ?, ?, ?)");

    for (const auto& track : metadata.tracks)
    {
        query.addBindValue(metadata.discId);
        query.addBindValue(track.trackNumber);
        query.addBindValue(track.title);
        query.addBindValue(track.artist);
        query.addBindValue(track.durationSeconds);

        if (!query.exec())
        {
            qCWarning(mediaCd) << "CdMetadataCache::store: track insert failed:" << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    return db.commit();
}

std::optional<CdAlbumArtInfo> CdMetadataCache::getAlbumArt(const QString& discId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT front_path, back_path, source FROM album_art WHERE disc_id = ?");
    query.addBindValue(discId);

    if (!query.exec() || !query.next())
    {
        return std::nullopt;
    }

    CdAlbumArtInfo info;
    info.frontPath = query.value(0).toString();
    info.backPath = query.value(1).toString();
    info.source = query.value(2).toString();
    return info;
}

bool CdMetadataCache::storeAlbumArt(const QString& discId, const QString& frontPath, const QString& backPath,
                                    const QString& source)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("INSERT OR REPLACE INTO album_art (disc_id, front_path, back_path, source) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(discId);
    query.addBindValue(frontPath);
    query.addBindValue(backPath);
    query.addBindValue(source);

    if (!query.exec())
    {
        qCWarning(mediaCd) << "CdMetadataCache::storeAlbumArt: failed:" << query.lastError().text();
        return false;
    }

    return true;
}
