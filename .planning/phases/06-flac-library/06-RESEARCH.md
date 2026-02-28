# Phase 6: FLAC Library - Research

**Researched:** 2026-02-28
**Domain:** Local FLAC library management with SQLite indexing, hierarchical browse models, and audio playback
**Confidence:** HIGH

## Summary

Phase 6 builds the FLAC library subsystem: a recursive directory scanner with TagLib metadata extraction, a SQLite-backed library database, three hierarchical QAbstractListModel subclasses for browsing, album art extraction/caching, and a FlacAudioStream for playback. The phase follows established patterns from the CD subsystem (CdMetadataCache for SQLite, CdAlbumArtProvider for art caching, CdController for lifecycle orchestration) and the audio pipeline (AudioStream interface, LocalPlaybackController for playback).

The three external libraries are TagLib (metadata extraction from FLAC files), libsndfile (FLAC decoding to raw PCM), and libsamplerate (resampling non-44100Hz files to the required 44100Hz output). All three are mature C libraries available via pkg-config on Raspberry Pi OS Trixie.

**Primary recommendation:** Follow the CdMetadataCache/CdController patterns closely. Use TagLib for metadata, libsndfile+libsamplerate for audio, QtConcurrent for async scanning, and QSqlDatabase for the library DB. The three browse models are straightforward QAbstractListModel subclasses with filter properties.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Automatic incremental scan on app startup (unchanged files skipped via mtime per FLAC-04)
- Emit scanProgress(current, total) signal for optional UI binding -- Phase 10 decides how to display it
- Library is browsable during an active scan -- models show whatever is in the DB, new entries appear as they're indexed
- Medium library size expected (~1,000-5,000 albums) -- incremental scanning keeps subsequent startups fast
- Artists: sorted alphabetically by name, each row shows artist name + album count
- Albums within an artist: sorted by year (oldest first -- chronological discography order)
- Album row info: thumbnail album art + album title + year + track count
- Track row info: track number + title + per-track artist + duration (handles compilations/various artists)
- Multi-disc albums: flat list ordered by disc number then track number (no disc headers/separators in model)
- Selecting a track queues the whole album as the playlist, starting at the selected track
- Next/previous navigates within the album (disc-aware ordering)
- Album finishes -> stop playback (no looping, no continuation to next album)
- No shuffle -- always sequential by disc then track number
- Playback is always user-initiated (consistent with CD-04 pattern)
- Extract front + back covers from FLAC picture blocks (consistent with CdAlbumArtInfo pattern)
- Fallback: scan album directory for cover.jpg, folder.jpg, front.jpg (and back variants)
- Final fallback: generic placeholder
- Art extraction happens during library scan (not on-demand) -- art is ready when user browses
- Cached in dedicated directory (~/.cache/media-console/album-art/) with SHA-1(artist+album) filenames per FLAC-05
- Never modify the user's music library directory

### Claude's Discretion
- SQLite schema design and query optimization
- QtConcurrent threading strategy for async scan
- Exact TagLib metadata extraction approach
- libsndfile + libsamplerate integration details for FlacAudioStream
- How browse models refresh during active scan (signal strategy)
- Error handling for corrupt/unreadable FLAC files

