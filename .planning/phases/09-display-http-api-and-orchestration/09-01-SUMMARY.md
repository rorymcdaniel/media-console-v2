---
phase: 09-display-http-api-and-orchestration
plan: 01
subsystem: display
tags: [ddcutil, ddc-ci, qprocess, qtimer, state-machine, display-control]

# Dependency graph
requires:
  - phase: 01-project-setup
    provides: IDisplayControl interface, StubDisplayControl, PlatformFactory
provides:
  - LinuxDisplayControl implementing IDisplayControl via ddcutil subprocess
  - ScreenTimeoutController ACTIVE/DIMMING/DIMMED/OFF state machine
  - UIState.screenDimmed Q_PROPERTY for QML binding
  - HAS_DDCUTIL compile definition and CMake ddcutil detection
affects: [10-qml-ui, 09-display-http-api-and-orchestration]

# Tech tracking
tech-stack:
  added: [ddcutil (subprocess), QProcess, QTimer (dimming animation)]
  patterns: [subprocess-wrapping for hardware control, timer-based state machine, smooth dimming animation]

key-files:
  created:
    - src/display/LinuxDisplayControl.h
    - src/display/LinuxDisplayControl.cpp
    - src/display/ScreenTimeoutController.h
    - src/display/ScreenTimeoutController.cpp
    - tests/test_LinuxDisplayControl.cpp
    - tests/test_ScreenTimeoutController.cpp
  modified:
    - src/platform/PlatformFactory.cpp
    - src/state/UIState.h
    - src/state/UIState.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "LinuxDisplayControl compiled unconditionally (uses only QProcess), HAS_DDCUTIL only controls PlatformFactory routing"
  - "ScreenTimeoutController uses seconds-to-ms conversion from DisplayConfig (dimTimeoutSeconds * 1000)"
  - "Smooth dimming: 20 steps at 50ms intervals (~1s total animation)"
  - "Door close uses doorCloseMode flag with 2s off delay, not mutating m_offTimeoutMs"

patterns-established:
  - "Subprocess hardware control: QProcess::start + waitForFinished with configurable timeout"
  - "Timer-based state machine: single-shot timers for state transitions, repeating timer for animation"
  - "Playback suppression: m_playbackSuppressing flag stops all timers during active playback"

requirements-completed: [DISP-01, DISP-02, DISP-03, DISP-04, DISP-05, DISP-06, DISP-07, DISP-08]

# Metrics
duration: 11min
completed: 2026-02-28
---

# Phase 9 Plan 1: Display Control and Screen Timeout Summary

**DDC/CI display control via ddcutil subprocess with ACTIVE/DIMMING/DIMMED/OFF state machine, playback-aware timeout suppression, and door sensor integration**

## Performance

- **Duration:** 11 min
- **Started:** 2026-02-28T20:24:00Z
- **Completed:** 2026-02-28T20:35:00Z
- **Tasks:** 2
- **Files modified:** 11

## Accomplishments
- LinuxDisplayControl wraps ddcutil for VCP 0x10 (brightness) and 0xD6 (power) with auto-detection via `ddcutil detect --brief`
- ScreenTimeoutController manages ACTIVE->DIMMING->DIMMED->OFF state transitions with configurable timers
- Smooth dimming animation over ~1 second with 20 brightness steps at 50ms intervals
- Playback-aware: screen stays on during music, door open/close triggers immediate state changes
- UIState gains screenDimmed property for Phase 10 QML binding
- 20 new tests (6 LinuxDisplayControl + 14 ScreenTimeoutController), all 343 tests pass

## Task Commits

Each task was committed atomically (TDD: test + feat):

1. **Task 1: LinuxDisplayControl with ddcutil subprocess**
   - `e55ecf0` (test) - LinuxDisplayControl guard path tests
   - `54eddb6` (feat) - Full ddcutil implementation + PlatformFactory + CMake

2. **Task 2: ScreenTimeoutController state machine**
   - `7795503` (test) - State machine tests + UIState.screenDimmed
   - `6ffaa25` (feat) - Full state machine implementation

## Files Created/Modified
- `src/display/LinuxDisplayControl.h` - DDC/CI display control header via ddcutil
- `src/display/LinuxDisplayControl.cpp` - autoDetectDisplay, setBrightness, setPower with QProcess
- `src/display/ScreenTimeoutController.h` - ACTIVE/DIMMING/DIMMED/OFF state machine header
- `src/display/ScreenTimeoutController.cpp` - Timer-based state transitions, activity detection, playback suppression
- `src/platform/PlatformFactory.cpp` - LinuxDisplayControl creation when HAS_DDCUTIL defined
- `src/state/UIState.h` - Added screenDimmed Q_PROPERTY
- `src/state/UIState.cpp` - setScreenDimmed implementation
- `CMakeLists.txt` - ddcutil detection, HAS_DDCUTIL define, ScreenTimeoutController sources
- `tests/CMakeLists.txt` - Added test files
- `tests/test_LinuxDisplayControl.cpp` - 6 tests for guard paths and initial state
- `tests/test_ScreenTimeoutController.cpp` - 14 tests covering all state transitions

## Decisions Made
- LinuxDisplayControl is compiled on all platforms (it only uses QProcess from Qt Core), with HAS_DDCUTIL controlling only the PlatformFactory routing decision
- DisplayConfig.dimTimeoutSeconds/offTimeoutSeconds are multiplied by 1000 in ScreenTimeoutController constructor for ms-based timers
- Smooth dimming uses 20 steps at 50ms intervals for ~1 second total animation
- Door close uses a doorCloseMode flag to apply a short 2-second off delay after dimming completes, rather than mutating the configured off timeout
- All ddcutil subprocess calls use a 5-second timeout with process.kill() on expiry

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required. ddcutil is only needed on the target Raspberry Pi hardware.

## Next Phase Readiness
- Display control subsystem complete, ready for QML integration in Phase 10
- ScreenTimeoutController can be wired to touch events, GPIO signals, and HTTP API activity in AppBuilder
- UIState.screenDimmed property available for QML overlay/fade effects

## Self-Check: PASSED

All 6 created files verified on disk. All 4 commit hashes found in git log.

---
*Phase: 09-display-http-api-and-orchestration*
*Completed: 2026-02-28*
