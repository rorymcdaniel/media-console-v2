# Phase 7: GPIO Hardware - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Physical rotary encoders and reed switch drive the application — volume knob controls volume, input knob switches sources, door controls display. All GPIO monitoring via libgpiod v2 on /dev/gpiochip4. Linux-only with no-op stubs on other platforms.

</domain>

<decisions>
## Implementation Decisions

### Volume encoder feel
- +2 per detent on the 0-200 volume range (PEC11R-4020F-S0024, 24 detents, GPIO 27/22/23)
- No acceleration — constant +2 regardless of rotation speed
- Full range requires ~4.2 revolutions

### Input encoder behavior
- Wrap-around cycling through 6 sources (Streaming, Phono, CD, Computer, Bluetooth, Library)
- 1 detent = 1 source (PEC11R-4320F-S0012, 12 detents, GPIO 16/20/5)
- Encoder drives the UI carousel with 4-second auto-select timeout (matches Phase 10 InputCarousel)
- GPIO layer emits navigation signals; carousel handles timeout and receiver command

### Push button behavior (input encoder)
- Context-dependent: during carousel browsing → confirms source immediately (skips 4s timeout); when idle → toggles mute
- State awareness needed: GPIO signal wiring checks whether carousel is actively browsing
- Falling edge trigger (press, not release) with 250ms debounce
- Consistent physical feel across both behaviors

### Reed switch semantics
- Updates UIState.doorOpen property (Phase 9 reacts to state changes for display control)
- Reads pin value immediately on start() — no ambiguous initial state
- 500ms debounce window
- Polarity: magnets apart = door open (display on), magnets together = door closed (display off)

### Mute button edge detection
- Falling edge only (press) — fixes double-toggle bug (GPIO-06)
- 250ms debounce, same as input encoder push button
- Receiver handles mute feedback via eISCP state change — no extra GPIO-layer feedback

### Claude's Discretion
- Signal design: whether to keep separate volumeUp()/volumeDown() or change to single volumeChanged(int delta)
- Whether GPIO monitor emits raw detents (+1/-1) or pre-scaled values (+2/-2)
- Interface updates to IGpioMonitor and StubGpioMonitor as needed
- Quadrature decoding implementation details
- Background thread architecture for libgpiod v2 event monitoring
- Error handling when libgpiod fails to open chip

</decisions>

<specifics>
## Specific Ideas

- Input encoder should feel like browsing a carousel — rotation previews, pause confirms. Push button is a shortcut to skip the wait.
- Mute and source-confirm share the same physical button but context determines behavior. This means the GPIO layer needs awareness of UI state (is carousel browsing active?), or the wiring layer handles the routing.

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `IGpioMonitor` (src/platform/IGpioMonitor.h): Interface with all needed signals (volumeUp/Down, muteToggled, inputNext/Previous/Select, reedSwitchChanged)
- `StubGpioMonitor` (src/platform/stubs/StubGpioMonitor.cpp): Complete stub with simulate methods for non-Linux testing
- `VolumeGestureController` (src/receiver/VolumeGestureController.h): Already handles gesture coalescing with 300ms timeout, accepts onEncoderTick(int delta)
- `CommandSource` enum (src/state/CommandSource.h): Local/External/API for tagging volume changes
- `PlatformFactory::createGpioMonitor()` (src/platform/PlatformFactory.h): Factory method ready

### Established Patterns
- Platform abstraction: interface + stub + real implementation, selected by PlatformFactory::isLinux()
- State objects: thin Q_PROPERTY bags (ReceiverState, PlaybackState, UIState) — reed switch should add doorOpen to UIState
- Background threads: audio pipeline uses QThread with atomic flags for control — GPIO monitors should follow similar pattern
- Logging: mediaGpio category already defined

### Integration Points
- AppBuilder (src/app/AppBuilder.cpp): Already creates and owns m_gpioMonitor, wired into AppContext
- VolumeGestureController: connect GPIO volume signals → onEncoderTick(int delta) with +2 multiplier
- ReceiverController: connect mute signal → toggleMute command
- UIState: needs new doorOpen Q_PROPERTY for reed switch state
- Phase 10 InputCarousel: GPIO input signals will drive carousel navigation (future wiring)

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 07-gpio-hardware*
*Context gathered: 2026-02-28*
