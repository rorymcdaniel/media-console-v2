---
phase: 13-inputcarousel-polish
plan: 02
subsystem: ui
tags: [qt, qml, pathview, carousel, overlay, uistate, encoder]

# Dependency graph
requires:
  - phase: 13-inputcarousel-polish
    provides: UIState bridge signals inputNextRequested/Previous/Select for encoder events
  - phase: 10-ui-foundation
    provides: Theme design tokens, ReceiverState/ReceiverController/UIState singletons, overlay z-layer pattern

provides:
  - InputCarousel.qml as full-screen PathView overlay (hidden by default, show/hide functions)
  - main.qml left panel static current-input display (icon + label, tap-to-open)
  - InputCarousel placed at z:500 as window-root sibling in main.qml

affects:
  - 13-inputcarousel-polish (subsequent plans in this phase build on this overlay UI)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - PathView overlay: visible=true before opacity=1.0 so Behavior on opacity fires (Qt skips animations on invisible items)
    - Overlay hide via opacity fade + hideTimer to defer visible=false until animation completes
    - PathView depth effect using (index - (n - offset) % n) raw distance formula with wrap-around correction

key-files:
  created: []
  modified:
    - src/qml/components/InputCarousel.qml
    - src/qml/main.qml

key-decisions:
  - "PathView uses currentIndex assignment (not positionViewAtIndex — ListView-only API) for ReceiverState sync"
  - "autoSelectTimer.stop() + autoSelectProgress reset called inside hide() to prevent stale auto-select after dismiss"
  - "show() sets visible=true before opacity=1.0 — required for Behavior on opacity to animate on an Item"
  - "InputCarousel placed at z:500 as window-root sibling after contentRow, consistent with volumeOverlay z-layer"

patterns-established:
  - "Overlay pattern: visible=true then opacity=1.0 in show(); opacity=0.0 then hideTimer sets visible=false in hide()"
  - "PathView depth effect: distFromCenter computed from (index - (n - offset) % n) with n/2 wrap correction"

requirements-completed: [CAR-01, CAR-02, CAR-03, CAR-04]

# Metrics
duration: 3min
completed: 2026-03-04
---

# Phase 13 Plan 02: InputCarousel Overlay Summary

**Horizontal PathView carousel rewritten as hidden-by-default full-screen overlay with 70% black backdrop, depth effect, and tap-to-open left panel**

## Performance

- **Duration:** ~3 min
- **Started:** 2026-03-04T16:35:11Z
- **Completed:** 2026-03-04T16:38:13Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- InputCarousel.qml fully rewritten from vertical ListView in left panel to hidden-by-default full-screen PathView overlay
- Horizontal PathView with 6-item depth effect: center card 1.0/1.0, adjacent 0.72/0.55, outer 0.45/0.25 scale/opacity
- UIState Connections block handles all three encoder signals (inputNextRequested/Previous/Select) — opens carousel on encoder turn, confirms or opens on select press
- Auto-select timer properly stops and resets on hide() — no stale auto-select after overlay dismissal
- main.qml left panel replaced with static accent-colored icon circle + input label; tap-to-open wires through inputCarousel.show()
- Full build clean, 368/368 tests pass

## Task Commits

Each task was committed atomically:

1. **Task 1: Rewrite InputCarousel.qml as full-screen overlay with horizontal PathView** - `b0f476d` (feat)
2. **Task 2: Replace left panel carousel embed with static current-input display in main.qml** - `f974c7b` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `src/qml/components/InputCarousel.qml` - Full rewrite: Item root with anchors.fill:parent, carouselOverlay Rectangle (70% black, hidden by default), horizontal PathView with depth-effect delegate, UIState/ReceiverState Connections, public show() function
- `src/qml/main.qml` - Left panel: replaced InputCarousel embed with MouseArea + Column (icon + label); added currentInputIconText()/currentInputLabelText() helpers; added InputCarousel at z:500 as window-root sibling

## Decisions Made

- PathView uses `carousel.currentIndex = idx` for ReceiverState sync. `positionViewAtIndex()` is a ListView-only API and does not exist on PathView.
- `hide()` explicitly calls `autoSelectTimer.stop()` and resets `autoSelectProgress = 0.0` to prevent the auto-select countdown from completing after the overlay has been dismissed.
- `show()` sets `visible = true` before `opacity = 1.0`. Qt skips Behavior animations on invisible items, so the opacity must be changed after the item becomes visible.
- InputCarousel sits at z:500, same level as ejectConfirmDialog, matching the existing overlay z-layer convention in main.qml.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## Next Phase Readiness

- Full carousel overlay is functional: tap left panel opens it, encoder events open and navigate it, card tap or auto-select closes it.
- ReceiverState.currentInput changes sync the carousel index correctly via currentIndex assignment.
- Ready for any further polish tasks in Phase 13 (animation tuning, visual refinements, etc.).

---
*Phase: 13-inputcarousel-polish*
*Completed: 2026-03-04*
