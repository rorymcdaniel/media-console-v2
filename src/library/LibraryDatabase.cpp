#include "LibraryDatabase.h"

#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "utils/Logging.h"

LibraryDatabase::LibraryDatabase(QObject* parent)
    : QObject(parent)
    , m_connectionName("library_db_" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

LibraryDatabase::~LibraryDatabase()
{
    close();
}

bool LibraryDatabase::open(const QString& dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);

    if (!db.open())
    {
        qCWarning(mediaLibrary) << "LibraryDatabase::open: failed to open database:" << db.lastError().text();
        return false;
    }

    if (!createTables())
    {
        qCWarning(mediaLibrary) << "LibraryDatabase::open: failed to create tables";
        db.close();
        return false;
    }

    qCInfo(mediaLibrary) << "LibraryDatabase: opened" << dbPath;
    return true;
}

void LibraryDatabase::close()
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

bool LibraryDatabase::createTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    bool ok = query.exec("CREATE TABLE IF NOT EXISTS tracks ("
                         "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "  file_path TEXT UNIQUE NOT NULL,"
                         "  title TEXT,"
                         "  artist TEXT,"
                         "  album_artist TEXT,"
                         "  album TEXT,"
                         "  track_number INTEGER DEFAULT 0,"
                         "  disc_number INTEGER DEFAULT 1,"
                         "  year INTEGER DEFAULT 0,"
                         "  genre TEXT,"
                         "  duration_seconds INTEGER DEFAULT 0,"
                         "  sample_rate INTEGER DEFAULT 0,"
                         "  bit_depth INTEGER DEFAULT 0,"
                         "  mtime INTEGER NOT NULL,"
                         "  created_at TEXT DEFAULT (datetime('now'))"
                         ")");

    if (!ok)
    {
        qCWarning(mediaLibrary) << "LibraryDatabase: failed to create tracks table:" << query.lastError().text();
        return false;
    }

    ok = query.exec("CREATE INDEX IF NOT EXISTS idx_tracks_album_artist ON tracks(album_artist)");
    if (!ok)
    {
        qCWarning(mediaLibrary) << "LibraryDatabase: failed to create album_artist index:" << query.lastError().text();
        return false;
    }

    ok = query.exec("CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album_artist, album)");
    if (!ok)
    {
        qCWarning(mediaLibrary) << "LibraryDatabase: failed to create album index:" << query.lastError().text();
        return false;
    }

    ok = query.exec("CREATE INDEX IF NOT EXISTS idx_tracks_file_path ON tracks(file_path)");
    if (!ok)
    {
        qCWarning(mediaLibrary) << "LibraryDatabase: failed to create file_path index:" << query.lastError().text();
        return false;
    }

    return true;
}

bool LibraryDatabase::upsertTrack(const LibraryTrack& track)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("INSERT OR REPLACE INTO tracks "
                  "(file_path, title, artist, album_artist, album, track_number, disc_number, "
                  " year, genre, duration_seconds, sample_rate, bit_depth, mtime) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(track.filePath);
    query.addBindValue(track.title);
    query.addBindValue(track.artist);
    query.addBindValue(track.albumArtist);
    query.addBindValue(track.album);
    query.addBindValue(track.trackNumber);
    query.addBindValue(track.discNumber);
    query.addBindValue(track.year);
    query.addBindValue(track.genre);
    query.addBindValue(track.durationSeconds);
    query.addBindValue(track.sampleRate);
    query.addBindValue(track.bitDepth);
    query.addBindValue(track.mtime);

    if (!query.exec())
    {
        qCWarning(mediaLibrary) << "LibraryDatabase::upsertTrack: failed:" << query.lastError().text();
        return false;
    }

    return true;
}

bool LibraryDatabase::upsertTrackBatch(const QVector<LibraryTrack>& tracks)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    if (!db.transaction())
    {
        qCWarning(mediaLibrary) << "LibraryDatabase::upsertTrackBatch: failed to begin transaction";
        return false;
    }

    for (const auto& track : tracks)
    {
        if (!upsertTrack(track))
        {
            db.rollback();
            return false;
        }
    }

    return db.commit();
}

