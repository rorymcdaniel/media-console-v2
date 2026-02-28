#include "LibraryAlbumArtProvider.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>

#include "utils/Logging.h"

const QStringList LibraryAlbumArtProvider::kFrontFallbacks
    = { "cover.jpg",  "Cover.jpg",  "cover.png", "Cover.png", "folder.jpg", "Folder.jpg",
        "folder.png", "Folder.png", "front.jpg", "Front.jpg", "front.png",  "Front.png" };

const QStringList LibraryAlbumArtProvider::kBackFallbacks
    = { "back.jpg",       "Back.jpg",       "back.png",       "Back.png",
        "back-cover.jpg", "Back-cover.jpg", "back-cover.png", "Back-cover.png" };

LibraryAlbumArtProvider::LibraryAlbumArtProvider(const QString& cacheDir, QObject* parent)
    : QObject(parent)
    , m_cacheDir(cacheDir)
{
    QDir().mkpath(m_cacheDir);
}

LibraryAlbumArt LibraryAlbumArtProvider::extractArt(const QString& flacPath, const QString& albumArtist,
                                                    const QString& album)
{
    QString hash = computeHash(albumArtist, album);

    // Skip re-extraction if cached
    if (hasCachedArt(albumArtist, album))
    {
        return getCachedArt(albumArtist, album);
    }

    LibraryAlbumArt art;

    // Try embedded FLAC picture blocks first
    art.frontPath = extractFromFlac(flacPath, hash, "front");
    art.backPath = extractFromFlac(flacPath, hash, "back");

    // Fall back to directory files
    QString albumDir = QFileInfo(flacPath).absolutePath();

    if (art.frontPath.isEmpty())
    {
        art.frontPath = findFileArt(albumDir, hash, "front");
    }

    if (art.backPath.isEmpty())
    {
        art.backPath = findFileArt(albumDir, hash, "back");
    }

    return art;
}

bool LibraryAlbumArtProvider::hasCachedArt(const QString& albumArtist, const QString& album) const
{
    QString hash = computeHash(albumArtist, album);
    return !findCachedFile(hash, "front").isEmpty();
}

LibraryAlbumArt LibraryAlbumArtProvider::getCachedArt(const QString& albumArtist, const QString& album) const
{
    QString hash = computeHash(albumArtist, album);
    LibraryAlbumArt art;
    art.frontPath = findCachedFile(hash, "front");
    art.backPath = findCachedFile(hash, "back");
    return art;
}

QString LibraryAlbumArtProvider::computeHash(const QString& albumArtist, const QString& album) const
{
    QByteArray data = (albumArtist + album).toUtf8();
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
}

QString LibraryAlbumArtProvider::extractFromFlac(const QString& flacPath, const QString& hash, const QString& type)
{
    TagLib::FLAC::File flacFile(flacPath.toUtf8().constData());
    if (!flacFile.isValid())
    {
        return {};
    }

    auto targetType = (type == "front") ? TagLib::FLAC::Picture::FrontCover : TagLib::FLAC::Picture::BackCover;

    for (const auto* pic : flacFile.pictureList())
    {
        if (pic->type() == targetType)
        {
            QByteArray artData(pic->data().data(), static_cast<int>(pic->data().size()));
            QString path = saveToDisk(hash, type, artData);
            if (!path.isEmpty())
            {
                qCDebug(mediaLibrary) << "LibraryAlbumArtProvider: extracted" << type << "from FLAC:" << flacPath;
                return path;
            }
        }
    }

    return {};
}

QString LibraryAlbumArtProvider::findFileArt(const QString& albumDir, const QString& hash, const QString& type)
{
    const QStringList& fallbacks = (type == "front") ? kFrontFallbacks : kBackFallbacks;

    for (const QString& filename : fallbacks)
    {
        QString candidate = albumDir + "/" + filename;
        if (QFile::exists(candidate))
        {
            QFile file(candidate);
            if (file.open(QIODevice::ReadOnly))
            {
                QByteArray data = file.readAll();
                file.close();
                QString path = saveToDisk(hash, type, data);
                if (!path.isEmpty())
                {
                    qCDebug(mediaLibrary) << "LibraryAlbumArtProvider: found" << type << "fallback:" << candidate;
                    return path;
                }
            }
        }
    }

    return {};
}

QString LibraryAlbumArtProvider::saveToDisk(const QString& hash, const QString& type, const QByteArray& data)
{
    if (data.isEmpty())
    {
        return {};
    }

    QString ext = detectExtension(data);
    QString filename = QString("%1/%2_%3.%4").arg(m_cacheDir, hash, type, ext);

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        qCWarning(mediaLibrary) << "LibraryAlbumArtProvider: failed to write" << filename;
        return {};
    }

    file.write(data);
    file.close();

    return filename;
}

QString LibraryAlbumArtProvider::detectExtension(const QByteArray& data) const
{
    if (data.startsWith("\x89PNG"))
    {
        return "png";
    }
    return "jpg";
}

QString LibraryAlbumArtProvider::findCachedFile(const QString& hash, const QString& type) const
{
    // Check for both jpg and png variants
    for (const QString& ext : { "jpg", "png" })
    {
        QString path = QString("%1/%2_%3.%4").arg(m_cacheDir, hash, type, ext);
        if (QFile::exists(path))
        {
            return path;
        }
    }
    return {};
}
