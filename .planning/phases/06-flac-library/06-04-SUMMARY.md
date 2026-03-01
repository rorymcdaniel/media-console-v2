---
phase: 06-flac-library
plan: 04
subsystem: app
tags: [appbuilder, appcontext, wiring, composition-root, flac-library]

requires:
  - phase: 06-03
    provides: FlacLibraryController, all three browse models
  - phase: 04-audio-pipeline
    provides: LocalPlaybackController
  - phase: 01-foundation
    provides: AppBuilder, AppContext composition root pattern
provides:
  - AppContext.flacLibraryController (non-owning pointer)
  - AppContext.libraryArtistModel, libraryAlbumModel, libraryTrackModel (non-owning pointers)
  - FlacLibraryController auto-started scanning in AppBuilder::build()
affects: [01-foundation, 06-flac-library]

tech-stack:
  added: []
  patterns: [composition root wiring, auto-start scanning after build, HAS_SNDFILE conditional in AppBuilder]

key-files:
  created: []
  modified:
    - src/app/AppBuilder.h
    - src/app/AppBuilder.cpp
    - src/app/AppContext.h
    - tests/test_AppBuilder.cpp

key-decisions:
  - "FlacLibraryController created inside HAS_SNDFILE conditional in AppBuilder — stub path on macOS"
  - "FlacLibraryController::start() called in AppBuilder::build() after construction — auto-starts scan"
  - "AppContext exposes non-owning flacLibraryController pointer and three model pointers for QML binding (Phase 10)"

patterns-established:
  - "HAS_SNDFILE guard in AppBuilder: same pattern as HAS_CDIO for conditional subsystem construction"
  - "Auto-start in build(): subsystem controllers that need initialization call start() before returning AppContext"

requirements-completed: [FLAC-01, FLAC-02, FLAC-03, FLAC-04, FLAC-05, FLAC-06, FLAC-07, FLAC-08]

completed: 2026-02-28
---

# Phase 06 Plan 04: AppBuilder Wiring Summary

**Wire FlacLibraryController into AppBuilder composition root and AppContext, completing Phase 6 FLAC Library subsystem integration**

## Accomplishments
- AppBuilder creates FlacLibraryController with LibraryConfig, LocalPlaybackController, PlaybackState dependencies
- FlacLibraryController::start() called in AppBuilder::build() to auto-start library scan
- AppContext extended with flacLibraryController, libraryArtistModel, libraryAlbumModel, libraryTrackModel (non-owning pointers)
- HAS_SNDFILE conditional in AppBuilder: real FlacLibraryController on Linux, nullptr on macOS/stub path
- test_AppBuilder: ContextHasFlacLibraryControllerOnLinux verifies non-null controller on Linux build
- Existing AppBuilder tests continue to pass (stub path on macOS)
- Phase 6 FLAC Library subsystem fully integrated and operational

## Test Coverage
- test_AppBuilder: 11 tests (12 after Phase 6 addition on Linux — ContextHasFlacLibraryControllerOnLinux)
- Project total: 265 tests, all pass

## Files Created/Modified
- `src/app/AppBuilder.h` - FlacLibraryController creation with HAS_SNDFILE conditional
- `src/app/AppBuilder.cpp` - Wiring FlacLibraryController to LocalPlaybackController, PlaybackState, LibraryConfig; start() call
- `src/app/AppContext.h` - Added flacLibraryController, libraryArtistModel, libraryAlbumModel, libraryTrackModel members
- `tests/test_AppBuilder.cpp` - Added ContextHasFlacLibraryControllerOnLinux test

---

*Phase: 06-flac-library*
*Completed: 2026-02-28*
