---
phase: 05-cd-subsystem
plan: 01
subsystem: cd
tags: [libcdio, libdiscid, libcdio-paranoia, cd-audio, pcm, platform-factory]

requires:
  - phase: 01-foundation
    provides: ICdDrive interface, AudioStream interface, PlatformFactory, AppConfig
provides:
  - LibcdioCdDrive (real ICdDrive using libcdio)
  - CdAudioStream (error-corrected CD audio extraction via paranoia)
  - PlatformFactory HAS_CDIO conditional wiring
  - CdConfig.idleTimeoutSeconds
affects: [05-cd-subsystem, 09-orchestration]

tech-stack:
  added: [libcdio, libcdio-paranoia, libcdio-cdda, libdiscid]
  patterns: [HAS_CDIO conditional compilation, sector-to-frame conversion, partial sector buffering]

key-files:
  created:
    - src/cd/LibcdioCdDrive.h
    - src/cd/LibcdioCdDrive.cpp
    - src/cd/CdAudioStream.h
    - src/cd/CdAudioStream.cpp
    - tests/test_CdAudioStream.cpp
  modified:
    - src/app/AppConfig.h
    - src/app/AppConfig.cpp
    - src/platform/PlatformFactory.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "CdAudioStream in unconditional LIB_SOURCES with HAS_CDIO guards in .cpp for paranoia calls -- math/state logic testable on macOS"
  - "LibcdioCdDrive stays in conditional CDIO block since all methods need libcdio"

patterns-established:
  - "HAS_CDIO conditional: same pattern as HAS_ALSA for cross-platform CD support"
  - "Sector buffering: partial sector reads handled via internal int16_t buffer"

requirements-completed: [CD-01, CD-02]

duration: 9min
completed: 2026-02-28
---

# Phase 05 Plan 01: LibcdioCdDrive + CdAudioStream Summary

**LibcdioCdDrive implementing ICdDrive via libcdio/libdiscid, CdAudioStream with paranoia error-corrected extraction exposing CD audio as PCM frames**

## Performance

- **Duration:** 9 min
- **Started:** 2026-02-28T15:06:37Z
- **Completed:** 2026-02-28T15:15:08Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- LibcdioCdDrive implements full ICdDrive interface: TOC reading, MusicBrainz disc ID via libdiscid, ioctl disc detection, spindle control
- CdAudioStream implements AudioStream with libcdio-paranoia for error-corrected audio extraction with partial sector buffering
- PlatformFactory creates LibcdioCdDrive on Linux (HAS_CDIO), StubCdDrive elsewhere
- CdConfig extended with idleTimeoutSeconds=300

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement LibcdioCdDrive with ICdDrive interface** - `7a3840c` (feat)
2. **Task 2: Implement CdAudioStream with paranoia extraction and tests** - `ffe041b` (feat)

## Files Created/Modified
- `src/cd/LibcdioCdDrive.h` - ICdDrive implementation using libcdio
- `src/cd/LibcdioCdDrive.cpp` - TOC reading, disc ID, ioctl detection, spindle control
- `src/cd/CdAudioStream.h` - AudioStream for CD audio with paranoia
- `src/cd/CdAudioStream.cpp` - Sector-to-frame conversion, partial buffering, HAS_CDIO guards
- `tests/test_CdAudioStream.cpp` - 18 tests for frame math, format accessors, error paths
- `src/app/AppConfig.h` - Added idleTimeoutSeconds to CdConfig
- `src/app/AppConfig.cpp` - QSettings loading for idle timeout
- `src/platform/PlatformFactory.cpp` - HAS_CDIO conditional for LibcdioCdDrive
- `CMakeLists.txt` - pkg_check_modules for libcdio/paranoia/cdda/discid, conditional linking
- `tests/CMakeLists.txt` - Added test_CdAudioStream

## Decisions Made
- CdAudioStream placed in unconditional LIB_SOURCES (not behind HAS_CDIO guard) because its math/state methods are testable on all platforms; only the paranoia read/seek calls are HAS_CDIO guarded in the .cpp
- LibcdioCdDrive stays in the conditional block since all its methods directly call libcdio

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] CdAudioStream link error on macOS**
- **Found during:** Task 2 (CdAudioStream tests)
- **Issue:** CdAudioStream was in the HAS_CDIO conditional sources, causing link errors on macOS since tests reference the class
- **Fix:** Moved CdAudioStream to unconditional LIB_SOURCES; HAS_CDIO guards in .cpp protect paranoia calls
- **Files modified:** CMakeLists.txt
- **Verification:** Build and all 225 tests pass on macOS
- **Committed in:** ffe041b (part of task commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Necessary for cross-platform testability. No scope creep.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- CD drive access layer complete, ready for Plan 05-02 (metadata subsystem)
- CdAudioStream available for CdController in Plan 05-03
- All 225 tests pass

---
*Phase: 05-cd-subsystem*
*Completed: 2026-02-28*
