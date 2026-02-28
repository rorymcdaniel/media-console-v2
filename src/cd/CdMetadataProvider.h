#pragma once

#include <QObject>
#include <QVector>

#include "cd/CdMetadataCache.h"
#include "platform/ICdDrive.h"

class QNetworkAccessManager;
class QNetworkReply;

/// Async three-tier metadata provider: MusicBrainz -> GnuDB -> Discogs.
/// Stops at first successful source. Emits metadataReady or lookupFailed.
/// All network I/O via QNetworkAccessManager (non-blocking, no threads).
class CdMetadataProvider : public QObject
{
    Q_OBJECT
public:
    explicit CdMetadataProvider(QNetworkAccessManager* nam, QObject* parent = nullptr);

    void lookup(const QString& discId, const QVector<TocEntry>& toc);
    void cancel();

    /// Compute freedb disc ID from TOC (needed for GnuDB queries)
    static quint32 computeFreedbDiscId(const QVector<TocEntry>& toc);

    /// Parse GnuDB DTITLE line: "Artist / Album"
    static QPair<QString, QString> parseDTitle(const QString& dtitle);

signals:
    void metadataReady(const CdMetadata& metadata);
    void lookupFailed(const QString& discId);

private:
    void lookupMusicBrainz();
    void lookupGnuDb();
    void lookupDiscogs();

    bool parseMusicBrainzResponse(const QByteArray& data);
    bool parseGnuDbResponse(const QByteArray& data);
    bool parseDiscogsResponse(const QByteArray& data);

    bool validateGnuDbTrackCount(int responseTrackCount);

    QString buildGnuDbQueryUrl();
    QString buildGnuDbReadUrl(const QString& category, const QString& discId);

    QNetworkAccessManager* m_nam; // Non-owning
    QString m_discId;
    QVector<TocEntry> m_toc;
    CdMetadata m_metadata;
    QNetworkReply* m_activeReply = nullptr;

    static constexpr int kRequestTimeoutMs = 10000;
};
