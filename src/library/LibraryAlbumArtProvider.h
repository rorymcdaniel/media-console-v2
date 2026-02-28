#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

struct LibraryAlbumArt
{
    QString frontPath;
    QString backPath;
};

/// Extracts album art from FLAC picture blocks and caches to disk.
/// Falls back to cover.jpg/folder.jpg/front.jpg in album directory.
/// Cache filenames use SHA-1(albumArtist + album) per FLAC-05.
class LibraryAlbumArtProvider : public QObject
{
    Q_OBJECT
public:
    explicit LibraryAlbumArtProvider(const QString& cacheDir, QObject* parent = nullptr);

    /// Extract art for an album. Can be called from background thread.
    /// Returns paths to cached front/back art (empty if not found).
    LibraryAlbumArt extractArt(const QString& flacPath, const QString& albumArtist, const QString& album);

    /// Check if art is already cached for this album.
    bool hasCachedArt(const QString& albumArtist, const QString& album) const;

    /// Get cached art paths (empty if not cached).
    LibraryAlbumArt getCachedArt(const QString& albumArtist, const QString& album) const;

    /// Compute SHA-1 hash for album art filename.
    QString computeHash(const QString& albumArtist, const QString& album) const;

private:
    QString extractFromFlac(const QString& flacPath, const QString& hash, const QString& type);
    QString findFileArt(const QString& albumDir, const QString& hash, const QString& type);
    QString saveToDisk(const QString& hash, const QString& type, const QByteArray& data);
    QString detectExtension(const QByteArray& data) const;
    QString findCachedFile(const QString& hash, const QString& type) const;

    QString m_cacheDir;

    // Fallback filenames to scan in album directory
    static const QStringList kFrontFallbacks;
    static const QStringList kBackFallbacks;
};
