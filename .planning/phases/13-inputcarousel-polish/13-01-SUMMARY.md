---
phase: 13-inputcarousel-polish
plan: 01
subsystem: ui
tags: [qt, qml, gpio, signals, uistate, encoder]

# Dependency graph
requires:
  - phase: 10-ui-foundation
    provides: UIState class with existing transient signal pattern (showToast, restartRequested)
  - phase: 10-ui-foundation
    provides: AppBuilder GPIO wiring with IGpioMonitor
provides:
  - UIState with three bridge signals: inputNextRequested, inputPreviousRequested, inputSelectRequested
  - UIState with three Q_INVOKABLE slots: requestInputNext, requestInputPrevious, requestInputSelect
  - AppBuilder GPIO encoder events routed through UIState (not directly to ReceiverController)
affects:
  - 13-inputcarousel-polish (QML carousel connects to UIState bridge signals in later plans)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Transient bridge signal pattern: Q_INVOKABLE slot emits corresponding signal (no state change)
    - GPIO event indirection through UIState so QML can intercept hardware events

key-files:
  created: []
  modified:
    - src/state/UIState.h
    - src/state/UIState.cpp
    - src/app/AppBuilder.cpp
    - tests/test_UIState.cpp

key-decisions:
  - "UIState bridge signals are transient (no backing property) — pure event bus for GPIO encoder turns"
  - "GPIO inputSelect (push button) routed through UIState; mute is now touch-only from AppBuilder's perspective"
  - "Q_INVOKABLE on slots enables QML to call them directly if needed in addition to C++ signal wiring"

patterns-established:
  - "Transient bridge: Q_INVOKABLE slot body is single emit statement, no guard, no state mutation"

requirements-completed: [CAR-04]

# Metrics
duration: 8min
completed: 2026-03-04
---

# Phase 13 Plan 01: UIState GPIO Signal Bridge Summary

**UIState gains three transient bridge signals (inputNextRequested/Previous/Select) so GPIO encoder events pass through QML instead of bypassing it via direct ReceiverController calls**

## Performance

- **Duration:** ~8 min
- **Started:** 2026-03-04T00:00:00Z
- **Completed:** 2026-03-04T00:08:00Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added three Q_INVOKABLE slots and matching transient signals to UIState following the existing showToast/restartRequested pattern
- Rewired AppBuilder so GPIO inputNext/inputPrevious/inputSelect connect to UIState bridge slots instead of ReceiverController directly
- Three new unit tests confirm signal emission on slot call (TDD, RED then GREEN)
- Full test suite remains green — 368/368 tests pass

## Task Commits

Each task was committed atomically:

1. **Task 1: Add UIState signal bridge (TDD)** - `d3944fc` (feat)
2. **Task 2: Rewire AppBuilder GPIO connections through UIState bridge** - `b40daa5` (feat)

**Plan metadata:** (docs commit follows)

_Note: Task 1 used TDD — tests written first (RED: compile error), then implementation (GREEN: all pass)_

## Files Created/Modified

- `src/state/UIState.h` - Added three Q_INVOKABLE slots and three transient signals in the signals: block
- `src/state/UIState.cpp` - Added emit-only slot bodies for requestInputNext/Previous/Select
- `src/app/AppBuilder.cpp` - Replaced three GPIO->ReceiverController connects with GPIO->UIState connects
- `tests/test_UIState.cpp` - Added InputNextRequestedEmits, InputPreviousRequestedEmits, InputSelectRequestedEmits tests

## Decisions Made

- UIState bridge signals are transient (no backing property) — same pattern as showToast/restartRequested. No state is stored; these are one-way event bus signals for the QML carousel to react to.
- GPIO inputSelect (push button) now routes through UIState. The mute toggle wiring is removed from AppBuilder. Mute is touch-only going forward; the push button belongs to the carousel confirm action.
- Q_INVOKABLE added to slots so QML can also call them programmatically if needed (no cost, forward-compatible).

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## Next Phase Readiness

- UIState bridge is in place. QML InputCarousel can now connect to `UIState.inputNextRequested`, `UIState.inputPreviousRequested`, and `UIState.inputSelectRequested` signals to show itself and navigate before committing an input change.
- ReceiverController::inputNext and inputPrevious are no longer called from AppBuilder; the QML carousel is responsible for calling ReceiverController.selectInput() after user confirms.

---
*Phase: 13-inputcarousel-polish*
*Completed: 2026-03-04*
