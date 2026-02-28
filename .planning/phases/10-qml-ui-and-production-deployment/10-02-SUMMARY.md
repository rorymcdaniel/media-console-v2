---
phase: 10-qml-ui-and-production-deployment
plan: 02
subsystem: ui
tags: [qml, qt6, carousel, now-playing, playback-controls, album-art, flipable, dynamic-accent]

# Dependency graph
requires:
  - phase: 10-qml-ui-and-production-deployment
    provides: "Theme.qml design tokens, main.qml layout skeleton, QML singleton registrations"
  - phase: 09-display-http-api-and-orchestration
    provides: "PlaybackRouter, AlbumArtResolver"
  - phase: 02-state-layer-and-qml-binding-surface
    provides: "ReceiverState, PlaybackState, UIState singletons"
provides:
  - "InputCarousel.qml: 3D perspective vertical wheel with 6 inputs, auto-select progress ring"
  - "NowPlaying.qml: Album art with Flipable flip, track info, progress bar with seek, dynamic accent"
  - "PlaybackControls.qml: Adaptive play/pause/next/previous per source type"
  - "Loader-based view switching in main.qml right panel"
affects: [10-03, 10-04]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "ListView with SnapToItem and StrictlyEnforceRange for centered carousel selection"
    - "Canvas-based progress ring with Timer for 4-second auto-select countdown"
    - "3D Rotation transform on ListView delegates for perspective effect"
    - "Flipable QML element with Rotation animation for album art flip"
    - "Canvas color extraction from album art for dynamic accent theming"
    - "Adaptive control visibility via property bindings on PlaybackState.activeSource"

key-files:
  created:
    - src/qml/components/InputCarousel.qml
    - src/qml/components/NowPlaying.qml
    - src/qml/components/PlaybackControls.qml
  modified:
    - src/qml/main.qml
    - CMakeLists.txt

key-decisions:
  - "ListModel for input sources instead of Repeater — enables ListView snap and scroll behavior for carousel"
  - "Canvas-based progress ring on per-delegate basis — ring renders only on the focused item"
  - "Flipable back shows album info text rather than back cover image — placeholder until back art data is available"
  - "Brightness-boosted dynamic accent — Canvas extracts average color with minimum luminance threshold to avoid dark accent"
  - "Loader-based view switching in right panel — components/NowPlaying.qml loaded by default, components/LibraryBrowser.qml and SpotifySearch.qml available via UIState.activeView"

patterns-established:
  - "InputCarousel.sourceToIndex/indexToSource for MediaSource enum <-> model index mapping"
  - "PlaybackControls.showPlayPause/showPrevNext adaptive visibility per active source"
  - "NowPlaying.formatTime(ms) helper for mm:ss time display"
  - "Canvas.loadImage + onImageLoaded + getImageData for album art color extraction"

requirements-completed: [UI-04, UI-05, UI-11, UI-12]

# Metrics
duration: 4min
completed: 2026-02-28
---

# Phase 10 Plan 02: InputCarousel and NowPlaying Summary

**3D perspective input carousel with 4-second auto-select ring, dominant album art NowPlaying with Flipable flip animation, adaptive playback controls, and dynamic accent color extraction**

## Performance

- **Duration:** 4 min
- **Started:** 2026-02-28T22:14:51Z
- **Completed:** 2026-02-28T22:19:36Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- InputCarousel with 6 inputs in vertical ListView, 3D Rotation transform for perspective, scale/opacity behavior based on distance from center, Canvas progress ring filling over 4 seconds for auto-select
- NowPlaying with 60/40 split layout: dominant Flipable album art (front/back 3D flip), track info with title/artist/album/track number, Slider progress bar with seek, and adaptive PlaybackControls
- Dynamic accent color extracted from album art via Canvas pixel averaging with minimum brightness boost
- Loader-based view switching in main.qml right panel, replacing placeholder text with component loading

## Task Commits

Each task was committed atomically:

1. **Task 1: InputCarousel with 3D perspective vertical wheel and auto-select** - `e47dcd8` (feat)
2. **Task 2: NowPlaying with album art, track info, seek bar, adaptive controls** - `355cbdc` (feat)

**Plan metadata:** (this commit) (docs: complete plan)

## Files Created/Modified
- `src/qml/components/InputCarousel.qml` - 3D perspective vertical carousel with 6 input sources, Canvas auto-select progress ring, snap-to-center ListView
- `src/qml/components/NowPlaying.qml` - Album art with Flipable flip animation, track info, progress bar with seek, dynamic accent color extraction via Canvas
- `src/qml/components/PlaybackControls.qml` - Adaptive play/pause/next/previous buttons, visibility driven by PlaybackState.activeSource
- `src/qml/main.qml` - InputCarousel in left panel, Loader-based view switching in right panel
- `CMakeLists.txt` - Added InputCarousel.qml, NowPlaying.qml, PlaybackControls.qml to QML_FILES

## Decisions Made
- Used ListModel + ListView with SnapToItem for carousel rather than Repeater, enabling smooth scroll and snap behavior
- Canvas-based progress ring renders on each delegate but only paints for the focused item, avoiding extra components
- Flipable back side shows album info text as placeholder since back cover art URL is not yet available from the data layer
- Dynamic accent color boosted to minimum luminance threshold to avoid very dark accent colors from dark album art
- Loader-based view switching loads components on demand, deferring LibraryBrowser and SpotifySearch loading until Plan 10-03

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- InputCarousel and NowPlaying are fully wired to C++ singletons (ReceiverState, PlaybackState, ReceiverController, PlaybackRouter, AlbumArtResolver)
- Loader in right panel ready for LibraryBrowser.qml and SpotifySearch.qml (Plan 10-03)
- Volume overlay and volume slider already functional from Plan 10-01
- Dynamic accent color system ready for use by all components

## Self-Check: PASSED

All files verified present:
- src/qml/components/InputCarousel.qml
- src/qml/components/NowPlaying.qml
- src/qml/components/PlaybackControls.qml

Commit e47dcd8 (Task 1) verified in git history.
Commit 355cbdc (Task 2) verified in git history.

---
*Phase: 10-qml-ui-and-production-deployment*
*Completed: 2026-02-28*