int LibraryDatabase::removeStaleEntries(const QSet<QString>& validPaths)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    // Get all file paths in database
    QSqlQuery query(db);
    query.exec("SELECT file_path FROM tracks");

    QVector<QString> stalePaths;
    while (query.next())
    {
        QString path = query.value(0).toString();
        if (!validPaths.contains(path))
        {
            stalePaths.append(path);
        }
    }

    if (stalePaths.isEmpty())
    {
        return 0;
    }

    if (!db.transaction())
    {
        return 0;
    }

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM tracks WHERE file_path = ?");

    for (const auto& path : stalePaths)
    {
        deleteQuery.addBindValue(path);
        if (!deleteQuery.exec())
        {
            qCWarning(mediaLibrary) << "LibraryDatabase::removeStaleEntries: delete failed:"
                                    << deleteQuery.lastError().text();
            db.rollback();
            return 0;
        }
    }

    db.commit();
    qCInfo(mediaLibrary) << "LibraryDatabase: removed" << stalePaths.size() << "stale entries";
    return stalePaths.size();
}

QVector<LibraryArtistEntry> LibraryDatabase::getArtists()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.exec("SELECT album_artist, COUNT(DISTINCT album) as album_count "
               "FROM tracks "
               "GROUP BY album_artist "
               "ORDER BY album_artist COLLATE NOCASE");

    QVector<LibraryArtistEntry> artists;
    while (query.next())
    {
        LibraryArtistEntry entry;
        entry.albumArtist = query.value(0).toString();
        entry.albumCount = query.value(1).toInt();
        artists.append(entry);
    }

    return artists;
}

QVector<LibraryAlbumEntry> LibraryDatabase::getAlbumsByArtist(const QString& albumArtist)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT DISTINCT album, MIN(year) as year, COUNT(*) as track_count "
                  "FROM tracks "
                  "WHERE album_artist = ? "
                  "GROUP BY album "
                  "ORDER BY year ASC, album COLLATE NOCASE");
    query.addBindValue(albumArtist);

    QVector<LibraryAlbumEntry> albums;
    if (query.exec())
    {
        while (query.next())
        {
            LibraryAlbumEntry entry;
            entry.album = query.value(0).toString();
            entry.albumArtist = albumArtist;
            entry.year = query.value(1).toInt();
            entry.trackCount = query.value(2).toInt();
            albums.append(entry);
        }
    }

    return albums;
}

QVector<LibraryTrack> LibraryDatabase::getTracksByAlbum(const QString& albumArtist, const QString& album)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT id, file_path, title, artist, album_artist, album, "
                  "       track_number, disc_number, year, genre, "
                  "       duration_seconds, sample_rate, bit_depth, mtime "
                  "FROM tracks "
                  "WHERE album_artist = ? AND album = ? "
                  "ORDER BY disc_number ASC, track_number ASC");
    query.addBindValue(albumArtist);
    query.addBindValue(album);

    QVector<LibraryTrack> tracks;
    if (query.exec())
    {
        while (query.next())
        {
            LibraryTrack track;
            track.id = query.value(0).toInt();
            track.filePath = query.value(1).toString();
            track.title = query.value(2).toString();
            track.artist = query.value(3).toString();
            track.albumArtist = query.value(4).toString();
            track.album = query.value(5).toString();
            track.trackNumber = query.value(6).toInt();
            track.discNumber = query.value(7).toInt();
            track.year = query.value(8).toInt();
            track.genre = query.value(9).toString();
            track.durationSeconds = query.value(10).toInt();
            track.sampleRate = query.value(11).toInt();
            track.bitDepth = query.value(12).toInt();
            track.mtime = query.value(13).toLongLong();
            tracks.append(track);
        }
    }

    return tracks;
}

qint64 LibraryDatabase::getTrackMtime(const QString& filePath)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT mtime FROM tracks WHERE file_path = ?");
    query.addBindValue(filePath);

    if (query.exec() && query.next())
    {
        return query.value(0).toLongLong();
    }

    return -1;
}

QMap<QString, qint64> LibraryDatabase::getAllMtimes()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    QMap<QString, qint64> mtimes;
    if (query.exec("SELECT file_path, mtime FROM tracks"))
    {
        while (query.next())
        {
            mtimes.insert(query.value(0).toString(), query.value(1).toLongLong());
        }
    }

    return mtimes;
}

int LibraryDatabase::trackCount() const
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (query.exec("SELECT COUNT(*) FROM tracks") && query.next())
    {
        return query.value(0).toInt();
    }

    return 0;
}
