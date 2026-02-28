---
phase: 05-cd-subsystem
plan: 02
subsystem: cd
tags: [sqlite, metadata, musicbrainz, gnudb, discogs, coverartarchive, album-art]

requires:
  - phase: 01-foundation
    provides: ICdDrive interface, TocEntry struct
  - phase: 05-01
    provides: CdAudioStream, LibcdioCdDrive
provides:
  - CdMetadataCache (SQLite-backed disc/track/art caching)
  - CdMetadataProvider (async three-tier metadata lookup)
  - CdAlbumArtProvider (cover art download with fallback)
affects: [05-cd-subsystem, 09-orchestration]

tech-stack:
  added: [Qt6::Sql, SQLite, MusicBrainz API, GnuDB CDDB protocol, Discogs API, CoverArtArchive]
  patterns: [async signal chain, three-tier fallback, freedb disc ID computation, DTITLE parsing]

key-files:
  created:
    - src/cd/CdMetadataCache.h
    - src/cd/CdMetadataCache.cpp
    - src/cd/CdMetadataProvider.h
    - src/cd/CdMetadataProvider.cpp
    - src/cd/CdAlbumArtProvider.h
    - src/cd/CdAlbumArtProvider.cpp
    - tests/test_CdMetadataCache.cpp
    - tests/test_CdMetadataProvider.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "CdMetadataCache uses unique QUuid connection name to avoid QSqlDatabase default connection conflicts"
  - "CdMetadataProvider uses UTF-8 with Latin-1 fallback for GnuDB responses (replacement char detection)"
  - "GnuDB DTITLE parsing uses ' / ' (with spaces) as separator to handle band names like AC/DC"
  - "All metadata/art classes in unconditional LIB_SOURCES since they have no platform-specific dependencies"

patterns-established:
  - "Three-tier async lookup: MusicBrainz -> GnuDB -> Discogs with signal-based chaining"
  - "CoverArtArchive front + optional back cover with Discogs fallback"
  - "Freedb disc ID computation from TOC for GnuDB compatibility"

requirements-completed: [CD-05, CD-06, CD-07, CD-08, CD-09, CD-10]

duration: 8min
completed: 2026-02-28
---

# Phase 05 Plan 02: Metadata Subsystem Summary

**CdMetadataCache for SQLite persistence, CdMetadataProvider for async three-tier metadata lookup, CdAlbumArtProvider for cover art downloads**

## Performance

- **Duration:** 8 min
- **Started:** 2026-02-28T15:16:07Z
- **Completed:** 2026-02-28T15:24:00Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- CdMetadataCache provides SQLite-backed storage for disc metadata, track info, and album art paths with upsert semantics
- CdMetadataProvider implements async three-tier fallback: MusicBrainz (JSON) -> GnuDB (CDDB protocol) -> Discogs (JSON)
- CdAlbumArtProvider downloads album art from CoverArtArchive with Discogs fallback, saves to disk with format detection
- Freedb disc ID computation from TOC for GnuDB compatibility
- GnuDB track count validation against TOC (CD-09)
- All network I/O via QNetworkAccessManager (non-blocking, no threads)

## Task Commits

Each task was committed atomically:

1. **Task 1: CdMetadataCache with SQLite storage** - `e4427eb` (feat)
2. **Task 2: CdMetadataProvider + CdAlbumArtProvider** - `b927c0c` (feat)

## Files Created/Modified
- `src/cd/CdMetadataCache.h` - CdTrackInfo, CdMetadata, CdAlbumArtInfo structs + SQLite cache class
- `src/cd/CdMetadataCache.cpp` - 3-table schema, prepared statements, transactions, unique connection name
- `src/cd/CdMetadataProvider.h` - Async metadata lookup with static helpers
- `src/cd/CdMetadataProvider.cpp` - MusicBrainz JSON, GnuDB CDDB, Discogs search, timeout handling
- `src/cd/CdAlbumArtProvider.h` - Cover art download with fallback chain
- `src/cd/CdAlbumArtProvider.cpp` - CoverArtArchive front+back, redirect following, format detection
- `tests/test_CdMetadataCache.cpp` - 8 tests for SQLite round-trip, upsert, album art, multi-disc
- `tests/test_CdMetadataProvider.cpp` - 10 tests for DTITLE parsing, freedb disc ID computation
- `CMakeLists.txt` - Added Qt6::Sql to dependencies
- `tests/CMakeLists.txt` - Added test_CdMetadataCache, test_CdMetadataProvider

## Decisions Made
- CdMetadataCache uses QUuid-based unique connection names to avoid SQLite default connection conflicts in tests
- GnuDB response parsing uses UTF-8 first with Latin-1 fallback (detecting QChar(0xFFFD) replacement character)
- DTITLE parsing uses " / " (space-slash-space) as separator, not bare "/", to handle band names like "AC/DC"

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] QTextCodec removed in Qt6**
- **Found during:** Task 2 compile
- **Issue:** CdMetadataProvider.cpp included `<QTextCodec>` which was removed in Qt6
- **Fix:** Removed unused include; UTF-8/Latin-1 fallback uses QString::fromUtf8/fromLatin1 directly
- **Files modified:** src/cd/CdMetadataProvider.cpp
- **Verification:** Build and all 243 tests pass
- **Committed in:** b927c0c (part of task commit)

**2. [Rule 3 - Blocking] QSqlDatabase requires QCoreApplication**
- **Found during:** Task 1 test execution (segfault)
- **Issue:** CdMetadataCache tests crashed because QSqlDatabase needs QCoreApplication
- **Fix:** Added QCoreApplication creation in test SetUp() following established pattern
- **Files modified:** tests/test_CdMetadataCache.cpp
- **Verification:** Build and all 233 tests pass after Task 1
- **Committed in:** e4427eb (part of task commit)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** No scope creep. Standard Qt6 migration issues resolved during implementation.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Metadata subsystem complete, ready for Plan 05-03 (CdController orchestrator)
- CdMetadataCache, CdMetadataProvider, CdAlbumArtProvider available as dependencies
- All 243 tests pass

---
*Phase: 05-cd-subsystem*
*Completed: 2026-02-28*
