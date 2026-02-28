#include "CdMetadataProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>

#include "utils/Logging.h"

CdMetadataProvider::CdMetadataProvider(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent)
    , m_nam(nam)
{
}

void CdMetadataProvider::lookup(const QString& discId, const QVector<TocEntry>& toc)
{
    cancel();
    m_discId = discId;
    m_toc = toc;
    m_metadata = CdMetadata();
    m_metadata.discId = discId;

    lookupMusicBrainz();
}

void CdMetadataProvider::cancel()
{
    if (m_activeReply)
    {
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply = nullptr;
    }
}

void CdMetadataProvider::lookupMusicBrainz()
{
    QString url = QString("https://musicbrainz.org/ws/2/discid/%1?fmt=json&inc=recordings+artists").arg(m_discId);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MediaConsole/2.0 (https://github.com/rory)");
    request.setRawHeader("Accept", "application/json");

    m_activeReply = m_nam->get(request);

    // Timeout
    QTimer::singleShot(kRequestTimeoutMs, m_activeReply,
                       [this]()
                       {
                           if (m_activeReply)
                           {
                               m_activeReply->abort();
                           }
                       });

    connect(m_activeReply, &QNetworkReply::finished, this,
            [this]()
            {
                auto* reply = m_activeReply;
                m_activeReply = nullptr;

                if (!reply)
                {
                    return;
                }

                bool success = false;
                if (reply->error() == QNetworkReply::NoError)
                {
                    success = parseMusicBrainzResponse(reply->readAll());
                }

                reply->deleteLater();

                if (success)
                {
                    m_metadata.source = "musicbrainz";
                    qCInfo(mediaCd) << "CdMetadataProvider: MusicBrainz success for" << m_discId;
                    emit metadataReady(m_metadata);
                }
                else
                {
                    qCInfo(mediaCd) << "CdMetadataProvider: MusicBrainz failed, trying GnuDB";
                    lookupGnuDb();
                }
            });
}

bool CdMetadataProvider::parseMusicBrainzResponse(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray releases = root["releases"].toArray();
    if (releases.isEmpty())
    {
        return false;
    }

    // Take first release
    QJsonObject release = releases[0].toObject();
    m_metadata.album = release["title"].toString();
    m_metadata.musicbrainzReleaseId = release["id"].toString();

    // Extract artist from artist-credit
    QJsonArray artistCredit = release["artist-credit"].toArray();
    if (!artistCredit.isEmpty())
    {
        m_metadata.artist = artistCredit[0].toObject()["artist"].toObject()["name"].toString();
    }

    // Extract year from date
    QString date = release["date"].toString();
    if (date.length() >= 4)
    {
        m_metadata.year = date.left(4).toInt();
    }

    // Extract tracks from media[0].tracks
    QJsonArray media = release["media"].toArray();
    if (!media.isEmpty())
    {
        QJsonArray tracks = media[0].toObject()["tracks"].toArray();
        for (int i = 0; i < tracks.size(); ++i)
        {
            QJsonObject trackObj = tracks[i].toObject();
            CdTrackInfo track;
            track.trackNumber = trackObj["position"].toInt();
            track.title = trackObj["title"].toString();
            track.durationSeconds = trackObj["length"].toInt() / 1000; // ms to seconds

            // Per-track artist
            QJsonArray trackArtistCredit = trackObj["artist-credit"].toArray();
            if (!trackArtistCredit.isEmpty())
            {
                track.artist = trackArtistCredit[0].toObject()["artist"].toObject()["name"].toString();
            }
            else
            {
                track.artist = m_metadata.artist;
            }

            m_metadata.tracks.append(track);
        }
    }

    return !m_metadata.album.isEmpty();
}

void CdMetadataProvider::lookupGnuDb()
{
    QString queryUrl = buildGnuDbQueryUrl();
    if (queryUrl.isEmpty())
    {
        qCInfo(mediaCd) << "CdMetadataProvider: Cannot build GnuDB query, trying Discogs";
        lookupDiscogs();
        return;
    }

    QNetworkRequest request(queryUrl);
    request.setRawHeader("User-Agent", "MediaConsole/2.0");

    m_activeReply = m_nam->get(request);

    QTimer::singleShot(kRequestTimeoutMs, m_activeReply,
                       [this]()
                       {
                           if (m_activeReply)
                           {
                               m_activeReply->abort();
                           }
                       });

    connect(m_activeReply, &QNetworkReply::finished, this,
            [this]()
            {
                auto* reply = m_activeReply;
                m_activeReply = nullptr;

                if (!reply)
                {
                    return;
                }

                if (reply->error() != QNetworkReply::NoError)
                {
                    reply->deleteLater();
                    qCInfo(mediaCd) << "CdMetadataProvider: GnuDB query failed, trying Discogs";
                    lookupDiscogs();
                    return;
                }

                QByteArray responseData = reply->readAll();
                reply->deleteLater();

                // Parse query response to get category and disc ID for read command
                QString response = QString::fromUtf8(responseData);
                QStringList lines = response.split('\n', Qt::SkipEmptyParts);

                if (lines.isEmpty()
                    || !lines[0].startsWith("200") && !lines[0].startsWith("210") && !lines[0].startsWith("211"))
                {
                    qCInfo(mediaCd) << "CdMetadataProvider: GnuDB query no matches, trying Discogs";
                    lookupDiscogs();
                    return;
                }

                // Parse first match: "200 category discid dtitle"
                // or "210 Found exact matches\ncategory discid dtitle\n."
                QString matchLine;
                if (lines[0].startsWith("200"))
                {
                    matchLine = lines[0].mid(4); // Skip "200 "
                }
                else if (lines.size() > 1)
                {
                    matchLine = lines[1];
                }

                if (matchLine.isEmpty())
                {
                    lookupDiscogs();
                    return;
                }

                QStringList parts = matchLine.split(' ');
                if (parts.size() < 2)
                {
                    lookupDiscogs();
                    return;
                }

                QString category = parts[0];
                QString gnudbDiscId = parts[1];

                // Second request: read full entry
                QString readUrl = buildGnuDbReadUrl(category, gnudbDiscId);
                QNetworkRequest readRequest(readUrl);
                readRequest.setRawHeader("User-Agent", "MediaConsole/2.0");

                m_activeReply = m_nam->get(readRequest);

                QTimer::singleShot(kRequestTimeoutMs, m_activeReply,
                                   [this]()
                                   {
                                       if (m_activeReply)
                                       {
                                           m_activeReply->abort();
                                       }
                                   });

                connect(m_activeReply, &QNetworkReply::finished, this,
                        [this]()
                        {
                            auto* readReply = m_activeReply;
                            m_activeReply = nullptr;

                            if (!readReply)
                            {
                                return;
                            }

                            bool success = false;
                            if (readReply->error() == QNetworkReply::NoError)
                            {
                                success = parseGnuDbResponse(readReply->readAll());
                            }

                            readReply->deleteLater();

                            if (success)
                            {
                                m_metadata.source = "gnudb";
                                qCInfo(mediaCd) << "CdMetadataProvider: GnuDB success for" << m_discId;
                                emit metadataReady(m_metadata);
                            }
                            else
                            {
                                qCInfo(mediaCd) << "CdMetadataProvider: GnuDB read failed, trying Discogs";
                                lookupDiscogs();
                            }
                        });
            });
}

bool CdMetadataProvider::parseGnuDbResponse(const QByteArray& data)
{
    // Try UTF-8 first, fall back to Latin-1
    QString response = QString::fromUtf8(data);
    if (response.contains(QChar(0xFFFD))) // Replacement character = invalid UTF-8
    {
        response = QString::fromLatin1(data);
    }

    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty())
    {
        return false;
    }

    // Skip status line
    int start = 0;
    if (lines[0].startsWith("210"))
    {
        start = 1;
    }

    QString dtitle;
    int trackCount = 0;
    QStringList trackTitles;

    for (int i = start; i < lines.size(); ++i)
    {
        const QString& line = lines[i];
        if (line == ".")
        {
            break;
        }

        if (line.startsWith("DTITLE="))
        {
            dtitle += line.mid(7);
        }
        else if (line.startsWith("TTITLE"))
        {
            int eqPos = line.indexOf('=');
            if (eqPos > 0)
            {
                trackTitles.append(line.mid(eqPos + 1));
                trackCount++;
            }
        }
        else if (line.startsWith("DYEAR="))
        {
            m_metadata.year = line.mid(6).trimmed().toInt();
        }
        else if (line.startsWith("DGENRE="))
        {
            m_metadata.genre = line.mid(7).trimmed();
        }
    }

    // Validate track count matches TOC (CD-09)
    if (!validateGnuDbTrackCount(trackCount))
    {
        qCWarning(mediaCd) << "CdMetadataProvider: GnuDB track count mismatch:" << trackCount << "vs TOC"
                           << m_toc.size();
        return false;
    }

    // Parse DTITLE: "Artist / Album"
    auto [artist, album] = parseDTitle(dtitle);
    m_metadata.artist = artist;
    m_metadata.album = album;

    // Build track list
    m_metadata.tracks.clear();
    for (int i = 0; i < trackTitles.size() && i < m_toc.size(); ++i)
    {
        CdTrackInfo track;
        track.trackNumber = i + 1;
        track.title = trackTitles[i].trimmed();
        track.artist = m_metadata.artist;
        track.durationSeconds = m_toc[i].durationSeconds;
        m_metadata.tracks.append(track);
    }

    return !m_metadata.album.isEmpty();
}