### Deferred Ideas (OUT OF SCOPE)
- Shuffle playback mode -- could be added as Phase 10 UI toggle
- Continue to next album after current finishes -- potential future enhancement
- File system watcher for live library updates -- startup scan is sufficient for now
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FLAC-01 | FlacAudioStream wraps libsndfile decoding + libsamplerate resampling to 44100Hz/16-bit/stereo | libsndfile sf_readf_short + libsamplerate SRC_SINC_MEDIUM_QUALITY; follows AudioStream interface pattern |
| FLAC-02 | Recursive directory scanner using TagLib for metadata extraction | TagLib::FileRef + TagLib::FLAC::File for metadata; QDirIterator for recursive traversal |
| FLAC-03 | SQLite database indexing: title, artist, album artist, album, track/disc number, year, genre, duration, sample rate, bit depth, file path, modification time | QSqlDatabase pattern from CdMetadataCache; single tracks table with indexes on album_artist, album |
| FLAC-04 | Incremental scanning: skip unchanged files based on mtime | Compare file mtime against stored mtime in DB; skip if match |
| FLAC-05 | Album art extraction from FLAC picture blocks, cached via SHA-1(artist+album) filename with MIME type auto-detection | TagLib::FLAC::File::pictureList() for embedded art; file fallback for cover.jpg/folder.jpg/front.jpg |
| FLAC-06 | Async scanning via QtConcurrent | QtConcurrent::run for scanner; signals back to main thread for DB writes and model refresh |
| FLAC-07 | Three hierarchical QAbstractListModel subclasses: LibraryArtistModel, LibraryAlbumModel, LibraryTrackBrowseModel | QAbstractListModel with roleNames, rowCount, data; filter properties for drill-down |
| FLAC-08 | Playlist-based playback with next/previous navigation and associated LibraryTrack metadata | FlacLibraryController manages playlist vector, current index, next/previous logic |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| TagLib | 1.x (apt: libtag1-dev) | FLAC metadata extraction (title, artist, album, year, genre, track/disc number, duration, sample rate, bit depth) and embedded picture extraction | De facto standard for audio metadata in C++. Handles all tag formats (Vorbis comments in FLAC). Available on Raspberry Pi OS Trixie. |
| libsndfile | 1.x (apt: libsndfile1-dev) | FLAC audio decoding to raw PCM frames | Standard C library for reading audio files. Handles FLAC natively (no separate FLAC library needed). Provides sf_readf_short for 16-bit output. |
| libsamplerate | 0.2.x (apt: libsamplerate0-dev) | Sample rate conversion for non-44100Hz FLAC files | Standard resampling library (Secret Rabbit Code). SRC_SINC_MEDIUM_QUALITY balances quality/speed. |
| Qt6::Sql | 6.8.2 | SQLite database for library index | Already in use (CdMetadataCache). QSqlDatabase with QSQLITE driver. |
| Qt6::Concurrent | 6.8.2 | Async scanning on background thread | Qt standard for thread offloading. QtConcurrent::run with QFutureWatcher. |

### Supporting
| Library | Purpose | When to Use |
|---------|---------|-------------|
| QCryptographicHash | SHA-1 hash for album art filenames | SHA-1(artist+album) per FLAC-05 |
| QDirIterator | Recursive FLAC file discovery | QDirIterator::Subdirectories flag |
| QFileInfo | File modification time for incremental scan | mtime comparison per FLAC-04 |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| libsndfile | FLAC library directly | libsndfile handles format detection and PCM conversion; direct FLAC lib adds complexity for no benefit |
| libsamplerate | speex resampler | libsamplerate has better quality for music; speex optimized for speech |
| TagLib | libav/ffmpeg | TagLib is lighter weight, no media codec dependency, sufficient for metadata-only use |

**Installation (Raspberry Pi OS Trixie):**
```bash
sudo apt install libtag1-dev libsndfile1-dev libsamplerate0-dev
```

## Architecture Patterns

### Recommended Project Structure
```
src/library/
    FlacAudioStream.h           # AudioStream implementation (FLAC-01)
    FlacAudioStream.cpp
    LibraryScanner.h            # Recursive scanner with TagLib (FLAC-02, FLAC-04, FLAC-06)
    LibraryScanner.cpp
    LibraryDatabase.h           # SQLite library index (FLAC-03)
    LibraryDatabase.cpp
    LibraryAlbumArtProvider.h   # Art extraction and caching (FLAC-05)
    LibraryAlbumArtProvider.cpp
    LibraryArtistModel.h        # QAbstractListModel (FLAC-07)
    LibraryArtistModel.cpp
    LibraryAlbumModel.h         # QAbstractListModel (FLAC-07)
    LibraryAlbumModel.cpp
    LibraryTrackModel.h         # QAbstractListModel (FLAC-07)
    LibraryTrackModel.cpp
    FlacLibraryController.h     # Lifecycle orchestrator (FLAC-08)
    FlacLibraryController.cpp
```

### Pattern 1: SQLite Database (from CdMetadataCache)
**What:** QSqlDatabase with named connection, CREATE TABLE IF NOT EXISTS, prepared statements
**When to use:** Library index storage and querying
**Example:**
```cpp
// Same pattern as CdMetadataCache -- unique connection name per instance
LibraryDatabase::LibraryDatabase(QObject* parent)
    : QObject(parent)
    , m_connectionName("library_db_" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

bool LibraryDatabase::open(const QString& dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);
    if (!db.open()) return false;
    return createTables();
}
```

