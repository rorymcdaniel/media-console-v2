# Phase 5: CD Subsystem - Research

**Researched:** 2026-02-28
**Domain:** CD audio extraction, disc metadata lookup, progressive UI updates
**Confidence:** HIGH

## Summary

Phase 5 implements the complete CD playback lifecycle: disc detection, TOC reading, paranoia-corrected audio extraction via CdAudioStream, three-tier async metadata lookup (MusicBrainz, GnuDB, Discogs), SQLite caching, album art downloading, and idle spindle management. The existing codebase provides strong foundations: ICdDrive interface, StubCdDrive, AudioStream base class, LocalPlaybackController with ring buffer, PlaybackState Q_PROPERTY bag, PlatformFactory with TODO slot for real CdDrive, and the mediaCd logging category.

The core challenge is orchestrating multiple async operations (disc detection polling, metadata network I/O, album art downloads) without blocking the Qt event loop, while providing progressive UI updates as data arrives. All network I/O must be fully async with graceful degradation when services are unavailable.

**Primary recommendation:** Use libcdio + libcdio-paranoia for audio extraction, libdiscid for MusicBrainz disc ID calculation, Qt's QNetworkAccessManager for all HTTP metadata lookups (MusicBrainz JSON API, GnuDB CDDB HTTP, Discogs REST, CoverArtArchive), and Qt SQL with SQLite for metadata caching. Keep all network I/O on the main thread via QNetworkAccessManager's async API (it's already non-blocking), and use a QTimer-based polling loop for disc detection with ioctl fallback.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Sequential fallback: MusicBrainz first, then GnuDB if MusicBrainz fails, then Discogs if GnuDB fails -- stop at first success
- Initial display shows track numbers + durations only (no placeholder text like "Unknown Album")
- When metadata arrives, all track titles swap in instantly (batch update, no animation)
- If all three sources fail, silently show "Audio CD" as artist/album -- no error indicators or "metadata unavailable" hints
- CoverArtArchive first (tied to MusicBrainz disc ID), Discogs as fallback only if CoverArtArchive has no art
- Album art fades in over ~300ms when downloaded -- smooth transition even though text uses instant swap
- Front and back cover support as per requirements
- 5-minute idle timer before spindle stop -- configurable via `CdConfig::idleTimeoutSeconds` (default 300)
- During spin-up delay, play button shows loading/spinner state -- playback starts automatically once spun up
- No toast or separate dialog for spin-up -- the button state is sufficient feedback
- Immediate "Reading disc..." loading state when insertion is detected -- appears before TOC is read
- Disc removal during playback: immediate stop, silent clear of track listing -- no toast or confirmation
- Reinsertion of previously-seen disc: re-read TOC, match disc ID against SQLite cache, show full metadata instantly if found (no network lookup)
- Non-audio disc insertion: brief toast notification ("Not an audio disc") -- then nothing else

### Claude's Discretion
- Front/back cover interaction pattern (tap to flip, swipe carousel, etc.)
- No-art placeholder visual (generic CD icon, styled text, etc.)
- Spin-up timeout duration before showing an error
- Loading skeleton/spinner design for "Reading disc..." state
- Exact debounce timing for hybrid disc detection (QFileSystemWatcher + ioctl polling)
- GnuDB response validation specifics
- SQLite cache schema design

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CD-01 | CdAudioStream wraps libcdio with paranoia error correction, implementing AudioStream interface | libcdio-paranoia API: cdio_cddap_open, cdio_paranoia_init, cdio_paranoia_read_limited for sector-by-sector extraction |
| CD-02 | TOC reading via CdDrive (libcdio wrapper) through ICdDrive interface | libcdio cdio_get_first_track_num, cdio_get_num_tracks, cdio_get_track_lba; ICdDrive already defines readToc() returning QVector<TocEntry> |
| CD-03 | Hybrid disc presence detection: QFileSystemWatcher + ioctl polling with debounced state changes | Linux ioctl CDROM_DRIVE_STATUS returns CDS_DISC_OK/CDS_NO_DISC; QTimer-based polling with debounce |
| CD-04 | CD playback is ALWAYS user-initiated -- no auto-play on insert or startup | CdController state machine: disc detected -> show TOC, never auto-play |
| CD-05 | Three-tier async metadata lookup: MusicBrainz -> GnuDB -> Discogs -- ALL fully async | QNetworkAccessManager for all HTTP; MusicBrainz JSON API, GnuDB CDDB HTTP protocol, Discogs REST API |
| CD-06 | Progressive metadata display: show TOC immediately, fill in titles/artist/album art async | Signal-based: CdController emits tocReady (immediate), then metadataReady (async), then albumArtReady (async) |
| CD-07 | SQLite metadata cache: instant lookup for previously-seen discs by disc ID | Qt SQL + SQLite; disc_id as primary key; single-thread access from main thread |
| CD-08 | Album art downloading from CoverArtArchive and Discogs, cached to disk | CoverArtArchive has no rate limits; Discogs requires auth token; both via QNetworkAccessManager |
| CD-09 | GnuDB response validation before caching | CDDB response parsing: validate DGENRE, DTITLE, TTITLE fields; reject if track count mismatch |
| CD-10 | Graceful degradation: fall back to generic "Audio CD" metadata if all sources fail | After cascade exhaustion, set artist="Audio CD", album="Audio CD" silently |
| CD-11 | Idle timer stops CD spindle after period of inactivity | QTimer with configurable CdConfig::idleTimeoutSeconds; ICdDrive::stopSpindle() |
| CD-12 | Spin-up timer handles drive spin-up delay on play | Detect drive not ready after play request; show loading state; poll until ready then start playback |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libcdio | 2.1.0+ | CD-ROM device access, TOC reading | GNU standard for CD I/O on Linux; used by VLC, GStreamer, cd-paranoia |
| libcdio-paranoia | 2.0.2+ | Error-corrected audio extraction | Fork of cdparanoia with libcdio integration; industry standard for jitter-free ripping |
| libdiscid | 0.6.5 | MusicBrainz disc ID calculation | Official MusicBrainz library; handles non-standard Base64 encoding (., _, - instead of +, /, =) |
| Qt6::Sql | 6.8.2 | SQLite metadata cache | Already in project Qt6 stack; provides QSqlDatabase/QSqlQuery with SQLite driver |
| Qt6::Network | 6.8.2 | HTTP requests for metadata APIs | Already linked; QNetworkAccessManager provides async HTTP natively |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QNetworkAccessManager | Qt6 built-in | Async HTTP for MusicBrainz/GnuDB/Discogs/CoverArtArchive | All metadata network I/O |
| QJsonDocument | Qt6 built-in | Parse MusicBrainz JSON and Discogs JSON responses | Metadata response parsing |
| QFileSystemWatcher | Qt6 built-in | Monitor /dev/cdrom for changes | Part of hybrid disc detection |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| libdiscid | Manual disc ID calc from TOC | libdiscid handles MusicBrainz's non-standard Base64; manual calc error-prone |
| Qt SQL + SQLite | Raw SQLite C API | Qt SQL integrates with Qt types; no benefit to raw API |
| QNetworkAccessManager | libcurl | QNAM already linked, integrates with Qt event loop; libcurl adds dependency |

### Installation (Raspberry Pi OS Trixie)
```bash
sudo apt install libcdio-dev libcdio-paranoia-dev libdiscid-dev
```

### CMake Integration
```cmake
# Find system packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(CDIO REQUIRED IMPORTED_TARGET libcdio)
pkg_check_modules(CDIO_PARANOIA REQUIRED IMPORTED_TARGET libcdio_paranoia)
pkg_check_modules(CDIO_CDDA REQUIRED IMPORTED_TARGET libcdio_cdda)
pkg_check_modules(DISCID REQUIRED IMPORTED_TARGET libdiscid)

# Qt SQL module
find_package(Qt6 REQUIRED COMPONENTS Sql)

# Link to library target
target_link_libraries(media-console-lib PUBLIC
    PkgConfig::CDIO
    PkgConfig::CDIO_PARANOIA
    PkgConfig::CDIO_CDDA
    PkgConfig::DISCID
    Qt6::Sql
)
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── cd/
│   ├── CdAudioStream.h/.cpp       # AudioStream impl using paranoia (CD-01)
│   ├── LibcdioCdDrive.h/.cpp       # ICdDrive impl using libcdio (CD-02)
│   ├── CdController.h/.cpp         # Orchestrator: detection, metadata, lifecycle (CD-03,04,06,11,12)
│   ├── CdMetadataProvider.h/.cpp   # Three-tier async lookup cascade (CD-05,09,10)
│   ├── CdMetadataCache.h/.cpp      # SQLite cache layer (CD-07)
│   └── CdAlbumArtProvider.h/.cpp   # Art download + disk cache (CD-08)
├── platform/
│   ├── ICdDrive.h                  # Already exists
│   └── stubs/StubCdDrive.h/.cpp    # Already exists
└── ...
```

### Pattern 1: CdAudioStream as AudioStream Implementation
**What:** CdAudioStream wraps libcdio-paranoia to implement the AudioStream interface. It opens a specific track by seeking to the track's start sector, reads sectors via cdio_paranoia_read_limited, and converts to 16-bit interleaved PCM.
**When to use:** When LocalPlaybackController::play() is called with a CD track.
**Example:**
```cpp
// CdAudioStream opens paranoia for a specific track range
bool CdAudioStream::open()
{
    m_cdda = cdio_cddap_identify(m_devicePath.toLocal8Bit().data(), CDDA_MESSAGE_FORGETIT, nullptr);
    if (!m_cdda) return false;
    if (cdio_cddap_open(m_cdda) != 0) return false;

    m_paranoia = cdio_paranoia_init(m_cdda);
    cdio_paranoia_modeset(m_paranoia, PARANOIA_MODE_FULL);

    // Seek to track start sector
    cdio_paranoia_seek(m_paranoia, m_startSector, SEEK_SET);
    m_currentSector = m_startSector;
    return true;
}

long CdAudioStream::readFrames(int16_t* buffer, size_t frames)
{
    // Each CD sector = 588 frames (2352 bytes / 4 bytes per frame)
    // Read sectors and copy to output buffer
    size_t framesRead = 0;
    while (framesRead < frames && m_currentSector < m_endSector)
    {
        int16_t* sector = cdio_paranoia_read_limited(m_paranoia, nullptr, 10);
        if (!sector) return -1;

        size_t sectorFrames = std::min(kFramesPerSector, frames - framesRead);
        std::memcpy(buffer + framesRead * 2, sector, sectorFrames * 4);
        framesRead += sectorFrames;
        m_currentSector++;
    }
    return static_cast<long>(framesRead);
}
```

### Pattern 2: Async Metadata Cascade
**What:** CdMetadataProvider chains QNetworkAccessManager requests sequentially: MusicBrainz first, GnuDB on failure, Discogs on failure. Each step is a signal/slot chain -- no blocking, no threads.
**When to use:** After TOC is read and disc ID is computed.
**Example:**
```cpp
void CdMetadataProvider::lookup(const QString& discId, const QVector<TocEntry>& toc)
{
    m_discId = discId;
    m_toc = toc;
    lookupMusicBrainz();
}

void CdMetadataProvider::lookupMusicBrainz()
{
    QUrl url(QString("https://musicbrainz.org/ws/2/discid/%1?fmt=json").arg(m_discId));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MediaConsole/2.0 (rory@example.com)");

    auto* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            auto doc = QJsonDocument::fromJson(reply->readAll());
            if (parseMusicBrainzResponse(doc)) {
                emit metadataReady(m_metadata);
                return;
            }
        }
        // Fall through to GnuDB
        lookupGnuDb();
    });
}
```

### Pattern 3: CdController as Lifecycle Orchestrator
**What:** CdController owns the detection loop, coordinates TOC reading, triggers metadata lookup, manages idle timer, and handles spin-up. It is a QObject that lives on the main thread and emits signals for state changes.
**When to use:** Single entry point for all CD subsystem behavior.
**Key signals:**
- `discDetected()` -- triggers "Reading disc..." UI state
- `tocReady(QVector<TocEntry>)` -- track listing available
- `metadataReady(CdMetadata)` -- titles/artist/album filled in
- `albumArtReady(QString frontPath, QString backPath)` -- art cached to disk
- `discRemoved()` -- clear everything
- `spinUpStarted()` / `spinUpComplete()` -- for play button loading state

### Anti-Patterns to Avoid
- **Blocking ioctl on main thread:** The CDROM_DRIVE_STATUS ioctl can block if the drive is busy. Wrap in QtConcurrent::run or use a dedicated polling thread.
- **QSqlDatabase across threads:** A QSqlDatabase connection must only be used by the thread that created it. Since metadata cache is main-thread-only, this is fine -- but never pass the connection to a worker thread.
- **Paranoia reads on main thread:** Audio extraction via cdio_paranoia_read_limited is blocking I/O. It MUST run on the playback thread (already handled by LocalPlaybackController's background thread).
- **Unbounded retry on network failure:** Each metadata source should have a single attempt with a reasonable timeout (10s). No infinite retry loops.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| MusicBrainz disc ID | Custom Base64 + TOC hash | libdiscid | Non-standard Base64 encoding (., _, -); edge cases with multisession discs |
| CD audio extraction | Raw ioctl reads | libcdio-paranoia | Jitter correction, C2 error handling, scratch recovery -- thousands of edge cases |
| CDDB protocol | Raw TCP socket CDDB | GnuDB HTTP interface | HTTP simpler than CDDB protocol; gnudb.org supports HTTP at /~cddb/cddb.cgi |
| JSON parsing | Manual string parsing | QJsonDocument | Already in Qt; handles UTF-8, escaping, nested objects |
| HTTP async | QTcpSocket + manual HTTP | QNetworkAccessManager | Handles redirects, SSL, connection pooling, async natively |

**Key insight:** CD audio is deceptively complex. Libcdio-paranoia handles hundreds of drive-specific quirks, read error recovery, and jitter correction that would take months to replicate.

## Common Pitfalls

### Pitfall 1: CD Sector Size vs PCM Frame Size
**What goes wrong:** Confusing CD sectors (2352 bytes = 588 stereo frames) with PCM frames or ALSA periods.
**Why it happens:** CD audio uses "sectors" of exactly 2352 bytes. Each sector contains 588 stereo 16-bit frames. The AudioStream interface works in frames, not sectors.
**How to avoid:** CdAudioStream internally works in sectors for paranoia reads but exposes frames through the AudioStream interface. Track position/seek converts between frame and sector coordinates.
**Warning signs:** Off-by-one errors in duration calculation, seek landing on wrong position.

### Pitfall 2: libcdio-paranoia Naming Convention
**What goes wrong:** Using cdparanoia function names (paranoia_read) instead of libcdio-paranoia names (cdio_paranoia_read_limited).
**Why it happens:** Many online examples reference the original cdparanoia library, not the libcdio fork.
**How to avoid:** Always use the cdio_ prefix: cdio_cddap_open, cdio_paranoia_init, cdio_paranoia_read_limited, cdio_paranoia_seek, cdio_paranoia_free, cdio_cddap_close.
**Warning signs:** Linker errors about undefined symbols.

### Pitfall 3: MusicBrainz Rate Limiting
**What goes wrong:** Getting HTTP 503 responses from MusicBrainz API.
**Why it happens:** MusicBrainz limits to 1 request per second for unauthenticated clients.
**How to avoid:** Set a proper User-Agent header (required by MusicBrainz TOS). For this use case (single disc lookup), rate limiting is unlikely to be hit -- but add the header anyway.
**Warning signs:** Intermittent 503 errors during metadata lookup.

### Pitfall 4: GnuDB Response Encoding
**What goes wrong:** Garbled track titles from GnuDB responses.
**Why it happens:** GnuDB responses use ISO-8859-1 or UTF-8 depending on the entry; the CDDB protocol doesn't have a clean encoding story.
**How to avoid:** Try UTF-8 first; if invalid, fall back to Latin-1. Validate that track count in response matches TOC track count.
**Warning signs:** Mojibake in track titles, especially for non-English albums.

### Pitfall 5: Disc Removal During Paranoia Read
**What goes wrong:** Segfault or hang when the disc is ejected while paranoia is mid-read.
**Why it happens:** cdio_paranoia_read_limited blocks and may not handle sudden device removal gracefully.
**How to avoid:** The playback thread checks m_stopRequested between reads (already in LocalPlaybackController pattern). On disc removal, CdController calls LocalPlaybackController::stop() which sets the atomic flag, causing the thread to exit the read loop before the next paranoia read.
**Warning signs:** Application hang on disc eject during playback.

### Pitfall 6: QFileSystemWatcher on /dev/cdrom
**What goes wrong:** QFileSystemWatcher may not fire for /dev/cdrom changes on all kernels.
**Why it happens:** /dev/cdrom is a device node; inotify behavior varies by kernel version and device type.
**How to avoid:** Use hybrid approach: QFileSystemWatcher as primary notification + QTimer-based ioctl polling as backup. The polling catches cases where the watcher misses events.
**Warning signs:** Disc insertion not detected until polling interval fires.

## Code Examples

### Disc Detection with ioctl Polling
```cpp
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <fcntl.h>

bool checkDiscPresent(const QString& devicePath)
{
    int fd = ::open(devicePath.toLocal8Bit().data(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return false;

    int status = ::ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
    ::close(fd);

    return (status == CDS_DISC_OK);
}

bool checkIsAudioDisc(const QString& devicePath)
{
    int fd = ::open(devicePath.toLocal8Bit().data(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return false;

    int discType = ::ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
    ::close(fd);

    return (discType == CDS_AUDIO || discType == CDS_MIXED);
}
```

### SQLite Cache Schema
```sql
CREATE TABLE IF NOT EXISTS disc_metadata (
    disc_id TEXT PRIMARY KEY,
    artist TEXT NOT NULL,
    album TEXT NOT NULL,
    genre TEXT,
    year INTEGER,
    source TEXT NOT NULL,  -- 'musicbrainz', 'gnudb', 'discogs'
    musicbrainz_release_id TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS track_metadata (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    disc_id TEXT NOT NULL,
    track_number INTEGER NOT NULL,
    title TEXT NOT NULL,
    artist TEXT,  -- per-track artist if different from album artist
    duration_seconds INTEGER NOT NULL,
    FOREIGN KEY (disc_id) REFERENCES disc_metadata(disc_id),
    UNIQUE(disc_id, track_number)
);

CREATE TABLE IF NOT EXISTS album_art (
    disc_id TEXT PRIMARY KEY,
    front_path TEXT,
    back_path TEXT,
    source TEXT NOT NULL,  -- 'coverartarchive', 'discogs'
    FOREIGN KEY (disc_id) REFERENCES disc_metadata(disc_id)
);
```

### GnuDB CDDB HTTP Request
```cpp
// GnuDB HTTP interface: http://gnudb.gnudb.org/~cddb/cddb.cgi
// Protocol: CDDB/1 + HTTP GET with query params

QString buildGnuDbQueryUrl(const QString& freedbDiscId, int trackCount,
                            const QVector<int>& offsets, int totalSeconds)
{
    // cddb query <discid> <ntrks> <off_1> <off_2> ... <nsecs>
    QString offsetStr;
    for (int off : offsets) {
        offsetStr += QString::number(off) + "+";
    }

    QString cmd = QString("cddb+query+%1+%2+%3%4")
        .arg(freedbDiscId)
        .arg(trackCount)
        .arg(offsetStr)
        .arg(totalSeconds);

    return QString("http://gnudb.gnudb.org/~cddb/cddb.cgi?cmd=%1&hello=user+hostname+MediaConsole+2.0&proto=6")
        .arg(cmd);
}
```

### CoverArtArchive Lookup
```cpp
// CoverArtArchive uses MusicBrainz release ID, not disc ID
// GET https://coverartarchive.org/release/{mbid}/front
// GET https://coverartarchive.org/release/{mbid}/back
// Returns: 307 redirect to actual image URL (Internet Archive)

void CdAlbumArtProvider::downloadArt(const QString& releaseId, const QString& discId)
{
    QUrl frontUrl(QString("https://coverartarchive.org/release/%1/front").arg(releaseId));
    auto* reply = m_nam->get(QNetworkRequest(frontUrl));
    connect(reply, &QNetworkReply::finished, this, [this, reply, discId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QString path = saveToDisk(discId, "front", data);
            emit frontArtReady(path);
        } else {
            // Try Discogs as fallback
            downloadFromDiscogs(discId);
        }
    });
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| cdparanoia standalone | libcdio-paranoia (integrated) | libcdio 2.0+ | Single library for both TOC + paranoia; cdio_ prefix on all functions |
| freedb.org CDDB | gnudb.org CDDB | 2020 (freedb shutdown) | Same protocol, different server; gnudb.org:80 or port 8880 |
| libmusicbrainz C++ | MusicBrainz JSON API over HTTP | Ongoing | HTTP+JSON simpler than C++ library; JSON API well-documented |
| libcoverart C | CoverArtArchive REST API | Ongoing | Direct HTTP simpler; no rate limits on coverartarchive.org |

**Deprecated/outdated:**
- freedb.org: Shut down 2020, redirects to gnudb.org. Use gnudb.gnudb.org as server.
- cdparanoia (standalone): Use libcdio-paranoia instead. Different function names (cdio_ prefix).
- libmusicbrainz5: Heavy C++ library. HTTP+JSON via QNetworkAccessManager is simpler and sufficient for disc lookup.
- libcddb: Old CDDB client library targeting freedb. Can use directly with gnudb, but raw HTTP is simpler for this use case.

## Open Questions

1. **GnuDB sustainability**
   - What we know: gnudb.org is community-funded and struggling with costs; may eventually limit access
   - What's unclear: Long-term availability; whether they'll require authentication
   - Recommendation: Already flagged in STATE.md as a concern. Discogs fallback exists. Monitor during deployment.

2. **Discogs authentication for album art**
   - What we know: Discogs requires authentication (API key/secret) for image URLs
   - What's unclear: Whether a free Discogs API key is sufficient for this volume of requests
   - Recommendation: Register a Discogs API key. Add to AppConfig::CdConfig. Only used as fallback when CoverArtArchive has no art. Volume will be very low.

3. **ioctl blocking behavior on drive busy**
   - What we know: CDROM_DRIVE_STATUS can block briefly when drive is spinning up
   - What's unclear: Maximum block duration on Pi 5 with USB CD drive
   - Recommendation: Run disc polling on a dedicated thread or via QtConcurrent::run to avoid blocking main thread. Short timeout (2-3s) on the ioctl call.

## Sources

### Primary (HIGH confidence)
- libcdio-paranoia API docs: https://www.gnu.org/software/libcdio/doxygen/libcdio-paranoia/paranoia_8h.html
- libcdio main page: https://www.gnu.org/software/libcdio/
- libdiscid 0.6.5: https://github.com/metabrainz/libdiscid
- MusicBrainz API docs: https://musicbrainz.org/doc/MusicBrainz_API
- CoverArtArchive API: https://musicbrainz.org/doc/Cover_Art_Archive/API
- Linux kernel CDROM ioctl docs: https://docs.kernel.org/userspace-api/ioctl/cdrom.html
- Qt6 QSqlDatabase: https://doc.qt.io/qt-6/qsqldatabase.html

### Secondary (MEDIUM confidence)
- gnudb.org protocol docs: https://gnudb.org/howtognudb.php
- Discogs API: https://www.discogs.com/developers
- Qt async database patterns: https://lnj.gitlab.io/post/async-databases-with-qtsql/

### Tertiary (LOW confidence)
- GnuDB long-term sustainability (community speculation)
- Exact ioctl blocking duration on Pi 5 (untested hardware)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - libcdio/paranoia/libdiscid are well-established, versions verified
- Architecture: HIGH - follows established project patterns (interface + impl, signal/slot async, PlatformFactory)
- Pitfalls: HIGH - CD audio and metadata pitfalls well-documented in community
- Network APIs: MEDIUM - MusicBrainz and CoverArtArchive well-documented; GnuDB less so

**Research date:** 2026-02-28
**Valid until:** 2026-03-28 (stable domain, slow-moving libraries)
