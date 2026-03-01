#include "LibraryScanner.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QtConcurrent>

#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

#include "library/LibraryDatabase.h"
#include "utils/Logging.h"

LibraryScanner::LibraryScanner(QObject* parent)
    : QObject(parent)
{
}

LibraryScanner::~LibraryScanner()
{
    cancel();
    m_watcher.waitForFinished();
}

void LibraryScanner::startScan(const QString& rootPath, const QMap<QString, qint64>& existingMtimes)
{
    if (m_watcher.isRunning())
    {
        qCWarning(mediaLibrary) << "LibraryScanner: scan already in progress";
        return;
    }

    m_cancelled.store(false);
    auto future = QtConcurrent::run([this, rootPath, existingMtimes]() { doScan(rootPath, existingMtimes); });
    m_watcher.setFuture(future);
}

void LibraryScanner::cancel()
{
    m_cancelled.store(true);
}

bool LibraryScanner::isScanning() const
{
    return m_watcher.isRunning();
}

void LibraryScanner::doScan(const QString& rootPath, const QMap<QString, qint64>& existingMtimes)
{
    qCInfo(mediaLibrary) << "LibraryScanner: starting scan of" << rootPath;

    // First pass: count FLAC files for progress reporting
    int total = 0;
    {
        QDirIterator countIt(rootPath, { "*.flac" }, QDir::Files, QDirIterator::Subdirectories);
        while (countIt.hasNext())
        {
            countIt.next();
            ++total;
        }
    }

    qCInfo(mediaLibrary) << "LibraryScanner: found" << total << "FLAC files";

    // Second pass: scan and extract metadata
    QDirIterator it(rootPath, { "*.flac" }, QDir::Files, QDirIterator::Subdirectories);
    QVector<LibraryTrack> batch;
    int processed = 0;
    int skipped = 0;

    while (it.hasNext())
    {
        if (m_cancelled.load())
        {
            qCInfo(mediaLibrary) << "LibraryScanner: cancelled after" << processed << "files";
            break;
        }

        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        qint64 currentMtime = fileInfo.lastModified().toSecsSinceEpoch();

        // Incremental scan: skip unchanged files (FLAC-04)
        if (existingMtimes.value(filePath, -1) == currentMtime)
        {
            ++skipped;
            ++processed;
            emit scanProgress(processed, total);
            continue;
        }

        LibraryTrack track;
        track.filePath = filePath;
        track.mtime = currentMtime;

        if (extractMetadata(filePath, track))
        {
            batch.append(track);
        }

        ++processed;

        if (batch.size() >= kBatchSize)
        {
            emit batchReady(batch);
            batch.clear();
        }

        emit scanProgress(processed, total);
    }

    // Emit remaining batch
    if (!batch.isEmpty())
    {
        emit batchReady(batch);
    }

    int totalProcessed = processed - skipped;
    qCInfo(mediaLibrary) << "LibraryScanner: scan complete. Processed:" << totalProcessed << "Skipped:" << skipped;
    emit scanComplete(totalProcessed, skipped);
}

bool LibraryScanner::extractMetadata(const QString& filePath, LibraryTrack& track)
{
    TagLib::FileRef fileRef(filePath.toUtf8().constData());
    if (fileRef.isNull())
    {
        emit scanError(filePath, "Failed to open file");
        return false;
    }

    TagLib::Tag* tag = fileRef.tag();
    if (!tag)
    {
        emit scanError(filePath, "No tag found");
        return false;
    }

    // Basic tag fields
    track.title = QString::fromStdString(tag->title().to8Bit(true));
    track.artist = QString::fromStdString(tag->artist().to8Bit(true));
    track.album = QString::fromStdString(tag->album().to8Bit(true));
    track.genre = QString::fromStdString(tag->genre().to8Bit(true));
    track.year = static_cast<int>(tag->year());
    track.trackNumber = static_cast<int>(tag->track());

    // Album artist from properties (Vorbis comment ALBUMARTIST)
    TagLib::PropertyMap props = fileRef.file()->properties();
    if (props.contains("ALBUMARTIST") && !props["ALBUMARTIST"].isEmpty())
    {
        track.albumArtist = QString::fromStdString(props["ALBUMARTIST"].front().to8Bit(true));
    }
    else
    {
        track.albumArtist = track.artist;
    }

    // Disc number from properties (handles "1/2" format)
    if (props.contains("DISCNUMBER") && !props["DISCNUMBER"].isEmpty())
    {
        QString discStr = QString::fromStdString(props["DISCNUMBER"].front().to8Bit(true));
        track.discNumber = discStr.split('/').first().toInt();
    }
    else
    {
        track.discNumber = 1;
    }

    // Audio properties
    TagLib::AudioProperties* audio = fileRef.audioProperties();
    if (audio)
    {
        track.durationSeconds = audio->lengthInSeconds();
        track.sampleRate = audio->sampleRate();
        if (auto* flacProps = dynamic_cast<TagLib::FLAC::Properties*>(audio))
            track.bitDepth = flacProps->bitsPerSample();
        else
            track.bitDepth = 0;
    }

    return true;
}
