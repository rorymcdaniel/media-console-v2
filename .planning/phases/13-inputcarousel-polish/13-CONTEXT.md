# Phase 13: InputCarousel Polish - Context

**Gathered:** 2026-03-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Replace the always-visible left-panel carousel with a hidden-by-default full-screen overlay carousel. The left panel becomes a static display of the current input (icon + label). The carousel appears when the user taps the left panel or turns the input encoder, and dismisses after auto-selecting or clicking outside.

No new backend features. All changes are in QML + the AppBuilder signal wiring that routes GPIO input encoder signals.

</domain>

<decisions>
## Implementation Decisions

### Core behavior
- InputCarousel is **hidden by default** (opacity: 0, not visible in layout)
- It becomes a **full-screen overlay** with a semi-transparent dark backdrop (≈70% black)
- Tapping the backdrop without selecting dismisses the carousel without changing the input
- Tapping anywhere on the left panel (current-input display area) opens the carousel
- Turning the input encoder opens the carousel and navigates it simultaneously
- Auto-selects the focused item after **4 seconds** of no navigation activity, then closes
- Tapping an item in the carousel selects immediately and closes

### Left panel (when carousel is hidden)
- Shows the **current active input**: large icon + source name label
- Layout matches the spirit of the previous version's inputSection (icon-dominant, label below)
- Panel width stays at 280px (current v2 value — narrower than original ~480px, which is fine)
- Entire left panel is the tap target to open carousel — no separate button needed

### Carousel layout
- **Horizontal PathView** (matching previous version's horizontal swipe style)
- Large cards with scale + opacity depth effect (center card largest and fully opaque, edges recede)
- Cards are the full carousel items (icon + label), not just circles
- Previous version used 225×282 cards — adapt sizing to the 280px-tall overlay height (since the overlay fills the full 720px screen, center on cards vertically)

### Show/hide animation
- Fade in: `Easing.InOutQuad` at `Theme.animMedium` (300ms)
- Fade out: same, then set `visible: false` after animation completes (use a Timer as in original)

### Encoder wiring (C++ change required)
- In `AppBuilder.cpp`, **disconnect** `IGpioMonitor::inputNext/inputPrevious` from `ReceiverController::inputNext/inputPrevious` (these currently bypass the carousel and immediately change the receiver input)
- Add bridge signals to `UIState` (or a thin new C++ signal emitter): `inputNextRequested()` and `inputPreviousRequested()`
- Connect `IGpioMonitor::inputNext/Previous` → UIState bridge signals
- QML InputCarousel connects to these signals: show carousel if hidden, then navigate the PathView
- QML carousel's auto-select timer still calls `ReceiverController.selectInput()` — no change to ReceiverController itself

### inputSelect encoder button
- Previous behavior: opens carousel if hidden, confirms selection if visible
- Remove current mute wiring (`inputSelect` → `ReceiverController::toggleMute`)
- Wire `inputSelect` → UIState bridge signal `inputSelectRequested()`
- QML carousel: if hidden → show; if visible → selectCurrent() + hide

### Auto-select
- Restart timer on every encoder navigation (same as original)
- Stop timer when user starts flicking/dragging PathView (`onMovementStarted`)
- Restart timer when drag ends (`onMovementEnded`)
- 4-second delay (existing value, unchanged)

### Claude's Discretion
- Exact card dimensions (adapt to 720px full-screen height vs original)
- Left panel icon size and typography sizing within the 280px width
- PathView path definition (start/end x values to fill 1920px width)
- Whether to show the current-input name in the status bar area as additional context

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `Theme.qml`: `animMedium` (300ms), `animFast` (150ms), `glassBg`, `glassBorder`, accent colors — use throughout
- `ReceiverController.selectInput(source)`: Q_INVOKABLE in QML — carousel calls this on confirm
- `ReceiverState.currentInput`: MediaSource enum — used to highlight the currently active item in the carousel
- Existing `InputCarousel.qml` has the `sourceModel` ListModel, `sourceToIndex()`/`indexToSource()` mapping functions, and auto-select timer logic — all reusable
- `UIState` (C++): already a QML singleton and the right place to add bridge signals for GPIO input encoder events

### Established Patterns
- Overlay pattern (established in main.qml): `visible: false` → `opacity: 0` → animate opacity → Timer sets `visible: false` after fade-out
- All existing overlays (volume overlay, toast, error dialog) use `opacity: visible ? 1.0 : 0.0` + `Behavior on opacity`
- `Easing.OutCubic` is the project standard for most animations — `InOutQuad` for the carousel fade matches the original's behavior

### Integration Points
- `AppBuilder.cpp` lines 82-89: where `inputNext/Previous/Select` are currently wired — the disconnect/rewire happens here
- `UIState.h/cpp`: add `inputNextRequested()`, `inputPreviousRequested()`, `inputSelectRequested()` signals + Q_INVOKABLE slots that emit them
- `main.qml` left panel (lines 284-303): replace `InputCarousel { anchors.fill: parent }` with a static current-input display component
- The overlay `InputCarousel` goes at the root of `main.qml` (same z-layer as the volume overlay, around z: 500)

</code_context>

<specifics>
## Specific Ideas

- "Fully hidden until you either press an input selection button or turn the input selector knob" — this is the exact intent from the user
- "While you do not need to match the style 100%" — the horizontal PathView style is chosen to match the original's feel, but exact sizing and theming can follow the v2 style guide
- The left panel in the previous version showed a 256×256 icon + 64px label — adapt proportionally to the 280px panel width

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 13-inputcarousel-polish*
*Context gathered: 2026-03-04*
