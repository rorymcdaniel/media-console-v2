---
phase: 09-display-http-api-and-orchestration
plan: 03
subsystem: orchestration
tags: [playback-router, album-art, source-dispatch, qml-property, composition-root]

# Dependency graph
requires:
  - phase: 09-display-http-api-and-orchestration
    provides: ScreenTimeoutController (plan 01), HttpApiServer (plan 02)
  - phase: 08-spotify-integration
    provides: SpotifyController for playback dispatch
  - phase: 05-cd-subsystem
    provides: CdController for CD playback dispatch
  - phase: 06-flac-library
    provides: FlacLibraryController for library playback dispatch
provides:
  - PlaybackRouter for source-aware playback command dispatch
  - AlbumArtResolver for unified album art Q_PROPERTY
  - AppBuilder wiring of all Phase 9 objects (ScreenTimeoutController, HttpApiServer, PlaybackRouter, AlbumArtResolver)
  - AppContext with 4 new non-owning pointers
affects: [10-qml-ui]

# Tech tracking
tech-stack:
  added: []
  patterns: [source-dispatch-switch, signal-driven-resolution, composition-root-wiring]

key-files:
  created:
    - src/orchestration/PlaybackRouter.h
    - src/orchestration/PlaybackRouter.cpp
    - src/orchestration/AlbumArtResolver.h
    - src/orchestration/AlbumArtResolver.cpp
    - tests/test_PlaybackRouter.cpp
    - tests/test_AlbumArtResolver.cpp
  modified:
    - src/app/AppBuilder.h
    - src/app/AppBuilder.cpp
    - src/app/AppContext.h
    - tests/CMakeLists.txt
    - CMakeLists.txt

key-decisions:
  - "HAS_SNDFILE guard for FlacLibraryController method calls in PlaybackRouter (macOS portability)"
  - "CD/Library play/pause are no-op in PlaybackRouter (user-initiated via track selection, not generic play button)"
  - "AlbumArtResolver initial resolve() in constructor for correct state on creation"
  - "GPIO activity signals (volumeChanged, inputNext, inputPrevious, inputSelect) all wired to ScreenTimeoutController"

patterns-established:
  - "Source-dispatch switch pattern: switch on activeSource with null-checked controller calls"
  - "Signal-driven property resolution: connect to multiple state change signals, resolve on any change"
  - "Composition root Phase 9 block: ScreenTimeoutController -> PlaybackRouter -> AlbumArtResolver -> HttpApiServer"

requirements-completed: [ORCH-01, ORCH-02]

# Metrics
duration: 8min
completed: 2026-02-28
---

# Phase 9 Plan 03: Orchestration and AppBuilder Wiring Summary

**PlaybackRouter source-aware command dispatch, AlbumArtResolver unified Q_PROPERTY, and AppBuilder composition root wiring for all Phase 9 objects**

## Performance

- **Duration:** 8 min
- **Started:** 2026-02-28T20:38:59Z
- **Completed:** 2026-02-28T20:46:41Z
- **Tasks:** 2
- **Files modified:** 11

## Accomplishments
- PlaybackRouter dispatches play/pause/stop/next/previous/seek to correct controller (Spotify, CD, Library) based on activeSource
- AlbumArtResolver provides single Q_PROPERTY for QML binding, resolving art from receiver CGI (streaming/BT) or PlaybackState (CD/Library)
- AppBuilder constructs and wires ScreenTimeoutController, HttpApiServer, PlaybackRouter, AlbumArtResolver with all signal/slot connections
- 21 new tests (11 PlaybackRouter + 10 AlbumArtResolver), all 364 tests pass with zero regressions

## Task Commits

Each task was committed atomically (TDD: test then feat):

1. **Task 1: PlaybackRouter and AlbumArtResolver (TDD)**
   - `5fcabc1` (test) - Failing tests for PlaybackRouter and AlbumArtResolver
   - `8e638eb` (feat) - Full implementations with source dispatch and art resolution

