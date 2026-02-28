---
phase: 05-cd-subsystem
plan: 03
subsystem: cd
tags: [lifecycle, disc-detection, progressive-display, idle-timer, spindle, polling]

requires:
  - phase: 05-01
    provides: LibcdioCdDrive, CdAudioStream
  - phase: 05-02
    provides: CdMetadataCache, CdMetadataProvider, CdAlbumArtProvider
provides:
  - CdController (lifecycle orchestrator for disc detection, metadata, playback)
  - Enhanced StubCdDrive (configurable TOC, disc ID, test observation)
affects: [05-cd-subsystem, 09-orchestration, 10-ui]

tech-stack:
  patterns: [QTimer polling, progressive signal chain, idle timeout, non-owning pointer wiring]

key-files:
  created:
    - src/cd/CdController.h
    - src/cd/CdController.cpp
    - tests/test_CdController.cpp
  modified:
    - src/platform/stubs/StubCdDrive.h
    - src/platform/stubs/StubCdDrive.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "CdController passes nullptr for playbackController in tests, null-checks before use"
  - "StubCdDrive enhanced with setToc/setDiscId/setAudioDisc/stopSpindleCallCount for comprehensive testing"
  - "Idle timer is single-shot, restarted on each CD activity, fires stopSpindle on drive"
  - "No Q_DECLARE_METATYPE in headers to avoid explicit-after-implicit-instantiation errors; registered in test file"

patterns-established:
  - "Progressive display signal chain: discDetected -> tocReady -> metadataReady -> albumArtReady"
  - "Metadata failure fallback: silent 'Audio CD' artist/album, no error indicators"
  - "Idle spindle management: configurable timeout, reset on activity, single-shot timer"

requirements-completed: [CD-03, CD-04, CD-06, CD-10, CD-11, CD-12]

duration: 8min
completed: 2026-02-28
---

# Phase 05 Plan 03: CdController Summary

**CdController lifecycle orchestrator coordinating disc detection, progressive metadata loading, idle spindle management, and user-initiated playback**

## Performance

- **Duration:** 8 min
- **Tasks:** 2 (implementation + tests)
- **Files modified:** 7

## Accomplishments
- CdController polls for disc insertion/removal via QTimer at configurable interval
- Progressive display: discDetected -> TOC -> cache check -> async metadata -> album art
- Cache hit path: instant metadata emission without network I/O
- Metadata failure: silent fallback to "Audio CD" artist/album (CD-10)
- No auto-play: disc insertion emits informational signals only (CD-04)
- Idle timer: configurable timeout stops CD spindle after inactivity (CD-11)
- Disc removal: cancels pending lookups, stops active playback, clears state
- Non-audio disc: emits nonAudioDiscDetected signal, no further processing
- Enhanced StubCdDrive with configurable TOC, disc ID, audio flag, and call counters
- 9 unit tests covering all major CdController behaviors

## Task Commits

Each task was committed atomically:

1. **Task 1+2: CdController implementation and tests** - `0fa594c` (feat)

## Files Created/Modified
- `src/cd/CdController.h` - Lifecycle orchestrator interface with progressive display signals
- `src/cd/CdController.cpp` - Polling, detection, metadata chain, idle timer, playback
- `tests/test_CdController.cpp` - 9 tests for insertion, removal, non-audio, no-autoplay, idle, eject
- `src/platform/stubs/StubCdDrive.h` - Enhanced with setToc/setDiscId/setAudioDisc/callCounters
- `src/platform/stubs/StubCdDrive.cpp` - Updated implementations for test configurability
- `CMakeLists.txt` - Added CdController to LIB_SOURCES
- `tests/CMakeLists.txt` - Added test_CdController

## Decisions Made
- StubCdDrive enhanced rather than creating a separate mock to keep the test infrastructure simple
- Metatype declarations placed in test file to avoid template instantiation order issues with Qt MOC

## Deviations from Plan
None.

## Issues Encountered
None

## User Setup Required
None

## Next Phase Readiness
- CdController complete, ready for Plan 05-04 (AppBuilder wiring)
- All 252 tests pass

---
*Phase: 05-cd-subsystem*
*Completed: 2026-02-28*