### Pattern 2: Lifecycle Orchestrator (from CdController)
**What:** Controller owns sub-components, wires signals, manages lifecycle
**When to use:** FlacLibraryController coordinates scanner, database, art provider, models, playback
**Example:**
```cpp
// Same as CdController: owns components, wires signals, manages state
FlacLibraryController::FlacLibraryController(
    LocalPlaybackController* playbackController,
    PlaybackState* playbackState,
    const LibraryConfig& config,
    QObject* parent)
    : QObject(parent)
    , m_playbackController(playbackController) // Non-owning
    , m_playbackState(playbackState)           // Non-owning
{
    m_database = std::make_unique<LibraryDatabase>(this);
    m_scanner = std::make_unique<LibraryScanner>(this);
    m_artProvider = std::make_unique<LibraryAlbumArtProvider>(artCacheDir, this);
    // Wire signals...
}
```

### Pattern 3: QAbstractListModel with Filter
**What:** Model queries SQLite and exposes roles; filter property triggers re-query
**When to use:** All three browse models
**Example:**
```cpp
class LibraryAlbumModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString artistFilter READ artistFilter WRITE setArtistFilter NOTIFY artistFilterChanged)

public:
    enum Roles { AlbumRole = Qt::UserRole + 1, YearRole, TrackCountRole, ArtPathRole };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setArtistFilter(const QString& artist); // Re-queries DB, emits modelReset
};
```

### Pattern 4: AppBuilder Wiring (from Phase 5)
**What:** AppBuilder creates controller, adds to AppContext, auto-starts
**When to use:** Final plan wires FlacLibraryController into composition root

### Anti-Patterns to Avoid
- **QSqlDatabase in threads:** Qt docs forbid using QSqlDatabase across threads. Scanner thread must NOT write to DB directly. Signal results back to main thread for DB writes.
- **Blocking main thread during scan:** All file I/O and TagLib reads must happen on background thread via QtConcurrent.
- **Direct model manipulation from background thread:** Model updates (beginInsertRows, endInsertRows) must happen on main thread.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| FLAC metadata parsing | Custom Vorbis comment parser | TagLib::FileRef / TagLib::FLAC::File | Handles all edge cases, multiple tag formats, Unicode |
| FLAC audio decoding | Custom FLAC decoder | libsndfile sf_open / sf_readf_short | Handles all FLAC variants, bit depths, channel configs |
| Sample rate conversion | Linear interpolation | libsamplerate src_simple or SRC_STATE | Proper sinc interpolation, anti-aliasing, no artifacts |
| SHA-1 hashing | Custom hash | QCryptographicHash::Sha1 | Qt built-in, correct and fast |
| Recursive directory walk | Manual recursion | QDirIterator with Subdirectories | Handles symlinks, permissions, edge cases |

**Key insight:** Audio metadata and decoding have decades of edge cases. TagLib and libsndfile handle all of them; custom parsers will break on real-world FLAC collections.

## Common Pitfalls

### Pitfall 1: QSqlDatabase Thread Safety
**What goes wrong:** Creating or using QSqlDatabase on a background thread causes "database not open" or crashes
**Why it happens:** Qt SQL connections are bound to the creating thread
**How to avoid:** Scanner runs on background thread, emits results via signal. Main thread handles all DB writes. Database queries for models also on main thread.
**Warning signs:** "QSqlQuery: database not open" in logs

### Pitfall 2: TagLib Null Pointers
**What goes wrong:** Crash when accessing tag() or audioProperties() on corrupt FLAC files
**Why it happens:** TagLib::FileRef::tag() and audioProperties() can return nullptr for unreadable files
**How to avoid:** Always null-check tag() and audioProperties() before accessing. Skip file on nullptr.
**Warning signs:** SIGSEGV in TagLib code paths

### Pitfall 3: libsndfile Format Mismatch
**What goes wrong:** FlacAudioStream outputs wrong sample rate or distorted audio
**Why it happens:** FLAC files can be any sample rate (44100, 48000, 88200, 96000, etc.)
**How to avoid:** Always check sf_info.samplerate after open. If not 44100, use libsamplerate to resample. AudioStream contract requires 44100Hz output.
**Warning signs:** Fast/slow playback, pitch shifting, static noise

### Pitfall 4: Album Artist vs Track Artist
**What goes wrong:** Compilation albums split into many "artists" instead of grouping under "Various Artists"
**Why it happens:** Using per-track ARTIST tag instead of ALBUMARTIST for browse grouping
**How to avoid:** Use ALBUMARTIST for artist-level grouping. Fall back to ARTIST if ALBUMARTIST is empty. Store both in database.
**Warning signs:** "Various Artists" albums appear under each contributing artist

