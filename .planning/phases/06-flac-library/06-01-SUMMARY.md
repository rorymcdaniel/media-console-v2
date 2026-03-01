---
phase: 06-flac-library
plan: 01
subsystem: library
tags: [libsndfile, libsamplerate, sqlite, flac, audio-stream, library-database]

requires:
  - phase: 04-audio-pipeline
    provides: AudioStream interface, LocalPlaybackController
  - phase: 01-foundation
    provides: AppConfig, AppBuilder, platform interfaces
provides:
  - FlacAudioStream (AudioStream implementation using libsndfile+libsamplerate)
  - LibraryDatabase (SQLite library index with full CRUD operations)
  - mediaLibrary logging category
affects: [06-flac-library]

tech-stack:
  added: [libsndfile, libsamplerate, TagLib, SQLite via Qt]
  patterns: [HAS_SNDFILE conditional compilation, resample-ratio-adjusted seek, LibraryTrack struct, upsertTrackBatch transaction]

key-files:
  created:
    - src/library/FlacAudioStream.h
    - src/library/FlacAudioStream.cpp
    - src/library/LibraryDatabase.h
    - src/library/LibraryDatabase.cpp
    - tests/test_FlacAudioStream.cpp
    - tests/test_LibraryDatabase.cpp
  modified:
    - src/utils/Logging.h
    - src/utils/Logging.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "FlacAudioStream always outputs 44100Hz/16-bit/stereo regardless of source format — libsamplerate handles resampling transparently"
  - "LibraryDatabase uses unique QSqlDatabase connection name per instance — same SQLite pattern as CdMetadataCache"
  - "FlacAudioStream in unconditional LIB_SOURCES with HAS_SNDFILE guards in .cpp — math/state logic testable on macOS"

patterns-established:
  - "HAS_SNDFILE conditional: same pattern as HAS_CDIO for cross-platform audio support"
  - "LibraryTrack struct: shared between scanner, database, and models"
  - "mtime-keyed upsert: getTrackMtime() enables incremental scanning without full re-scan"

requirements-completed: [FLAC-01, FLAC-03]

completed: 2026-02-28
---

# Phase 06 Plan 01: FlacAudioStream + LibraryDatabase Summary

**FlacAudioStream implementing AudioStream via libsndfile+libsamplerate with 44100Hz/stereo normalization, and LibraryDatabase with full SQLite CRUD for the library index**

## Accomplishments
- FlacAudioStream implements full AudioStream interface: open/close/readFrames/seek/totalFrames/positionFrames/sampleRate/channels/bitDepth
- Transparent resampling via libsamplerate to 44100Hz; mono files duplicated to stereo
- seek() converts output frame position accounting for resample ratio before calling sf_seek
- totalFrames() returns output frame count after resampling
- LibraryDatabase implements SQLite library index with tracks table (title, artist, albumArtist, album, trackNumber, discNumber, year, genre, durationSeconds, sampleRate, bitDepth, filePath, mtime)
- Indexes on albumArtist, album, filePath for fast browse queries
- upsertTrack()/upsertTrackBatch() insert-or-replace by filePath
- getTrackMtime() returns stored mtime for incremental scan comparison
- getArtists() returns distinct albumArtists sorted alphabetically with albumCount
- getAlbumsByArtist() returns albums sorted by year
- getTracksByAlbum() returns tracks sorted by disc number then track number
- removeStaleEntries() deletes tracks whose files no longer exist
- mediaLibrary logging category declared in Logging.h and defined in Logging.cpp

## Test Coverage
- test_FlacAudioStream: 8 tests (construction, format accessors, open failure, readFrames without open)
- test_LibraryDatabase: 11 tests (CRUD round-trip, mtime storage, sorted queries, stale removal, batch upsert)
- Project total: 265 tests, all pass

## Files Created/Modified
- `src/library/FlacAudioStream.h` - AudioStream implementation header using libsndfile+libsamplerate
- `src/library/FlacAudioStream.cpp` - Decoding, resampling, mono-to-stereo, seek with ratio adjustment
- `src/library/LibraryDatabase.h` - SQLite library index interface with LibraryTrack struct
- `src/library/LibraryDatabase.cpp` - Full CRUD: upsert, mtime lookup, artist/album/track queries, stale removal
- `tests/test_FlacAudioStream.cpp` - 8 tests for interface compliance and error paths
- `tests/test_LibraryDatabase.cpp` - 11 tests for CRUD operations and sorted queries
- `src/utils/Logging.h` - Added mediaLibrary Q_DECLARE_LOGGING_CATEGORY
- `src/utils/Logging.cpp` - Added Q_LOGGING_CATEGORY definition for mediaLibrary
- `CMakeLists.txt` - pkg_check_modules for libsndfile/libsamplerate/taglib, HAS_SNDFILE conditional
- `tests/CMakeLists.txt` - Added test_FlacAudioStream and test_LibraryDatabase

## Decisions Made
- FlacAudioStream placed in unconditional LIB_SOURCES so that interface compliance tests run on all platforms; HAS_SNDFILE guards in the .cpp protect libsndfile/libsamplerate calls
- LibraryDatabase uses a unique QSqlDatabase connection name (constructed from filePath) to avoid Qt multi-connection conflicts

---

*Phase: 06-flac-library*
*Completed: 2026-02-28*