void CdMetadataProvider::lookupDiscogs()
{
    // Discogs search by track count -- requires API key for reliable access
    // For now, simplified search
    QString url = QString("https://api.discogs.com/database/search?type=release&format=CD&per_page=5");

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MediaConsole/2.0 +https://github.com/rory");

    m_activeReply = m_nam->get(request);

    QTimer::singleShot(kRequestTimeoutMs, m_activeReply,
                       [this]()
                       {
                           if (m_activeReply)
                           {
                               m_activeReply->abort();
                           }
                       });

    connect(m_activeReply, &QNetworkReply::finished, this,
            [this]()
            {
                auto* reply = m_activeReply;
                m_activeReply = nullptr;

                if (!reply)
                {
                    return;
                }

                bool success = false;
                if (reply->error() == QNetworkReply::NoError)
                {
                    success = parseDiscogsResponse(reply->readAll());
                }

                reply->deleteLater();

                if (success)
                {
                    m_metadata.source = "discogs";
                    qCInfo(mediaCd) << "CdMetadataProvider: Discogs success for" << m_discId;
                    emit metadataReady(m_metadata);
                }
                else
                {
                    qCInfo(mediaCd) << "CdMetadataProvider: all sources failed for" << m_discId;
                    emit lookupFailed(m_discId);
                }
            });
}

