# Phase 13: InputCarousel Polish - Research

**Researched:** 2026-03-04
**Domain:** QML animation, ListView/PathView, full-screen overlay pattern, C++ signal bridge (Qt/QML)
**Confidence:** HIGH

## Summary

Phase 13 transforms the always-visible left-panel input carousel into a hidden-by-default full-screen overlay. The change is pure QML and C++ signal wiring — no new backend logic, no new data models, no new routes. The existing `InputCarousel.qml` provides the right data model and business logic (sourceModel, sourceToIndex/indexToSource, auto-select timer, focusInput/selectImmediately). What must change: (1) reshape the carousel from a vertical ListView in the left panel to a horizontal PathView overlay filling the full 1920x720 window, (2) add show/hide behavior with a semi-transparent backdrop, (3) add a static left panel showing the current input, and (4) rewire GPIO encoder signals through UIState bridge signals instead of bypassing QML.

The most important technical constraint is Pi 5 performance. ListView with SnapToItem performs well on the Pi 5. PathView with its path-based delegate transforms can cause layout recalculation overhead on every content scroll event — the existing vertical ListView approach is already proven smooth. The phase should continue with ListView or use PathView carefully. The CONTEXT.md locks "horizontal PathView" as the approach — this is achievable on Pi 5 if the PathView path is a simple straight horizontal line (no curves) and delegate count is kept low (6 items, all pre-created, no Loader).

The C++ change is minimal: add three pure signals to `UIState` (`inputNextRequested`, `inputPreviousRequested`, `inputSelectRequested`), add Q_INVOKABLE slots that emit them, and in AppBuilder disconnect the current direct GPIO-to-ReceiverController connections and reconnect through UIState.

**Primary recommendation:** Keep PathView path as a single straight horizontal line. Use fixed-count delegates (6 items, no pathItemCount restriction). Reuse existing auto-select Timer, focusInput(), selectImmediately() logic verbatim. Follow the established overlay pattern from the volume overlay and eject confirmation dialog.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Core behavior:**
- InputCarousel is hidden by default (opacity: 0, not visible in layout)
- It becomes a full-screen overlay with a semi-transparent dark backdrop (~70% black)
- Tapping the backdrop without selecting dismisses the carousel without changing the input
- Tapping anywhere on the left panel (current-input display area) opens the carousel
- Turning the input encoder opens the carousel and navigates it simultaneously
- Auto-selects the focused item after 4 seconds of no navigation activity, then closes
- Tapping an item in the carousel selects immediately and closes

**Left panel (when carousel is hidden):**
- Shows the current active input: large icon + source name label
- Layout matches the spirit of the previous version's inputSection (icon-dominant, label below)
- Panel width stays at 280px (current v2 value)
- Entire left panel is the tap target to open carousel — no separate button needed

