# Phase 9: Display, HTTP API, and Orchestration - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Three subsystems: (1) DDC/CI display control with a screen timeout state machine, (2) REST API for remote control and Spotify OAuth setup, (3) PlaybackRouter and AlbumArtResolver to eliminate routing duplication. This phase delivers backend infrastructure — QML UI integration happens in Phase 10.

</domain>

<decisions>
## Implementation Decisions

### Screen timeout behavior
- State machine: ACTIVE → DIMMED (5 min default) → OFF (20 min default) with configurable timeouts and dim brightness (default 25%)
- Activity events that reset to ACTIVE: touch events, door open (reed switch), encoder turns/presses, and incoming HTTP API calls
- All active playback sources suppress the timeout — screen stays on while any music is playing regardless of source
- Door open: instant wake to full brightness (ACTIVE state) — someone is physically present
- Door close: immediate transition to DIMMED then OFF — skip the 5-min wait, user is done at the console
- DIMMED state: reduce backlight brightness via DDC/CI, UI content stays visible (no QML fade-to-black)
- LinuxDisplayControl implements IDisplayControl using `ddcutil` subprocess calls — the interface already exists from Phase 1
- Smooth dimming transitions via timed brightness steps (DISP-02)

### HTTP API endpoints and access
- QHttpServer on configurable port (default 8080) — lift pattern from reference HttpApiServer
- Core endpoints matching reference: POST /api/volume, POST /api/input, POST /api/display, GET /api/status
- Spotify endpoints matching reference: GET /setup/spotify (HTML auth page), GET /auth/spotify/callback, GET /spotify/status, POST /spotify/play
- Add playback control endpoints: POST /api/playback/play, POST /api/playback/pause, POST /api/playback/next, POST /api/playback/previous — these go through PlaybackRouter
- Optional HTTPS via auto-generated self-signed cert (API-05) — same openssl approach as reference
- No authentication required — API is local network only (behind router firewall)
- No CORS headers needed — API is called from scripts/curl, not browser JS

### PlaybackRouter dispatch model
- PlaybackRouter owns a map of MediaSource → controller (CdController, FlacLibraryController, SpotifyController)
- Active source tracked in ReceiverState::activeSource — PlaybackRouter reads this to dispatch
- Unified Q_INVOKABLE methods: play(), pause(), stop(), next(), previous(), seek(int ms) — dispatches to correct controller
- When a new source activates while another is playing: the new source takes over, old source's controller gets stop() called
- CD and Library share receiver input "02" (S/PDIF via Nvdigi HAT) — PlaybackRouter handles the ambiguity by checking which local controller is actually playing
- Streaming/Bluetooth/Phono: PlaybackRouter sends receiver input switch via ReceiverController, no local audio controller involved
- Spotify: PlaybackRouter calls SpotifyController.activateSpotify() which handles device transfer

### AlbumArtResolver source rules
- Streaming sources (Spotify, Bluetooth): album art from receiver CGI endpoint (NJA2 parsing already in ReceiverController)
- CD: local art from CdAlbumArtProvider (already built in Phase 5)
- Library: local art from LibraryAlbumArtProvider (already built in Phase 6)
- Phono/None: no album art — return empty/default
- AlbumArtResolver exposes a single Q_PROPERTY albumArtUrl that QML binds to — internally selects source based on active MediaSource
- Fallback: generic placeholder icon when art is unavailable from any source

### Claude's Discretion
- DDC/CI bus auto-detection strategy (DISP-03)
- Exact dimming step timing for smooth transitions
- HTTP API error response format (JSON structure)
- AlbumArtResolver polling/signal strategy for art updates
- ScreenTimeoutController timer implementation details

</decisions>

<specifics>
## Specific Ideas

- Reference implementation HttpApiServer.cpp (~/Code/media-console/src/api/) is the blueprint for route setup, SSL, and Spotify OAuth pages
- The Spotify OAuth HTML pages in the reference (green Connect button, success/error screens) should be lifted directly — they work well
- ORCH-03 (VolumeGestureController) is already complete from Phase 3 — don't duplicate

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `IDisplayControl` (src/platform/IDisplayControl.h): Interface ready — autoDetectDisplay, setPower, setBrightness, brightness, isPowered + signals
- `StubDisplayControl` (src/platform/stubs/StubDisplayControl.h): Dev/test stub already built
- `PlatformFactory`: Already creates display control via runtime injection — just needs LinuxDisplayControl added
- `CdAlbumArtProvider` and `LibraryAlbumArtProvider`: Local art providers already built
- `SpotifyController.activateSpotify()`: Spotify session takeover ready
- `ReceiverController`: Already parses NJA2 for receiver album art URL

### Established Patterns
- Platform abstraction: `IDisplayControl` → `StubDisplayControl` / `LinuxDisplayControl` via `PlatformFactory`
- Controller pattern: Each subsystem has a controller (CdController, FlacLibraryController, SpotifyController) that PlaybackRouter can delegate to
- AppBuilder composition root: All new objects constructed in AppBuilder, exposed via AppContext
- Signal-based communication between controllers and state objects

### Integration Points
- `AppContext` already has `IDisplayControl* displayControl` — ScreenTimeoutController reads this
- `AppContext` needs: PlaybackRouter*, AlbumArtResolver*, HttpApiServer*
- `AppBuilder` constructs and wires all new objects
- `ReceiverState::activeSource` drives PlaybackRouter dispatch
- `UIState` already exists — add screen timeout state (dimmed, off) for QML binding in Phase 10

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 09-display-http-api-and-orchestration*
*Context gathered: 2026-02-28*
