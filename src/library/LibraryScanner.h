#pragma once

#include <QFutureWatcher>
#include <QMap>
#include <QObject>
#include <QVector>

#include <atomic>

struct LibraryTrack;

/// Recursive FLAC file scanner with TagLib metadata extraction.
/// Runs asynchronously via QtConcurrent. Emits batches of scanned tracks
/// for main thread DB insertion. Supports incremental scanning via mtime comparison.
class LibraryScanner : public QObject
{
    Q_OBJECT
public:
    explicit LibraryScanner(QObject* parent = nullptr);
    ~LibraryScanner() override;

    /// Start async scan. existingMtimes maps filePath -> mtime for skip logic.
    void startScan(const QString& rootPath, const QMap<QString, qint64>& existingMtimes);

    /// Request cancellation. Scan stops at next file boundary.
    void cancel();

    /// Returns true if scan is currently running.
    bool isScanning() const;

signals:
    void batchReady(const QVector<LibraryTrack>& tracks);
    void scanProgress(int processed, int total);
    void scanComplete(int totalProcessed, int totalSkipped);
    void scanError(const QString& filePath, const QString& error);

private:
    void doScan(const QString& rootPath, const QMap<QString, qint64>& existingMtimes);
    bool extractMetadata(const QString& filePath, LibraryTrack& track);

    QFutureWatcher<void> m_watcher;
    std::atomic<bool> m_cancelled { false };

    static constexpr int kBatchSize = 50;
};
