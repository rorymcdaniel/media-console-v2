---
phase: 08-spotify-integration
plan: 03
subsystem: controller
tags: [spotify, controller, search-debounce, device-transfer, playback-polling, session-takeover, qtimer]

# Dependency graph
requires:
  - phase: 08-01
    provides: SpotifyAuth with isAuthenticated(), accessToken(), authStateChanged signal
  - phase: 08-02
    provides: SpotifyClient REST API wrapper with all endpoint methods and typed signals
provides:
  - SpotifyController business logic orchestrator for all Spotify functionality
  - Search with 300ms debounce via QTimer
  - Device discovery and transfer by desiredDeviceName
  - Active session detection with takeover confirm/cancel flow
  - Playback state polling at 3s when Spotify is active source
  - PlaybackState metadata updates from Spotify API JSON
  - AppBuilder construction of full Spotify stack (auth + client + controller)
  - AppContext.spotifyController pointer for QML binding (Phase 10)
affects: [09, 10]

# Tech tracking
tech-stack:
  added: []
  patterns: [Controller orchestrator with QTimer debounce, signal-based session takeover flow, null-safe construction]

key-files:
  created:
    - src/spotify/SpotifyController.h
    - src/spotify/SpotifyController.cpp
    - tests/test_SpotifyController.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt
    - src/app/AppBuilder.h
    - src/app/AppBuilder.cpp
    - src/app/AppContext.h

key-decisions:
  - "Null-safe construction: all pointer dereferences guarded for defensive coding"
  - "Album art for active playback deferred to receiver CGI endpoint (SPOT-08), not Spotify API URLs"
  - "Session takeover uses findAndTransferToDevice (getDevices then match by name) rather than storing pending device IDs"

patterns-established:
  - "SpotifyController as orchestrator: owns debounce timers, polls only when active, delegates all API calls to SpotifyClient"
  - "Active session detection: getCurrentPlayback on activate, compare device name, emit signal for UI dialog"

requirements-completed: [SPOT-02, SPOT-03, SPOT-04, SPOT-05, SPOT-06, SPOT-07, SPOT-08]

# Metrics
duration: 5min
completed: 2026-02-28
---

# Phase 8 Plan 3: Spotify Controller Summary

**SpotifyController orchestrating search debounce, device transfer by name, session takeover detection, and 3s playback polling with AppBuilder integration**

## Performance

- **Duration:** 5 min
- **Started:** 2026-02-28T19:55:13Z
- **Completed:** 2026-02-28T20:00:32Z
- **Tasks:** 2 (Task 1 TDD: RED + GREEN)
- **Files modified:** 8

## Accomplishments
- SpotifyController class with 300ms search debounce, device discovery by desiredDeviceName, session takeover confirm/cancel, and 3s playback polling
- Full signal wiring between SpotifyAuth, SpotifyClient, PlaybackState, and UIState with null-safe guards
- AppBuilder constructs complete Spotify stack (auth + client + controller) with token restore on startup
- AppContext exposes spotifyController pointer for QML binding in Phase 10

## Task Commits

Each task was committed atomically:

1. **Task 1 (RED): SpotifyController failing tests** - `d78d361` (test)
2. **Task 1 (GREEN): SpotifyController full implementation** - `df25d0a` (feat)
3. **Task 2: AppBuilder and AppContext wiring** - `ad2e178` (feat)

_TDD task had RED/GREEN commits as per TDD protocol._

## Files Created/Modified
- `src/spotify/SpotifyController.h` - Business logic orchestrator class with Q_PROPERTY bindings for QML
- `src/spotify/SpotifyController.cpp` - Full implementation: search debounce, device transfer, session takeover, playback polling, error handling
- `tests/test_SpotifyController.cpp` - 5 tests: debounce, isSpotifyAvailable, isSpotifyActive, null safety, clearSearch
- `CMakeLists.txt` - Added SpotifyController.h/cpp to LIB_SOURCES
- `tests/CMakeLists.txt` - Added test_SpotifyController.cpp to TEST_SOURCES
- `src/app/AppBuilder.h` - Added SpotifyAuth, SpotifyClient, SpotifyController unique_ptrs
- `src/app/AppBuilder.cpp` - Spotify stack construction, token restore, context wiring
- `src/app/AppContext.h` - Added SpotifyController* spotifyController pointer

## Decisions Made
- All pointer dereferences guarded with null checks for defensive coding (constructor accepts nullptr for all dependencies)
- Album art for active playback intentionally not set by SpotifyController -- the receiver's CGI endpoint provides album art via NJA2 parsing in ReceiverController (SPOT-08)
- Session takeover uses findAndTransferToDevice (getDevices -> match by name -> transferPlayback) rather than caching device IDs from the active session check, since device IDs may change between checks

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required. Spotify client_id must already be set in QSettings INI file (configured in Plan 08-01).

## Next Phase Readiness
- SpotifyController provides complete API for QML UI integration (Phase 10)
- Phase 9 PlaybackRouter/ORCH-01 can coordinate between receiver-detected Spotify and SpotifyController-managed Spotify
- All 297 tests pass with no regressions

## Self-Check: PASSED

- FOUND: src/spotify/SpotifyController.h
- FOUND: src/spotify/SpotifyController.cpp
- FOUND: tests/test_SpotifyController.cpp
- FOUND: src/app/AppBuilder.h (modified)
- FOUND: src/app/AppBuilder.cpp (modified)
- FOUND: src/app/AppContext.h (modified)
- FOUND: commit d78d361 (RED)
- FOUND: commit df25d0a (GREEN)
- FOUND: commit ad2e178 (Task 2)

---
*Phase: 08-spotify-integration*
*Completed: 2026-02-28*