**Carousel layout:**
- Horizontal PathView (matching previous version's horizontal swipe style)
- Large cards with scale + opacity depth effect (center card largest and fully opaque, edges recede)
- Cards are the full carousel items (icon + label), not just circles
- Previous version used 225x282 cards — adapt sizing to 280px-tall overlay height

**Show/hide animation:**
- Fade in: Easing.InOutQuad at Theme.animMedium (300ms)
- Fade out: same, then set visible: false after animation completes (use a Timer as in original)

**Encoder wiring (C++ change required):**
- In AppBuilder.cpp, disconnect IGpioMonitor::inputNext/inputPrevious from ReceiverController::inputNext/inputPrevious
- Add bridge signals to UIState: inputNextRequested() and inputPreviousRequested()
- Connect IGpioMonitor::inputNext/Previous to UIState bridge signals
- QML InputCarousel connects to these signals: show carousel if hidden, then navigate the PathView
- QML carousel's auto-select timer still calls ReceiverController.selectInput() — no change to ReceiverController

**inputSelect encoder button:**
- Remove current mute wiring (inputSelect -> ReceiverController::toggleMute)
- Wire inputSelect -> UIState bridge signal inputSelectRequested()
- QML carousel: if hidden -> show; if visible -> selectCurrent() + hide

**Auto-select:**
- Restart timer on every encoder navigation
- Stop timer when user starts flicking/dragging PathView (onMovementStarted)
- Restart timer when drag ends (onMovementEnded)
- 4-second delay (existing value, unchanged)

**Integration points:**
- AppBuilder.cpp lines 82-89: where inputNext/Previous/Select are currently wired
- UIState.h/cpp: add inputNextRequested(), inputPreviousRequested(), inputSelectRequested() signals + Q_INVOKABLE slots
- main.qml left panel (lines 284-303): replace InputCarousel { anchors.fill: parent } with static current-input display component
- The overlay InputCarousel goes at the root of main.qml (same z-layer as the volume overlay, around z: 500)

### Claude's Discretion
- Exact card dimensions (adapt to 720px full-screen height vs original)
- Left panel icon size and typography sizing within the 280px width
- PathView path definition (start/end x values to fill 1920px width)
- Whether to show the current-input name in the status bar area as additional context

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CAR-01 | InputCarousel rotation animation is smooth and continuous — no jank or frame drops on Pi 5 | PathView with straight-line path + fixed 6 delegates + no Loader avoids layout recalculation overhead. Behavior-based scale/opacity animations use GPU compositing. |
| CAR-02 | Carousel snaps to the selected input with a natural deceleration curve | PathView has built-in `preferredHighlightBegin/End` + `highlightRangeMode: StrictlyEnforceRange`. `snapMode: PathView.SnapToItem` provides the deceleration curve. The drag-release momentum is handled by Qt's physics simulation. |
| CAR-03 | Non-selected inputs are visually de-emphasised (scale, opacity) to reinforce the 3D wheel illusion | PathView path attributes (PathPercent + PathAttribute) or delegate-computed distance from offset=0.5 drive scale and opacity. Behavior on scale/opacity uses OutCubic easing (Theme.animMedium). |
| CAR-04 | Physical encoder rotation drives the carousel with no perceptible lag — one encoder event, one immediate visual step | Bridge signals from UIState fire synchronously on the Qt main thread. PathView.incrementCurrentIndex() / decrementCurrentIndex() are called directly in the QML signal handler — no timers, no deferred queuing. |
</phase_requirements>

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Quick / QML | Qt 6 (project-current) | PathView, ListView, Behavior, NumberAnimation, Timer | Already the project's entire UI layer |
| QtQuick.Controls | Qt 6 | No new controls needed this phase | Already imported |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Qt C++ QObject signals | Qt 6 | Bridge GPIO events to QML | Adding inputNextRequested / inputPreviousRequested / inputSelectRequested to UIState |
| GoogleTest + QSignalSpy | GTest v1.17.0 + Qt 6 | Unit test UIState bridge signals | Existing test pattern for all state classes |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| PathView (horizontal) | ListView horizontal + SnapToItem | ListView is already proven smooth in this codebase (Phase 10 decision). PathView allows path-attribute-driven scale/opacity which is cleaner than distance-from-center math in delegate. Since CONTEXT locks PathView, use it. |
| PathView path attributes for depth | Delegate-computed distFromCenter (existing approach) | The existing approach in InputCarousel.qml uses contentY-based distFromCenter. PathView uses offset-based distFromCenter via `PathView.offset`. Both work; delegate-computed is simpler to understand and avoids PathAttribute complexity. |
| UIState bridge signals | New thin C++ singleton | UIState is already the correct QML singleton for UI event bridge. No new class needed. |

**Installation:** No new packages. All dependencies already in project.

---

## Architecture Patterns

### Recommended Project Structure

No new files required. Changes touch:

```
src/
├── state/UIState.h            # Add 3 signals + 3 Q_INVOKABLE slots
├── state/UIState.cpp          # Implement the 3 slots (emit-only)
├── app/AppBuilder.cpp         # Rewire GPIO lines 82-89
└── qml/
    ├── main.qml               # Left panel replacement + carousel overlay
    └── components/
        └── InputCarousel.qml  # Full rewrite: horizontal PathView + overlay behavior
```

### Pattern 1: Full-Screen Overlay (Established Project Pattern)

**What:** `visible: false` start state; set `visible: true` first; animate `opacity` 0→1 with `Behavior`; on hide, animate 0, then `Timer` sets `visible: false` after animation completes.

**When to use:** Every overlay in this project uses this pattern. The carousel uses it too.

**Example (from existing volume overlay in main.qml):**
```qml
// Source: main.qml lines 344-349
Rectangle {
    visible: UIState.volumeOverlayVisible
    z: 500
    opacity: visible ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutCubic }
    }
}
```

**Carousel variant** (backdrop dismiss + fade-out timer):
```qml
Rectangle {
    id: carouselOverlay
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.70)
    visible: false      // starts hidden
    z: 500
    opacity: 0.0

    Behavior on opacity {
        NumberAnimation { duration: Theme.animMedium; easing.type: Easing.InOutQuad }
    }

    // Backdrop tap dismisses without selecting
    MouseArea {
        anchors.fill: parent
        onClicked: carouselOverlay.hide()
    }

    function show() {
        visible = true
        opacity = 1.0
    }

    function hide() {
        opacity = 0.0
        hideTimer.restart()
    }

    Timer {
        id: hideTimer
        interval: Theme.animMedium
        onTriggered: carouselOverlay.visible = false
    }
}
```

**Note:** The show() call must set `visible = true` BEFORE `opacity = 1.0`, otherwise the Behavior never fires (the item was invisible when opacity changed). This ordering is critical and is why the project uses `visible: false` + separate `opacity` property rather than `visible: opacity > 0`.

### Pattern 2: PathView with Straight-Line Path

**What:** A PathView whose Path is a single horizontal line from left to right. Delegates are positioned along the line. `preferredHighlightBegin` and `preferredHighlightEnd` at 0.5 keeps the selected item centered.

**When to use:** Horizontal swipe carousel with snap-to-item behavior.

**Example:**
```qml
PathView {
    id: carousel
    anchors.fill: parent
    model: sourceModel
    currentIndex: root.focusedIndex >= 0 ? root.focusedIndex : 0
    snapMode: PathView.SnapToItem
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    highlightRangeMode: PathView.StrictlyEnforceRange
    interactive: true
    pathItemCount: 6   // all 6 items, always in scene

    path: Path {
        startX: -cardWidth / 2    // center of leftmost card at left edge
        startY: carousel.height / 2
        PathLine {
            x: carousel.width + cardWidth / 2  // center of rightmost card at right edge
            y: carousel.height / 2
        }
    }

    delegate: Item {
        // PathView.offset gives fractional position 0..1 along path
        // Distance from center (0.5) drives scale and opacity
        property real pathPos: PathView.isCurrentItem ? 0 : 0.5 // see note below
    }
}
```

**Important:** PathView wraps. With 6 items on a straight line, wrapping can cause visual glitches (item jumps from right edge to left edge). Set `pathItemCount` to 6 (all items) so Qt keeps all delegates active. The wrap is invisible because all 6 are always rendered — the illusion of non-wrapping behavior on a short list.

### Pattern 3: Delegate Distance-from-Center for 3D Depth Effect

**What:** In a PathView delegate, use `PathView.view.offset` and the delegate's position in the model to compute distance from center. Drive `scale` and `opacity` from this distance.

**When to use:** When non-selected items must appear smaller and dimmer than the selected item.

**Example (delegate body):**
```qml
delegate: Item {
    id: delegateItem
    width: cardWidth
    height: cardHeight

    required property int index
    required property string labelText
    required property string iconText
    required property int sourceValue

    // Fractional distance from center position (0 = center, 1 = one slot away)
    property real distFromCenter: {
        var n = sourceModel.count
        var offset = PathView.view.offset
        var raw = index - (n - offset) % n
        // Normalize to [-n/2, n/2]
        if (raw > n / 2) raw -= n
        if (raw < -n / 2) raw += n
        return Math.abs(raw)
    }

    property real itemScale: distFromCenter < 0.5 ? 1.0
                           : distFromCenter < 1.5 ? 0.72
                           : 0.45

    property real itemOpacity: distFromCenter < 0.5 ? 1.0
                             : distFromCenter < 1.5 ? 0.55
                             : 0.25

    scale: itemScale
    opacity: itemOpacity

    Behavior on scale   { NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic } }
    Behavior on opacity { NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic } }
}
```

**Note:** The `distFromCenter` formula above is the standard pattern for PathView offset-based distance. It accounts for wrapping. The existing ListView `distFromCenter` in InputCarousel.qml used `contentY`-based math which does not translate to PathView.

### Pattern 4: UIState Signal Bridge (C++ side)

**What:** Add pure-transient signals to UIState that carry no state — they are events, not properties. QML connects to these signals via `Connections`. AppBuilder routes GPIO signals through them.

**When to use:** When a hardware event must reach QML but should not permanently change any C++ property.

**Example (UIState.h addition):**
```cpp
signals:
    // ... existing signals ...
    void inputNextRequested();
    void inputPreviousRequested();
    void inputSelectRequested();

public slots:
    // ... existing slots ...
    Q_INVOKABLE void requestInputNext();
    Q_INVOKABLE void requestInputPrevious();
    Q_INVOKABLE void requestInputSelect();
```

**UIState.cpp addition:**
```cpp
void UIState::requestInputNext()     { emit inputNextRequested(); }
void UIState::requestInputPrevious() { emit inputPreviousRequested(); }
void UIState::requestInputSelect()   { emit inputSelectRequested(); }
```

**AppBuilder.cpp change (lines 82-89):**
```cpp
// BEFORE:
connect(m_gpioMonitor.get(), &IGpioMonitor::inputNext,
        m_receiverController.get(), &ReceiverController::inputNext);
connect(m_gpioMonitor.get(), &IGpioMonitor::inputPrevious,
        m_receiverController.get(), &ReceiverController::inputPrevious);
connect(m_gpioMonitor.get(), &IGpioMonitor::inputSelect,
        m_receiverController.get(), &ReceiverController::toggleMute);

// AFTER:
connect(m_gpioMonitor.get(), &IGpioMonitor::inputNext,
        m_uiState.get(), &UIState::requestInputNext);
connect(m_gpioMonitor.get(), &IGpioMonitor::inputPrevious,
        m_uiState.get(), &UIState::requestInputPrevious);
connect(m_gpioMonitor.get(), &IGpioMonitor::inputSelect,
        m_uiState.get(), &UIState::requestInputSelect);
```

**QML consumer (in InputCarousel.qml):**
```qml
Connections {
    target: UIState
    function onInputNextRequested() {
        carouselOverlay.show()
        carousel.incrementCurrentIndex()
        root.focusInput(carousel.currentIndex)
    }
    function onInputPreviousRequested() {
        carouselOverlay.show()
        carousel.decrementCurrentIndex()
        root.focusInput(carousel.currentIndex)
    }
    function onInputSelectRequested() {
        if (!carouselOverlay.visible) {
            carouselOverlay.show()
        } else {
            root.selectImmediately(carousel.currentIndex)
            carouselOverlay.hide()
        }
    }
}
```

### Pattern 5: Static Current-Input Display (Left Panel)

**What:** A simple `Column` (icon + label) inside a `MouseArea` that fills the left panel. Reads from `ReceiverState.currentInput` via the existing `sourceToIndex()` mapping.

**When to use:** Replaces `InputCarousel { anchors.fill: parent }` at main.qml lines 299-302.

**Example:**
```qml
// Replace InputCarousel with:
MouseArea {
    anchors.fill: parent
    onClicked: carouselOverlay.show()

    Column {
        anchors.centerIn: parent
        spacing: Theme.spacingMedium

        // Large icon — adapt from existing 72px icon circle
        Rectangle {
            width: 96
            height: 96
            radius: 48
            color: Theme.accent
            border.color: Theme.accentLight
            border.width: 2
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                anchors.centerIn: parent
                text: currentInputIcon()   // function reads ReceiverState.currentInput
                font.pixelSize: 40
                color: Theme.textPrimary
            }
        }

        Text {
            text: currentInputLabel()
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMedium
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
```

### Anti-Patterns to Avoid

- **Setting `visible = false` before fade-out completes:** The overlay must stay `visible: true` until the `hideTimer` fires. Setting visible = false immediately skips the animation.
- **Using `Loader` for PathView delegates:** Loader adds async instantiation overhead. With only 6 delegates and `pathItemCount: 6`, all items are always in the scene — no Loader needed.
- **Calling `positionViewAtIndex()` on a PathView:** PathView does not have `positionViewAtIndex()`. Use `currentIndex =` assignment or `incrementCurrentIndex()` / `decrementCurrentIndex()`.
- **Connecting GPIO signals directly to a QML function:** Qt signal/slot connections from C++ to QML functions require Q_INVOKABLE or a slot. The bridge pattern (C++ emits UIState signal → QML Connections handler) is the correct approach.
- **Using `PathPercent` + `PathAttribute` for depth:** This is the "correct" PathView approach but adds complexity. Delegate-computed distance from `PathView.view.offset` achieves the same result with less boilerplate and is easier to tune.
- **Reusing the distFromCenter formula from the old ListView:** The existing `distFromCenter` in InputCarousel.qml uses `carousel.contentY` which is a ListView-only property. PathView uses `PathView.view.offset` instead.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Snap-to-item physics | Custom spring/momentum animation | `PathView.snapMode: PathView.SnapToItem` | Qt's snap physics matches platform feel; hand-rolled springs are notoriously hard to tune |
| Encoder lag mitigation | Debounce or batch timer | Direct `incrementCurrentIndex()` in signal handler | Signal handler fires synchronously on the Qt main thread — there is no queuing delay to eliminate |
| Show/hide state machine | Custom bool flags + conditional logic | `visible: false` + `opacity: 0` + `Behavior` + `Timer` | The established project overlay pattern handles all edge cases (animation interrupt, rapid show/hide) |
| PathView offset math | Custom wrapping logic | `PathView.view.offset` (Qt's own tracking) | Qt manages the fractional offset — reading it directly is always accurate |

**Key insight:** PathView's built-in snap, highlight range enforcement, and offset tracking handle all the hard physics. The implementation work is configuration and visual styling, not physics.

---

## Common Pitfalls

### Pitfall 1: visible vs opacity ordering in show()

**What goes wrong:** If `visible = true` is set after `opacity = 1.0`, the Behavior on opacity never fires because the item was not visible when the property changed.

**Why it happens:** Qt skips Behavior animations on invisible items to avoid invisible work.

**How to avoid:** Always set `visible = true` first, then change `opacity`. In hide(), set `opacity = 0` first, then the Timer sets `visible = false` after the animation duration.

**Warning signs:** Overlay appears instantly without fade-in.

### Pitfall 2: PathView wrapping artifacts with small model count

**What goes wrong:** With 6 items and a straight-line path, items wrap around — item 5 appears to the left of item 0 when navigating. This creates visual discontinuity.

**Why it happens:** PathView always wraps. With a short model and `pathItemCount` < model count, the wrap point is in the visible area.

**How to avoid:** Set `pathItemCount: sourceModel.count` (6). This places all items simultaneously, so the wrap position is invisible (there are no "missing" items to fill from the other side).

**Warning signs:** Item snaps from far right to far left during navigation.

### Pitfall 3: Encoder events arriving before carousel is visible

**What goes wrong:** The QML Connections handler fires on the first encoder tick, calling `carousel.incrementCurrentIndex()` before `carouselOverlay.visible = true` is processed, resulting in silent navigation with no visible change.

**Why it happens:** show() sets `visible = true` synchronously, but if the show() call and the incrementCurrentIndex() call happen in the same JS frame, QML may batch the property changes.

**How to avoid:** In the encoder signal handler, call show() first, then navigate. Since `visible = true` is a synchronous property write in Qt's property engine, the PathView is live by the time `incrementCurrentIndex()` executes.

**Warning signs:** Carousel sometimes misses the first encoder tick after appearing.

### Pitfall 4: Auto-select timer counting down while carousel is hidden

**What goes wrong:** If the timer is running when the carousel is hidden (e.g., dismissed by backdrop tap), the timer fires and calls `ReceiverController.selectInput()` unexpectedly after close.

**Why it happens:** `hide()` function does not stop the autoSelectTimer.

**How to avoid:** Call `autoSelectTimer.stop()` and `autoSelectProgress = 0.0` inside `hide()`.

**Warning signs:** Input changes unexpectedly a few seconds after closing the carousel.

### Pitfall 5: distFromCenter recalculation on every scroll frame

**What goes wrong:** The `distFromCenter` property binding in every delegate re-evaluates every time `PathView.view.offset` changes (which is every animation frame during scroll). With 6 delegates, this is 6 binding evaluations per frame.

**Why it happens:** QML property bindings are reactive — any binding dependency change triggers re-evaluation.

**How to avoid:** This is acceptable for 6 items on Pi 5 (well under the GPU's capability). No special optimization needed. Do NOT add `Behavior` on `distFromCenter` itself — only add Behavior on `scale` and `opacity`, which are what the GPU composites.

**Warning signs:** CPU spikes during carousel scroll (profile with Qt Quick Profiler if observed on device).

### Pitfall 6: The inputSelect mute side-effect remaining active

**What goes wrong:** If only the inputNext/inputPrevious rewiring is done but `inputSelect` → `ReceiverController::toggleMute` is left connected, pressing the encoder button in the new carousel will both toggle mute AND open/confirm the carousel.

**Why it happens:** AppBuilder.cpp line 88 connects `inputSelect` to `toggleMute` — this must be removed.

**How to avoid:** Remove all three old connections (lines 82-89) and replace with the three UIState bridge connections. Do not leave partial connections.

**Warning signs:** Mute state changes unexpectedly when pressing the input encoder button.

---

## Code Examples

### Current Left Panel Wiring (to be replaced — main.qml lines 284-303)
```qml
// Source: main.qml lines 284-303 (current code to remove)
Rectangle {
    id: leftPanel
    width: Theme.leftPanelWidth
    height: parent.height
    color: Theme.secondaryBg

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Theme.glassBorder
    }

    InputCarousel {
        anchors.fill: parent
    }
}
```

### Existing Auto-Select Timer Logic (reusable verbatim)
```qml
// Source: InputCarousel.qml lines 45-73 — carry forward unchanged
Timer {
    id: autoSelectTimer
    interval: 40  // ~25fps update for smooth ring animation
    repeat: true
    onTriggered: {
        autoSelectProgress += (40 / 4000)  // 4 seconds total
        if (autoSelectProgress >= 1.0) {
            autoSelectTimer.stop()
            autoSelectProgress = 1.0
            ReceiverController.selectInput(indexToSource(focusedIndex))
        }
    }
}

function focusInput(index) {
    if (index === sourceToIndex(ReceiverState.currentInput)) {
        autoSelectTimer.stop()
        autoSelectProgress = 0.0
        return
    }
    focusedIndex = index
    autoSelectProgress = 0.0
    autoSelectTimer.restart()
}
```

### PathView Auto-Select Pause on Drag
```qml
// Source: CONTEXT.md — onMovementStarted/onMovementEnded
PathView {
    id: carousel
    onMovementStarted: autoSelectTimer.stop()
    onMovementEnded: {
        // Restart only if there's a pending selection
        if (root.focusedIndex !== sourceToIndex(ReceiverState.currentInput)) {
            autoSelectProgress = 0.0
            autoSelectTimer.restart()
        }
    }
    onCurrentIndexChanged: root.focusInput(currentIndex)
}
```

### Backdrop MouseArea Pattern (established in main.qml dialogs)
```qml
// Source: main.qml eject dialog pattern (lines 667-669)
MouseArea {
    anchors.fill: parent
    onClicked: carouselOverlay.hide()  // dismiss without selecting
}
// PathView must be on top of this — use z ordering or place after
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| inputNext/inputPrevious bypass QML (direct ReceiverController call) | Bridge through UIState signals → QML | This phase | Carousel can intercept and show itself before changing input |
| Always-visible left-panel carousel | Hidden-by-default full-screen overlay | This phase | Cleaner idle state, more screen space for content |
| Vertical ListView carousel | Horizontal PathView overlay | This phase | Matches original v1 horizontal swipe feel |
| inputSelect = mute toggle | inputSelect = carousel open/confirm | This phase | Mute now only via touchscreen status bar button |

**Deprecated/outdated in this phase:**
- `carousel.positionViewAtIndex(idx, ListView.Center)` — ListView API, not valid on PathView. Replace with `carousel.currentIndex = idx`.
- `distFromCenter` using `carousel.contentY` — ListView-only. Replace with `PathView.view.offset`-based formula.
- `InputCarousel { anchors.fill: parent }` inside leftPanel — replaced by static current-input display.

---

## Open Questions

1. **Card sizing within 720px full-screen height**
   - What we know: Original used 225x282px cards. The overlay fills 1920x720. Available vertical space after no status bar interference is ~720px. Vertical center cards should be ~300-400px tall to feel substantial.
   - What's unclear: Exact card dimensions (marked as Claude's Discretion in CONTEXT.md).
   - Recommendation: Use 220px wide x 320px tall cards. Center them vertically in 720px (200px top/bottom padding). This gives a dominant center card that fills ~44% of screen height.

2. **PathView startX/endX values for 6 cards at 1920px width**
   - What we know: Cards are ~220px wide. Path must accommodate 6 cards. Center card at x=960.
   - What's unclear: Exact spacing between cards (marked as Claude's Discretion).
   - Recommendation: 280px card spacing (wider than card width to create visible gaps). Path: startX = 960 - 2.5*280 = 260, endX = 960 + 2.5*280 = 1660. All 6 cards fit with visible gaps.

3. **Whether to show current-input name in status bar**
   - What we know: Status bar currently shows: connection status, eject/search buttons, volume slider, mute, power, time.
   - What's unclear: Visual balance (marked as Claude's Discretion in CONTEXT.md).
   - Recommendation: Skip it. The left panel already shows the current input prominently. Adding it to the status bar would crowd the 1920px layout.

---

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | GoogleTest v1.17.0 + QSignalSpy (Qt 6) |
| Config file | tests/CMakeLists.txt (FetchContent) |
| Quick run command | `cd /Users/rory/Code/media-console-v2/build && ctest -R UIState --output-on-failure` |
| Full suite command | `cd /Users/rory/Code/media-console-v2/build && ctest --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CAR-01 | Smooth rotation, no jank on Pi 5 | manual-only | N/A — requires Pi 5 hardware + visual inspection | N/A |
| CAR-02 | Natural deceleration snap | manual-only | N/A — requires touch/encoder interaction + visual inspection | N/A |
| CAR-03 | Non-selected items de-emphasised (scale, opacity) | manual-only | N/A — visual verification on device | N/A |
| CAR-04 | Encoder drives carousel with no perceptible lag | manual-only | N/A — requires physical encoder + visual inspection | N/A |
| UIState bridge signals | inputNextRequested/inputPreviousRequested/inputSelectRequested emit on slot call | unit | `cd build && ctest -R UIState --output-on-failure` | ❌ Wave 0: add tests to tests/test_UIState.cpp |
| AppBuilder wiring | GPIO inputNext wired to UIState, not ReceiverController | unit | `cd build && ctest -R AppBuilder --output-on-failure` | ❌ Wave 0: add test to tests/test_AppBuilder.cpp |

**Note:** CAR-01 through CAR-04 are all visual/perceptual requirements. They have no automated test coverage possible from C++ unit tests. Verification is by manual inspection on the Pi 5 device.

### Sampling Rate
- **Per task commit:** `cd /Users/rory/Code/media-console-v2/build && ctest -R "UIState|AppBuilder" --output-on-failure`
- **Per wave merge:** `cd /Users/rory/Code/media-console-v2/build && ctest --output-on-failure`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `tests/test_UIState.cpp` — add tests for: `inputNextRequestedEmits`, `inputPreviousRequestedEmits`, `inputSelectRequestedEmits` (same pattern as existing `showToastSignalEmits` test at line 136)
- [ ] `tests/test_AppBuilder.cpp` — add test verifying that after build(), a fake IGpioMonitor::inputNext emission does NOT call ReceiverController::inputNext directly (requires either a spy approach or checking that UIState receives the signal)

*(The AppBuilder test is difficult to write cleanly because AppBuilder owns all objects. Mark as "best effort" — the UIState signal tests are the critical coverage.)*

---

## Sources

### Primary (HIGH confidence)
- Direct codebase read: `src/qml/components/InputCarousel.qml` — full existing implementation
- Direct codebase read: `src/qml/main.qml` — overlay patterns, left panel structure, z-layer assignments
- Direct codebase read: `src/state/UIState.h` + `UIState.cpp` — current signals, slot pattern
- Direct codebase read: `src/app/AppBuilder.cpp` — exact lines 82-89 to rewire
- Direct codebase read: `src/platform/IGpioMonitor.h` — signal names confirmed
- Direct codebase read: `src/qml/Theme.qml` — animMedium=300ms, animFast=150ms, leftPanelWidth=280px confirmed
- Direct codebase read: `.planning/phases/13-inputcarousel-polish/13-CONTEXT.md` — locked decisions

### Secondary (MEDIUM confidence)
- Qt 6 PathView documentation (general knowledge, verified against actual QML patterns in codebase): `PathView.SnapToItem`, `preferredHighlightBegin/End`, `incrementCurrentIndex()`, `pathItemCount`, `onMovementStarted/onMovementEnded`
- Qt 6 QObject signal pattern: Pure transient signals without backing property — well-established in this codebase (`showToast`, `restartRequested`)

### Tertiary (LOW confidence)
- PathView `PathView.view.offset` for distFromCenter formula — standard Qt Quick pattern but not verified against this project's specific Qt version. **Flag for validation:** test that `PathView.view.offset` is accessible in delegate context in the project's Qt build.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all libraries already in project, no new dependencies
- Architecture: HIGH — patterns directly observed in existing codebase files
- C++ signal bridge: HIGH — exact same pattern as `showToast` and `restartRequested` in UIState
- PathView specifics: MEDIUM — Qt 6 documented behavior, but `PathView.view.offset` in delegate context needs validation on first run
- Pitfalls: HIGH — derived from direct code reading (distFromCenter formula, visible/opacity ordering, timer cleanup)

**Research date:** 2026-03-04
**Valid until:** 2026-04-04 (stable Qt APIs, no fast-moving dependencies)
