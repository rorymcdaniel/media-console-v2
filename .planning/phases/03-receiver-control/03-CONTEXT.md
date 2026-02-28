# Phase 3: Receiver Control - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

eISCP TCP connection to Onkyo TX-8260 receiver with full control over power, volume, input, mute, and metadata. Includes VolumeGestureController (ORCH-03) for smooth encoder-driven volume gestures with optimistic UI. GPIO hardware input and QML UI components are separate phases.

</domain>

<decisions>
## Implementation Decisions

### Connection resilience
- Connect immediately on startup, retry forever with backoff
- Error banner (persistent, top of screen) when receiver is disconnected — aligns with UI-15 ErrorBanner
- Preserve last-known ReceiverState during disconnection — do not clear to defaults
- On reconnect, silently refresh all state from receiver

### Volume gesture tuning
- VolumeGestureController built in Phase 3 (ORCH-03), not deferred to Phase 7
- Volume display: decimal 0.0–100.0 (full precision from 0-200 receiver range)
- Reconciliation after gesture: snap UI to receiver-reported value immediately (no animation)
- Command source tagging infrastructure built now: Local, External, API enum — ready for GPIO (Phase 7) and HTTP API (Phase 9)
- Volume overlay shows only for Local source; External changes update state silently

### Metadata transitions
- Clear all metadata fields immediately on input switch — no stale data from previous source
- Progressive display: show each eISCP fragment (NTI, NAT, NAL, NJA2) as it arrives — don't buffer
- Non-streaming inputs (Phono, CD, Computer, Bluetooth): show source name as title placeholder
- Set both streamingService enum AND serviceName string on service detection — QML binds directly to string

### Stale data & error handling
- 30-second stale data warning (RECV-12): subtle indicator in status bar, not toast — clears when data resumes
- Command failures (rejected input switch, etc.): revert optimistic state silently, no notification
- Receiver power-off connection behavior: Claude's discretion based on actual eISCP protocol behavior

### Claude's Discretion
- Reconnection backoff strategy (exponential vs fixed, timing)
- Volume gesture timeout duration (when gesture is considered "ended")
- Connection behavior when receiver powers off (stay connected vs disconnect)
- eISCP protocol implementation details (framing, checksums, command queuing)

</decisions>

<specifics>
## Specific Ideas

- Kiosk is always-on — connection must be persistent and self-healing
- Volume overlay source distinction is important: user-initiated changes show overlay, external changes (Spotify Connect, Onkyo app) update silently
- Decimal volume display (42.5 not 43) — full precision matters

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ReceiverState` (src/state/ReceiverState.h): Q_PROPERTY bag with all receiver properties — volume, power, muted, currentInput, title, artist, album, albumArtUrl, fileInfo, serviceName, streamingService. All setters emit change signals. Volume is int (0-200).
- `MediaSource` enum (src/state/MediaSource.h): 6 inputs with `toHexCode()`/`fromHexCode()` conversion. CD and Library share hex code 0x23 — disambiguated by context.
- `StreamingService` enum (src/state/StreamingService.h): Spotify, Pandora, AirPlay, AmazonMusic, Chromecast.
- `PlaybackMode` enum (src/state/PlaybackMode.h): Stopped, Playing, Paused.
- `AppConfig::ReceiverConfig` (src/app/AppConfig.h): host ("192.168.68.63") and port (60128) for TCP connection.
- `AppContext` (src/app/AppContext.h): Holds ReceiverState pointer — new receiver controller wires into this.

### Established Patterns
- Q_PROPERTY bags as thin reactive state — no business logic in state objects
- QML singleton registration via qmlRegisterSingletonInstance()
- Platform abstraction via interfaces (IAudioOutput, ICdDrive, etc.) with stub implementations
- Composition root: AppBuilder constructs object graph, AppContext provides convenience pointers
- Logging categories: media.receiver already defined

### Integration Points
- AppBuilder: new ReceiverController constructed here, wired to ReceiverState via signals/slots
- AppConfig: ReceiverConfig provides host/port for TCP connection
- ReceiverState: controller sets properties, QML observes via bindings
- UIState: error state properties available for connection status indicators
- VolumeGestureController: new class, constructed by AppBuilder, sits between encoder input and receiver commands

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 03-receiver-control*
*Context gathered: 2026-02-28*
