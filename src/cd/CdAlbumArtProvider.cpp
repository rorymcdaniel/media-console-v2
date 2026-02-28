#include "CdAlbumArtProvider.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include "utils/Logging.h"

static constexpr int kRequestTimeoutMs = 15000;

CdAlbumArtProvider::CdAlbumArtProvider(QNetworkAccessManager* nam, const QString& cacheDir, QObject* parent)
    : QObject(parent)
    , m_nam(nam)
    , m_cacheDir(cacheDir)
{
    QDir().mkpath(m_cacheDir);
}

void CdAlbumArtProvider::download(const QString& musicbrainzReleaseId, const QString& discId)
{
    if (!musicbrainzReleaseId.isEmpty())
    {
        downloadFromCoverArtArchive(musicbrainzReleaseId, discId);
    }
    else
    {
        // No MusicBrainz release ID -- try Discogs directly
        downloadFromDiscogs(discId);
    }
}

void CdAlbumArtProvider::cancel()
{
    // Active replies are managed by connection lambdas; cancellation happens
    // via QNetworkReply::abort if needed. For simplicity, we don't track
    // multiple concurrent downloads here.
}

void CdAlbumArtProvider::downloadFromCoverArtArchive(const QString& releaseId, const QString& discId)
{
    // CoverArtArchive front cover
    QString frontUrl = QString("https://coverartarchive.org/release/%1/front").arg(releaseId);

    QNetworkRequest request(frontUrl);
    request.setRawHeader("User-Agent", "MediaConsole/2.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_nam->get(request);

    QTimer::singleShot(kRequestTimeoutMs, reply,
                       [reply]()
                       {
                           if (reply->isRunning())
                           {
                               reply->abort();
                           }
                       });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, releaseId, discId]()
            {
                QString frontPath;

                if (reply->error() == QNetworkReply::NoError)
                {
                    QByteArray data = reply->readAll();
                    if (!data.isEmpty())
                    {
                        frontPath = saveToDisk(discId, "front", data);
                    }
                }

                reply->deleteLater();

                if (frontPath.isEmpty())
                {
                    qCInfo(mediaCd) << "CdAlbumArtProvider: CoverArtArchive front failed for" << discId;
                    // Try Discogs as fallback
                    downloadFromDiscogs(discId);
                    return;
                }

                qCInfo(mediaCd) << "CdAlbumArtProvider: front cover saved to" << frontPath;

                // Try back cover (optional)
                QString backUrl = QString("https://coverartarchive.org/release/%1/back").arg(releaseId);
                QNetworkRequest backRequest(backUrl);
                backRequest.setRawHeader("User-Agent", "MediaConsole/2.0");
                backRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                         QNetworkRequest::NoLessSafeRedirectPolicy);

                QNetworkReply* backReply = m_nam->get(backRequest);

                QTimer::singleShot(kRequestTimeoutMs, backReply,
                                   [backReply]()
                                   {
                                       if (backReply->isRunning())
                                       {
                                           backReply->abort();
                                       }
                                   });

                connect(backReply, &QNetworkReply::finished, this,
                        [this, backReply, discId, frontPath]()
                        {
                            QString backPath;

                            if (backReply->error() == QNetworkReply::NoError)
                            {
                                QByteArray data = backReply->readAll();
                                if (!data.isEmpty())
                                {
                                    backPath = saveToDisk(discId, "back", data);
                                }
                            }

                            backReply->deleteLater();

                            // Emit with whatever we got (front is required, back is optional)
                            emit albumArtReady(discId, frontPath, backPath);
                        });
            });
}

void CdAlbumArtProvider::downloadFromDiscogs(const QString& discId)
{
    // Discogs requires API key for reliable image access
    // For now, emit failure -- can be enhanced later with Discogs API key
    qCInfo(mediaCd) << "CdAlbumArtProvider: Discogs fallback not yet implemented for" << discId;
    emit albumArtFailed(discId);
}

QString CdAlbumArtProvider::saveToDisk(const QString& discId, const QString& type, const QByteArray& data)
{
    // Detect image format from magic bytes
    QString extension = "jpg";
    if (data.startsWith("\x89PNG"))
    {
        extension = "png";
    }

    QString filename = QString("%1/%2_%3.%4").arg(m_cacheDir, discId, type, extension);

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        qCWarning(mediaCd) << "CdAlbumArtProvider: failed to write" << filename;
        return {};
    }

    file.write(data);
    file.close();

    return filename;
}
