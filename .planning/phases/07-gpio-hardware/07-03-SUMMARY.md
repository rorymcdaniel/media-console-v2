---
phase: 07-gpio-hardware
plan: 03
subsystem: app
tags: [gpio, platform-factory, appbuilder, signal-wiring, input-cycling]

requires:
  - phase: 07-gpio-hardware
    provides: "IGpioMonitor, LinuxGpioMonitor, GpioConfig, UIState.doorOpen"
  - phase: 03-receiver-control
    provides: "ReceiverController, VolumeGestureController"
provides:
  - "PlatformFactory HAS_GPIOD conditional GPIO monitor creation"
  - "AppBuilder GPIO signal wiring: volume, input, mute, reed switch"
  - "ReceiverController.inputNext/inputPrevious for 6-source cycling"
  - "GPIO monitor auto-start during AppBuilder.build() with non-fatal failure"
affects: [09-display-http-api, 10-qml-ui]

tech-stack:
  added: []
  patterns: [GPIO signal wiring via Qt connect, input source cycling with wrap-around]

key-files:
  created: []
  modified:
    - src/platform/PlatformFactory.h
    - src/platform/PlatformFactory.cpp
    - src/app/AppBuilder.cpp
    - src/receiver/ReceiverController.h
    - src/receiver/ReceiverController.cpp
    - tests/test_PlatformFactory.cpp

key-decisions:
  - "Push button routes to toggleMute for now; Phase 10 InputCarousel will add context-dependent routing"
  - "ReceiverController.inputNext/inputPrevious added as convenience methods that cycle through 6 sources with wrap-around"
  - "GPIO monitor failure is non-fatal -- logs warning and continues with no hardware input"

patterns-established:
  - "GPIO signal wiring: volumeChanged->onEncoderTick, inputSelect->toggleMute, reedSwitchChanged->setDoorOpen"
  - "Source cycling order: Streaming, Phono, CD, Computer, Bluetooth, Library"

requirements-completed: [GPIO-01, GPIO-02, GPIO-03, GPIO-04, GPIO-05]

duration: 8min
completed: 2026-02-28
---

# Plan 07-03: PlatformFactory and AppBuilder Wiring Summary

**PlatformFactory returns LinuxGpioMonitor on Linux; AppBuilder wires all GPIO signals to volume gestures, input cycling, mute toggle, and door state**

## Performance

- **Duration:** 8 min
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- PlatformFactory returns LinuxGpioMonitor when HAS_GPIOD defined, StubGpioMonitor otherwise
- All 5 GPIO signal connections wired in AppBuilder: volumeChanged, inputNext, inputPrevious, inputSelect, reedSwitchChanged
- ReceiverController gains inputNext/inputPrevious for wrap-around 6-source cycling
- GPIO monitor starts during build() with non-fatal failure handling

## Task Commits

Each task was committed atomically:

1. **Task 1: Update PlatformFactory for HAS_GPIOD conditional** - `fcb44ca` (feat)
2. **Task 2: Wire GPIO signals in AppBuilder** - `626d2c1` (feat)

## Files Created/Modified
- `src/platform/PlatformFactory.h` - createGpioMonitor accepts GpioConfig
- `src/platform/PlatformFactory.cpp` - HAS_GPIOD conditional with LinuxGpioMonitor
- `src/app/AppBuilder.cpp` - GPIO signal wiring and monitor start
- `src/receiver/ReceiverController.h` - Added inputNext/inputPrevious slots
- `src/receiver/ReceiverController.cpp` - Input cycling with 6-source wrap-around
- `tests/test_PlatformFactory.cpp` - Updated to pass GpioConfig

## Decisions Made
- Push button routes to toggleMute for now -- Phase 10 adds context-dependent routing via InputCarousel
- ReceiverController owns input cycling logic (not a separate controller) for simplicity
- Source cycling order matches InputCarousel: Streaming, Phono, CD, Computer, Bluetooth, Library

## Deviations from Plan

### Auto-fixed Issues

**1. Added inputNext/inputPrevious to ReceiverController**
- **Found during:** Task 2 (AppBuilder wiring)
- **Issue:** ReceiverController had selectInput(MediaSource) but no convenience methods for cycling
- **Fix:** Added inputNext/inputPrevious that compute next/previous MediaSource and call selectInput
- **Files modified:** ReceiverController.h, ReceiverController.cpp
- **Verification:** Full test suite passes (279 tests)
- **Committed in:** 626d2c1

---

**Total deviations:** 1 auto-fixed (missing convenience methods)
**Impact on plan:** Plan anticipated this -- noted "Check ReceiverController.h during execution" with decision to add cycling methods.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 7 GPIO Hardware complete -- all signals wired and functional
- Phase 9 DisplayController can react to UIState.doorOpen changes
- Phase 10 InputCarousel can replace the push button -> toggleMute connection with context-dependent routing

---
*Plan: 07-03 (gpio-hardware)*
*Completed: 2026-02-28*