2. **Task 2: Wire Phase 9 objects in AppBuilder/AppContext**
   - `b783be2` (feat) - AppBuilder construction, signal wiring, service startup, AppContext pointers

## Files Created/Modified
- `src/orchestration/PlaybackRouter.h` - Source-aware playback command dispatch header
- `src/orchestration/PlaybackRouter.cpp` - Switch-based dispatch to Spotify/CD/Library controllers, source change auto-stop
- `src/orchestration/AlbumArtResolver.h` - Unified album art Q_PROPERTY header
- `src/orchestration/AlbumArtResolver.cpp` - Signal-driven resolution: receiver CGI for streaming/BT, PlaybackState for CD/Library
- `tests/test_PlaybackRouter.cpp` - 11 tests: null safety, no-op sources, source change, rapid switching
- `tests/test_AlbumArtResolver.cpp` - 10 tests: per-source art resolution, signal emission, reactive updates
- `src/app/AppBuilder.h` - Added unique_ptr members for Phase 9 objects
- `src/app/AppBuilder.cpp` - Phase 9 construction block: ScreenTimeoutController, PlaybackRouter, AlbumArtResolver, HttpApiServer
- `src/app/AppContext.h` - 4 new non-owning pointers (screenTimeoutController, httpApiServer, playbackRouter, albumArtResolver)
- `CMakeLists.txt` - Added orchestration source files to LIB_SOURCES
- `tests/CMakeLists.txt` - Added test files to TEST_SOURCES

## Decisions Made
- Used HAS_SNDFILE compile guard for FlacLibraryController method calls in PlaybackRouter since FlacLibraryController is not compiled on macOS (only Linux with libsndfile)
- CD and Library play/pause are no-ops in PlaybackRouter because those sources use user-initiated track selection (CdController::playTrack, FlacLibraryController::playTrack) rather than generic play/pause buttons
- AlbumArtResolver calls resolve() in constructor to establish correct initial state before any signals fire
- All four GPIO activity signals wired to ScreenTimeoutController::activityDetected for screen wake on any hardware input

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] FlacLibraryController linker errors on macOS**
- **Found during:** Task 1 (PlaybackRouter implementation)
- **Issue:** FlacLibraryController.cpp not compiled on macOS (no libsndfile), causing linker errors for method calls even with runtime null-checks
- **Fix:** Wrapped all FlacLibraryController method calls with `#ifdef HAS_SNDFILE` preprocessor guards
- **Files modified:** src/orchestration/PlaybackRouter.cpp
- **Verification:** Build succeeds on macOS, all tests pass
- **Committed in:** 8e638eb (Task 1 feat commit)

**2. [Rule 3 - Blocking] CdController missing play/pause/next/previous methods**
- **Found during:** Task 1 (PlaybackRouter implementation)
- **Issue:** Plan interfaces section specified CdController::play(), pause(), next(), previous() but actual CdController only has stop(), playTrack(int), start(), eject()
- **Fix:** Made CD play/pause/next/previous no-ops with log messages (CD playback is user-initiated via track selection)
- **Files modified:** src/orchestration/PlaybackRouter.cpp
- **Verification:** All tests pass, no crashes
- **Committed in:** 8e638eb (Task 1 feat commit)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both auto-fixes necessary for compilation and correct behavior. No scope creep.

## Issues Encountered
None beyond the deviations documented above.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 9 complete: all 3 plans executed, all display/API/orchestration components wired
- PlaybackRouter ready for QML binding in Phase 10 (Q_INVOKABLE methods)
- AlbumArtResolver Q_PROPERTY available for QML album art display
- AppContext exposes all Phase 9 objects for QML engine registration
- All 364 tests pass with zero regressions

## Self-Check: PASSED

All files verified -- see verification below.

---
*Phase: 09-display-http-api-and-orchestration*
*Completed: 2026-02-28*
