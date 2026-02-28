#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;

/// Downloads album art from CoverArtArchive (primary) with Discogs fallback.
/// Caches downloaded art to disk. Emits albumArtReady with file paths.
class CdAlbumArtProvider : public QObject
{
    Q_OBJECT
public:
    explicit CdAlbumArtProvider(QNetworkAccessManager* nam, const QString& cacheDir, QObject* parent = nullptr);

    void download(const QString& musicbrainzReleaseId, const QString& discId);
    void cancel();

signals:
    void albumArtReady(const QString& discId, const QString& frontPath, const QString& backPath);
    void albumArtFailed(const QString& discId);

private:
    void downloadFromCoverArtArchive(const QString& releaseId, const QString& discId);
    void downloadFromDiscogs(const QString& discId);
    QString saveToDisk(const QString& discId, const QString& type, const QByteArray& data);

    QNetworkAccessManager* m_nam; // Non-owning
    QString m_cacheDir;
};