### Pitfall 5: Large Model Resets During Scan
**What goes wrong:** UI freezes or flickers during scan as models reset repeatedly
**Why it happens:** Calling beginResetModel/endResetModel for every scanned file
**How to avoid:** Batch updates. Scanner emits batches (e.g., every 50 files or every 2 seconds). Models refresh once per batch.
**Warning signs:** UI unresponsive during scan, high CPU on main thread

### Pitfall 6: libsamplerate Buffer Sizing
**What goes wrong:** Buffer overflow or truncated audio when resampling
**Why it happens:** Output frame count differs from input frame count when ratio != 1.0
**How to avoid:** Calculate output frames as ceil(input_frames * output_rate / input_rate). Allocate output buffer accordingly. Handle partial conversions.
**Warning signs:** Clicks at buffer boundaries, truncated tracks

## Code Examples

### TagLib Metadata Extraction
```cpp
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

bool extractMetadata(const QString& path, LibraryTrack& track)
{
    TagLib::FileRef fileRef(path.toUtf8().constData());
    if (fileRef.isNull() || !fileRef.tag())
        return false;

    TagLib::Tag* tag = fileRef.tag();
    track.title = QString::fromStdString(tag->title().to8Bit(true));
    track.artist = QString::fromStdString(tag->artist().to8Bit(true));
    track.album = QString::fromStdString(tag->album().to8Bit(true));
    track.genre = QString::fromStdString(tag->genre().to8Bit(true));
    track.year = tag->year();
    track.trackNumber = tag->track();

    // Album artist from properties (Vorbis comment ALBUMARTIST)
    TagLib::PropertyMap props = fileRef.file()->properties();
    if (props.contains("ALBUMARTIST"))
        track.albumArtist = QString::fromStdString(
            props["ALBUMARTIST"].front().to8Bit(true));
    else
        track.albumArtist = track.artist;

    // Disc number from properties
    if (props.contains("DISCNUMBER"))
    {
        QString discStr = QString::fromStdString(
            props["DISCNUMBER"].front().to8Bit(true));
        track.discNumber = discStr.split('/').first().toInt();
    }

    // Audio properties
    if (auto* audio = fileRef.audioProperties())
    {
        track.durationSeconds = audio->lengthInSeconds();
        track.sampleRate = audio->sampleRate();
        track.bitDepth = audio->bitsPerSample();
    }

    return true;
}
```

### Album Art from FLAC Picture Blocks
```cpp
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>

QString extractFrontCover(const QString& path, const QString& cacheDir, const QString& hash)
{
    TagLib::FLAC::File flacFile(path.toUtf8().constData());
    if (!flacFile.isValid())
        return {};

    for (const auto* pic : flacFile.pictureList())
    {
        if (pic->type() == TagLib::FLAC::Picture::FrontCover)
        {
            QString mimeType = QString::fromStdString(pic->mimeType().to8Bit());
            QString ext = mimeType.contains("png") ? "png" : "jpg";
            QString outPath = QString("%1/%2_front.%3").arg(cacheDir, hash, ext);

            QFile file(outPath);
            if (file.open(QIODevice::WriteOnly))
            {
                file.write(QByteArray(pic->data().data(), pic->data().size()));
                file.close();
                return outPath;
            }
        }
    }
    return {};
}
```

### libsndfile + libsamplerate in FlacAudioStream
```cpp
#include <sndfile.h>
#include <samplerate.h>

bool FlacAudioStream::open()
{
    SF_INFO sfInfo = {};
    m_sndfile = sf_open(m_filePath.toLocal8Bit().data(), SFM_READ, &sfInfo);
    if (!m_sndfile) return false;

    m_nativeRate = sfInfo.samplerate;
    m_channels = sfInfo.channels;
    m_totalNativeFrames = sfInfo.frames;
    m_needsResample = (m_nativeRate != 44100);

    if (m_needsResample)
    {
        int error = 0;
        m_srcState = src_new(SRC_SINC_MEDIUM_QUALITY, m_channels, &error);
        m_srcRatio = 44100.0 / m_nativeRate;
    }
    return true;
}

long FlacAudioStream::readFrames(int16_t* buffer, size_t frames)
{
    if (!m_needsResample)
    {
        // Direct read -- already 44100Hz
        return sf_readf_short(m_sndfile, buffer, frames);
    }

    // Read native frames, resample to 44100
    size_t nativeFrames = static_cast<size_t>(frames / m_srcRatio) + 1;
    // ... resample with src_process ...
}
```