bool CdMetadataProvider::parseDiscogsResponse(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray results = root["results"].toArray();
    if (results.isEmpty())
    {
        return false;
    }

    QJsonObject first = results[0].toObject();
    QString title = first["title"].toString(); // "Artist - Album"
    int dashPos = title.indexOf(" - ");
    if (dashPos > 0)
    {
        m_metadata.artist = title.left(dashPos);
        m_metadata.album = title.mid(dashPos + 3);
    }
    else
    {
        m_metadata.album = title;
    }

    m_metadata.year = first["year"].toString().toInt();

    return !m_metadata.album.isEmpty();
}

bool CdMetadataProvider::validateGnuDbTrackCount(int responseTrackCount)
{
    return responseTrackCount == m_toc.size();
}

quint32 CdMetadataProvider::computeFreedbDiscId(const QVector<TocEntry>& toc)
{
    if (toc.isEmpty())
    {
        return 0;
    }

    // Sum of digit sums of start second offsets
    auto digitSum = [](int n) -> int
    {
        int sum = 0;
        while (n > 0)
        {
            sum += n % 10;
            n /= 10;
        }
        return sum;
    };

    int n = 0;
    for (const auto& entry : toc)
    {
        int seconds = entry.startSector / 75;
        n += digitSum(seconds);
    }

    // Total disc length in seconds
    int lastEnd = toc.last().endSector;
    int firstStart = toc.first().startSector;
    int totalSeconds = (lastEnd - firstStart) / 75;

    quint32 discId = (static_cast<quint32>(n % 0xFF) << 24) | (static_cast<quint32>(totalSeconds) << 8)
        | static_cast<quint32>(toc.size());

    return discId;
}

QPair<QString, QString> CdMetadataProvider::parseDTitle(const QString& dtitle)
{
    // DTITLE format: "Artist / Album"
    // Use first occurrence of " / " as separator
    int sepPos = dtitle.indexOf(" / ");
    if (sepPos > 0)
    {
        return { dtitle.left(sepPos).trimmed(), dtitle.mid(sepPos + 3).trimmed() };
    }
    // No separator: treat entire string as album
    return { QString(), dtitle.trimmed() };
}

QString CdMetadataProvider::buildGnuDbQueryUrl()
{
    if (m_toc.isEmpty())
    {
        return {};
    }

    quint32 freedbId = computeFreedbDiscId(m_toc);
    QString discIdHex = QString("%1").arg(freedbId, 8, 16, QChar('0'));

    // Build offset list (sector offsets + 150 for 2-second CD pregap)
    QStringList offsets;
    for (const auto& entry : m_toc)
    {
        offsets.append(QString::number(entry.startSector + 150));
    }

    // Total disc length in seconds
    int totalSeconds = (m_toc.last().endSector + 150) / 75;

    QString cmd
        = QString("cddb+query+%1+%2+%3+%4").arg(discIdHex).arg(m_toc.size()).arg(offsets.join('+')).arg(totalSeconds);

    return QString("http://gnudb.gnudb.org/~cddb/cddb.cgi?cmd=%1"
                   "&hello=user+hostname+MediaConsole+2.0&proto=6")
        .arg(cmd);
}

QString CdMetadataProvider::buildGnuDbReadUrl(const QString& category, const QString& discId)
{
    return QString("http://gnudb.gnudb.org/~cddb/cddb.cgi?cmd=cddb+read+%1+%2"
                   "&hello=user+hostname+MediaConsole+2.0&proto=6")
        .arg(category, discId);
}
