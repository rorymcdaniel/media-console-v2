---
phase: 07-gpio-hardware
plan: 02
subsystem: platform
tags: [gpio, libgpiod, quadrature-decoder, rotary-encoder, reed-switch, qthread]

requires:
  - phase: 07-gpio-hardware
    provides: "IGpioMonitor with volumeChanged(int delta), GpioConfig struct, CMake HAS_GPIOD"
provides:
  - "QuadratureDecoder state machine for CW/CCW rotary encoder direction"
  - "LinuxGpioMonitor: libgpiod v2 driver monitoring 7 GPIO lines in single request"
  - "Background QThread with poll()-based event loop (100ms timeout)"
  - "Pre-scaled volume delta emission, FALLING-edge push button, debounced reed switch"
affects: [07-03]

tech-stack:
  added: [libgpiod v2 C++ bindings (gpiod.hpp)]
  patterns: [quadrature state machine lookup table, single-request multi-line GPIO monitoring, poll()-based edge event loop]

key-files:
  created:
    - src/platform/QuadratureDecoder.h
    - src/platform/LinuxGpioMonitor.h
    - src/platform/LinuxGpioMonitor.cpp
    - tests/test_QuadratureDecoder.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "QuadratureDecoder in separate header (pure C++) for cross-platform testability"
  - "Push button emits inputSelect() -- wiring layer routes to mute or carousel confirm"
  - "Reed switch polarity: INACTIVE (high) = magnets apart = door open = true"
  - "Fixed lookup table indexing to use binary order (00=0, 01=1, 10=2, 11=3) not Gray code order"

patterns-established:
  - "Quadrature decoder: 4x4 lookup table indexed by (A<<1)|B state"
  - "GPIO monitor: single libgpiod line_request for all lines, poll() event loop"
  - "Background thread: QThread with atomic stop flag and 100ms poll timeout"

requirements-completed: [GPIO-01, GPIO-02, GPIO-03, GPIO-04, GPIO-06]

duration: 12min
completed: 2026-02-28
---

# Plan 07-02: QuadratureDecoder and LinuxGpioMonitor Summary

**Quadrature state machine decodes rotary encoder direction; LinuxGpioMonitor drives volume, input, mute, and reed switch via libgpiod v2 poll() loop**

## Performance

- **Duration:** 12 min
- **Tasks:** 1
- **Files created:** 4
- **Files modified:** 2

## Accomplishments
- QuadratureDecoder correctly decodes CW (+1) and CCW (-1) from Gray code state transitions with noise rejection
- LinuxGpioMonitor requests all 7 GPIO lines in a single libgpiod v2 line_request
- Background QThread with poll()-based event loop and 100ms timeout for responsive stop
- Push button uses FALLING edge only with 250ms hardware debounce (fixes GPIO-06 double-toggle)
- Reed switch reads initial state on start() -- no ambiguous initial state

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement QuadratureDecoder and LinuxGpioMonitor** - `cb6cd1b` (feat)

## Files Created/Modified
- `src/platform/QuadratureDecoder.h` - Pure-logic Gray code state machine (cross-platform testable)
- `src/platform/LinuxGpioMonitor.h` - Real GPIO monitor header (HAS_GPIOD guarded)
- `src/platform/LinuxGpioMonitor.cpp` - Full implementation with libgpiod v2 (HAS_GPIOD guarded)
- `tests/test_QuadratureDecoder.cpp` - 9 tests covering CW, CCW, invalid, reset, full revolution
- `CMakeLists.txt` - Added QuadratureDecoder.h to unconditional LIB_SOURCES
- `tests/CMakeLists.txt` - Added test_QuadratureDecoder.cpp

## Decisions Made
- Separated QuadratureDecoder into own header for cross-platform testability (no gpiod dependency)
- Push button emits inputSelect() -- context-dependent routing deferred to wiring layer
- Fixed lookup table to use binary state indexing (00=0, 01=1, 10=2, 11=3) not Gray code order

## Deviations from Plan

### Auto-fixed Issues

**1. Quadrature lookup table indexing error**
- **Found during:** Task 1 (QuadratureDecoder tests)
- **Issue:** Table column order was Gray code (00,01,11,10) but indexing is binary (00,01,10,11)
- **Fix:** Reordered table columns to match binary state encoding
- **Verification:** All 9 QuadratureDecoder tests pass

---

**Total deviations:** 1 auto-fixed (lookup table indexing)
**Impact on plan:** Bug fix during TDD red phase -- exactly what TDD is for.

## Issues Encountered
None beyond the table indexing fix caught by tests.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- LinuxGpioMonitor ready for PlatformFactory integration (Plan 03)
- All signals match IGpioMonitor interface for AppBuilder wiring

---
*Plan: 07-02 (gpio-hardware)*
*Completed: 2026-02-28*