### LibraryDatabase Schema
```sql
CREATE TABLE IF NOT EXISTS tracks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT UNIQUE NOT NULL,
    title TEXT,
    artist TEXT,
    album_artist TEXT,
    album TEXT,
    track_number INTEGER DEFAULT 0,
    disc_number INTEGER DEFAULT 1,
    year INTEGER DEFAULT 0,
    genre TEXT,
    duration_seconds INTEGER DEFAULT 0,
    sample_rate INTEGER DEFAULT 0,
    bit_depth INTEGER DEFAULT 0,
    mtime INTEGER NOT NULL,
    created_at TEXT DEFAULT (datetime('now'))
);

CREATE INDEX IF NOT EXISTS idx_tracks_album_artist ON tracks(album_artist);
CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album_artist, album);
CREATE INDEX IF NOT EXISTS idx_tracks_file_path ON tracks(file_path);
```

### QtConcurrent Scanner Pattern
```cpp
void LibraryScanner::startScan(const QString& rootPath)
{
    m_cancelled.store(false);
    auto future = QtConcurrent::run([this, rootPath]() {
        scanDirectory(rootPath);
    });
    m_watcher.setFuture(future);
}

void LibraryScanner::scanDirectory(const QString& rootPath)
{
    QDirIterator it(rootPath, {"*.flac"}, QDir::Files, QDirIterator::Subdirectories);
    QVector<ScannedTrack> batch;
    int total = 0, processed = 0;

    // Count files first for progress
    // ... or estimate from previous scan count

    while (it.hasNext() && !m_cancelled.load())
    {
        QString path = it.next();
        QFileInfo info(path);

        ScannedTrack track;
        track.filePath = path;
        track.mtime = info.lastModified().toSecsSinceEpoch();

        if (extractMetadata(path, track))
            batch.append(track);

        if (batch.size() >= 50)
        {
            emit batchReady(batch);  // Main thread handles DB insert
            batch.clear();
        }
        emit scanProgress(++processed, total);
    }
    if (!batch.isEmpty())
        emit batchReady(batch);
    emit scanComplete();
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| TagLib 1.x String::to8Bit() | TagLib 2.x toCString(true) | TagLib 2.0 (2023) | Check which version is installed; API may differ slightly |
| libsndfile sf_read_short | sf_readf_short (frame-based) | Long-standing | Always use frame-based reads for correct channel handling |
| Raw pthreads for scanning | QtConcurrent::run | Qt 6 | Standard Qt pattern for background work |

**Note:** Raspberry Pi OS Trixie (Debian 13) ships TagLib 1.x. The code examples above use TagLib 1.x API (to8Bit). If TagLib 2.x is installed, minor adjustments to string conversion may be needed.

## Open Questions

1. **TagLib version on Raspberry Pi OS Trixie**
   - What we know: Debian typically ships TagLib 1.x in stable
   - What's unclear: Exact version in Trixie (may have 2.0)
   - Recommendation: Use TagLib::FileRef API which is stable across versions. Guard string conversion with version check if needed. LOW impact either way.

2. **Mono FLAC files**
   - What we know: AudioStream contract requires stereo output
   - What's unclear: Whether the library has mono FLAC files
   - Recommendation: libsndfile can handle mono files; if channels==1, duplicate to stereo in readFrames. Simple implementation.

## Sources

### Primary (HIGH confidence)
- Existing codebase: CdMetadataCache, CdAlbumArtProvider, CdController, AudioStream, LocalPlaybackController patterns
- Qt 6.8 documentation: QAbstractListModel, QSqlDatabase, QtConcurrent
- TagLib API: FileRef, FLAC::File, PropertyMap, FLAC::Picture

### Secondary (MEDIUM confidence)
- libsndfile documentation: sf_open, sf_readf_short, SF_INFO struct
- libsamplerate documentation: src_new, src_process, SRC_SINC_MEDIUM_QUALITY

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All libraries are mature, well-documented, and already proven in the project (SQLite, Qt patterns) or standard Linux audio stack (TagLib, libsndfile, libsamplerate)
- Architecture: HIGH - Directly follows established CdController/CdMetadataCache patterns from Phase 5
- Pitfalls: HIGH - Well-known issues with TagLib null checks, thread safety, resampling math

**Research date:** 2026-02-28
**Valid until:** 2026-03-30 (stable libraries, patterns won't change)
