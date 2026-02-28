---
phase: 07-gpio-hardware
plan: 01
subsystem: platform
tags: [gpio, libgpiod, qt-property, encoder, reed-switch]

requires:
  - phase: 01-foundation
    provides: IGpioMonitor interface, StubGpioMonitor, PlatformFactory, AppConfig
  - phase: 02-state-layer
    provides: UIState Q_PROPERTY pattern
provides:
  - "IGpioMonitor with volumeChanged(int delta) signal replacing volumeUp/volumeDown"
  - "UIState.doorOpen Q_PROPERTY for reed switch state"
  - "GpioConfig struct with configurable chip path and pin assignments"
  - "CMake libgpiodcxx conditional compilation with HAS_GPIOD"
  - "StubGpioMonitor with simulateVolumeChange(int delta)"
affects: [07-02, 07-03, 09-display-control]

tech-stack:
  added: [libgpiodcxx (conditional)]
  patterns: [HAS_GPIOD conditional compilation, delta-based volume signal]

key-files:
  created: []
  modified:
    - src/platform/IGpioMonitor.h
    - src/platform/stubs/StubGpioMonitor.h
    - src/platform/stubs/StubGpioMonitor.cpp
    - src/state/UIState.h
    - src/state/UIState.cpp
    - src/app/AppConfig.h
    - src/app/AppConfig.cpp
    - CMakeLists.txt
    - tests/test_StubGpioMonitor.cpp
    - tests/test_UIState.cpp

key-decisions:
  - "volumeChanged(int delta) replaces volumeUp/volumeDown -- carries pre-scaled delta for VolumeGestureController"
  - "doorOpen defaults to true (display on assumption) with change guard"
  - "GpioConfig uses /dev/gpiochip4 default (RPi5 GPIO header)"

patterns-established:
  - "Delta-based volume signal: volumeChanged(int delta) carries +/-N per detent"
  - "HAS_GPIOD conditional compilation: same pattern as HAS_ALSA and HAS_CDIO"

requirements-completed: [GPIO-05, GPIO-06]

duration: 8min
completed: 2026-02-28
---

# Plan 07-01: GPIO Interface Evolution Summary

**IGpioMonitor updated to delta-based volumeChanged signal, UIState gains doorOpen property, CMake detects libgpiodcxx conditionally**

## Performance

- **Duration:** 8 min
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- Replaced volumeUp/volumeDown signals with volumeChanged(int delta) for proper VolumeGestureController integration
- Added doorOpen Q_PROPERTY to UIState with default true and change guard
- Added GpioConfig struct with all pin assignments, debounce values, and chip path
- Added CMake libgpiodcxx conditional detection with HAS_GPIOD compile definition

## Task Commits

Each task was committed atomically:

1. **Task 1: Update IGpioMonitor interface, StubGpioMonitor, and GpioConfig** - `11753bb` (feat)
2. **Task 2: Add doorOpen Q_PROPERTY to UIState and CMake libgpiodcxx support** - `45d4acb` (feat)

## Files Created/Modified
- `src/platform/IGpioMonitor.h` - Replaced volumeUp/volumeDown with volumeChanged(int delta)
- `src/platform/stubs/StubGpioMonitor.h` - Updated simulate methods to match new interface
- `src/platform/stubs/StubGpioMonitor.cpp` - Implemented simulateVolumeChange(int delta)
- `src/state/UIState.h` - Added doorOpen Q_PROPERTY
- `src/state/UIState.cpp` - Implemented setDoorOpen with change guard
- `src/app/AppConfig.h` - Added GpioConfig struct with pin assignments
- `src/app/AppConfig.cpp` - Added GPIO section reading from QSettings
- `CMakeLists.txt` - Added conditional libgpiodcxx detection and HAS_GPIOD
- `tests/test_StubGpioMonitor.cpp` - Updated tests for volumeChanged signal
- `tests/test_UIState.cpp` - Added doorOpen tests

## Decisions Made
- Used volumeChanged(int delta) instead of separate signals -- carries pre-scaled delta for gesture controller
- doorOpen defaults to true (door open = display on assumption)
- GpioConfig defaults to /dev/gpiochip4 for Raspberry Pi 5

## Deviations from Plan
None - plan executed exactly as written

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Interface contracts stable for LinuxGpioMonitor implementation (Plan 02)
- GpioConfig ready for constructor injection
- CMake will compile LinuxGpioMonitor only when libgpiod found on Linux

---
*Plan: 07-01 (gpio-hardware)*
*Completed: 2026-02-28*
