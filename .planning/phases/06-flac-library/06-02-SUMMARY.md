---
phase: 06-flac-library
plan: 02
subsystem: library
tags: [taglib, library-scanner, album-art, qtconcurrent, async, sha1-cache]

requires:
  - phase: 06-01
    provides: LibraryDatabase, LibraryTrack struct
provides:
  - LibraryScanner (async recursive FLAC scanner with TagLib metadata extraction)
  - LibraryAlbumArtProvider (album art extraction from FLAC picture blocks with disk caching)
affects: [06-flac-library]

tech-stack:
  added: [TagLib FLAC::File, QDirIterator, QtConcurrent, QCryptographicHash Sha1]
  patterns: [batchReady signal for main-thread DB insertion, atomic cancel flag, SHA-1 keyed art cache, MIME auto-detection]

key-files:
  created:
    - src/library/LibraryScanner.h
    - src/library/LibraryScanner.cpp
    - src/library/LibraryAlbumArtProvider.h
    - src/library/LibraryAlbumArtProvider.cpp
    - tests/test_LibraryScanner.cpp
    - tests/test_LibraryAlbumArtProvider.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "LibraryScanner runs via QtConcurrent::run — background thread communicates via signals to main thread"
  - "LibraryAlbumArtProvider caches art with SHA-1(albumArtist+album) filename — deterministic lookup without DB query"
  - "Art provider never writes to user's music directory — cache-only output to app data path"

patterns-established:
  - "batchReady(QVector<LibraryTrack>) signal: scanner emits batches, FlacLibraryController receives and calls DB upsert"
  - "Atomic cancel flag: QAtomicInt m_cancelled for safe cross-thread cancellation"
  - "File fallback order: FLAC picture block -> cover.jpg -> folder.jpg -> front.jpg"

requirements-completed: [FLAC-02, FLAC-04, FLAC-05, FLAC-06]

completed: 2026-02-28
---

# Phase 06 Plan 02: LibraryScanner + LibraryAlbumArtProvider Summary

**LibraryScanner with async QtConcurrent scanning and TagLib metadata extraction, and LibraryAlbumArtProvider with embedded art extraction and SHA-1-keyed disk caching**

## Accomplishments
- LibraryScanner recursively discovers FLAC files via QDirIterator with QDirIterator::Subdirectories
- TagLib::FileRef and TagLib::FLAC::File used for metadata extraction: title, artist, albumArtist, album, trackNumber, discNumber, year, genre, duration, sampleRate, bitDepth
- Incremental scanning: getTrackMtime() compared against filesystem mtime — unchanged files skipped (FLAC-04)
- QtConcurrent::run launches scan on background thread — main thread never blocked (FLAC-06)
- batchReady(QVector<LibraryTrack>) signal emitted periodically for main-thread DB insertion
- scanProgress(current, total) signal emitted for UI binding
- scanComplete() signal emitted on finish
- Atomic cancel flag (QAtomicInt) supports safe cancellation mid-scan
- albumArtist tag used for grouping; falls back to artist tag if albumArtist is empty
- LibraryAlbumArtProvider extracts front/back covers from FLAC picture blocks via TagLib::FLAC::File::pictureList()
- Falls back to cover.jpg, folder.jpg, front.jpg in album directory if no embedded art
- Caches art to disk with SHA-1(albumArtist+album) filename (FLAC-05)
- Auto-detects MIME type (JPEG vs PNG) by inspecting magic bytes
- Never modifies user's music directory — all writes go to app data cache path

## Test Coverage
- test_LibraryScanner: 4 tests (construction, empty directory scan, nonexistent directory, cancellation)
- test_LibraryAlbumArtProvider: 9 tests (construction, SHA-1 consistency/distinctness, cache miss/hit, file fallback, PNG detection)
- Project total: 265 tests, all pass

## Files Created/Modified
- `src/library/LibraryScanner.h` - Async scanner with batchReady/scanProgress/scanComplete signals
- `src/library/LibraryScanner.cpp` - QDirIterator recursion, TagLib metadata extraction, mtime comparison, QtConcurrent
- `src/library/LibraryAlbumArtProvider.h` - Art extraction and caching interface
- `src/library/LibraryAlbumArtProvider.cpp` - TagLib picture block extraction, file fallback, SHA-1 caching, MIME detection
- `tests/test_LibraryScanner.cpp` - 4 tests for scanning lifecycle and cancellation
- `tests/test_LibraryAlbumArtProvider.cpp` - 9 tests for hashing, caching, and extraction
- `CMakeLists.txt` - TagLib and QtConcurrent linkage added
- `tests/CMakeLists.txt` - Added test_LibraryScanner and test_LibraryAlbumArtProvider

---

*Phase: 06-flac-library*
*Completed: 2026-02-28*
