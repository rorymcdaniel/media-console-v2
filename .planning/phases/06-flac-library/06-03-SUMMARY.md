---
phase: 06-flac-library
plan: 03
subsystem: library
tags: [qabstractlistmodel, browse-models, flac-library-controller, playlist, playback]

requires:
  - phase: 06-01
    provides: LibraryDatabase, LibraryTrack struct
  - phase: 06-02
    provides: LibraryScanner, LibraryAlbumArtProvider
  - phase: 04-audio-pipeline
    provides: LocalPlaybackController, FlacAudioStream
provides:
  - LibraryArtistModel (QAbstractListModel for artist browse)
  - LibraryAlbumModel (QAbstractListModel for album browse filtered by artist)
  - LibraryTrackModel (QAbstractListModel for track browse filtered by artist+album)
  - FlacLibraryController (orchestrator: scanner, database, art provider, models, playback)
affects: [06-flac-library, 10-ui]

tech-stack:
  added: []
  patterns: [QAbstractListModel with filter Q_PROPERTYs, playlist-based next/previous navigation, multi-disc track ordering]

key-files:
  created:
    - src/library/LibraryArtistModel.h
    - src/library/LibraryArtistModel.cpp
    - src/library/LibraryAlbumModel.h
    - src/library/LibraryAlbumModel.cpp
    - src/library/LibraryTrackModel.h
    - src/library/LibraryTrackModel.cpp
    - src/library/FlacLibraryController.h
    - src/library/FlacLibraryController.cpp
    - tests/test_LibraryModels.cpp
    - tests/test_FlacLibraryController.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Single test file (test_LibraryModels.cpp) covers all three model classes — they share test fixture and database"
  - "FlacLibraryController::playTrack(index) builds album playlist from LibraryTrackModel — disc-aware ordering for next/previous"
  - "Album finishes with no auto-continue — stop playback cleanly, no looping"

patterns-established:
  - "Filter Q_PROPERTY pattern: setArtistFilter() triggers DB re-query and model reset"
  - "Playlist slice: playTrack(index) stores QVector<LibraryTrack> sorted by disc+track for navigation"
  - "User-initiated only: playTrack() is Q_INVOKABLE, no auto-play on scan complete"

requirements-completed: [FLAC-07, FLAC-08]

completed: 2026-02-28
---

# Phase 06 Plan 03: Browse Models + FlacLibraryController Summary

**Three QAbstractListModel browse models for artist/album/track hierarchy, and FlacLibraryController orchestrating scanner, database, art provider, models, and LocalPlaybackController integration**

## Accomplishments
- LibraryArtistModel: QAbstractListModel with albumArtist and albumCount roles; refresh() queries getArtists()
- LibraryAlbumModel: Q_PROPERTY artistFilter triggers re-query on change; exposes album, year, trackCount, artPath roles
- LibraryTrackModel: Q_PROPERTY artistFilter and albumFilter; exposes trackNumber, discNumber, title, artist, durationSeconds, filePath roles; sorts by discNumber then trackNumber for multi-disc albums
- trackAt(index) returns LibraryTrack for FlacLibraryController playlist construction
- allTracks() returns full filtered list for playlist building
- FlacLibraryController owns database, scanner, art provider, all three models
- start() opens database and launches LibraryScanner
- Scanner batchReady connected to DB upsert and model refresh
- playTrack(index) creates FlacAudioStream from LibraryTrackModel, builds album playlist for navigation
- next() and previous() navigate within current album playlist (disc-aware ordering)
- PlaybackState updated with track metadata (title, artist, album, albumArtUrl, position/duration)
- Album end triggers stop — no looping, no auto-continue to next album
- Models browsable during active scan (show current DB contents)

## Test Coverage
- test_LibraryModels: 13 tests (artist/album/track model row counts, data values, role names, filtering, multi-disc sort, trackAt, allTracks)
- test_FlacLibraryController: 6 tests (construction, scanning state, database open, null playback controller safety, empty playlist navigation, stop clears state)
- Project total: 265 tests, all pass

## Files Created/Modified
- `src/library/LibraryArtistModel.h` - QAbstractListModel for artist browse
- `src/library/LibraryArtistModel.cpp` - getArtists() query, albumArtist/albumCount roles
- `src/library/LibraryAlbumModel.h` - QAbstractListModel with artistFilter Q_PROPERTY
- `src/library/LibraryAlbumModel.cpp` - getAlbumsByArtist() query, album/year/trackCount/artPath roles
- `src/library/LibraryTrackModel.h` - QAbstractListModel with artistFilter+albumFilter Q_PROPERTYs
- `src/library/LibraryTrackModel.cpp` - getTracksByAlbum() query, disc+track sort, trackAt()/allTracks()
- `src/library/FlacLibraryController.h` - Orchestrator header with Q_INVOKABLE playTrack/next/previous/stop
- `src/library/FlacLibraryController.cpp` - Scanner integration, playlist management, PlaybackState updates
- `tests/test_LibraryModels.cpp` - 13 tests across all three model classes
- `tests/test_FlacLibraryController.cpp` - 6 tests for lifecycle and playback guard
- `CMakeLists.txt` - Updated library source list for new files
- `tests/CMakeLists.txt` - Added test_LibraryModels and test_FlacLibraryController

---

*Phase: 06-flac-library*
*Completed: 2026-02-28*
