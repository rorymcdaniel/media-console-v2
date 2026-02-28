# Phase 8: Spotify Integration - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can authenticate with Spotify, search for music, transfer playback to the receiver, and manage active sessions. This phase creates the `src/spotify/` module and integrates it into the existing composition root (AppBuilder), state layer, and QML UI. Creating new source types, modifying receiver protocol handling, or building companion apps are out of scope.

</domain>

<decisions>
## Implementation Decisions

### Authentication flow
- External setup only — no login screen on the kiosk touchscreen
- CLI command on the Pi (e.g., `media-console --spotify-auth`) prints a URL
- User opens URL on any browser, authorizes, Spotify redirects back to Pi's localhost
- Tokens persisted and auto-refreshed (5 min pre-expiry per SPOT-01)
- When Spotify tokens are missing or expired beyond recovery: Spotify appears in source list but grayed out, tapping shows toast "Spotify not configured — run setup command"

### Search and browse UX
- Qt VirtualKeyboard module for touchscreen text input
- Mixed results list — single scrollable view with grouped sections (artists, albums, tracks)
- All result rows show album art thumbnails inline
- Tapping a track starts playback immediately (transfers to receiver)
- Tapping an album or artist drills into a detail view showing their tracks
- Suggested/featured playlists appear as default content in search view before user types; replaced by search results when typing

### Playback control placement
- Spotify transport controls (play/pause/next/prev) integrated into existing NowPlaying view when active source is Spotify
- Touch-only transport buttons — physical knobs continue to control volume and input switching as before
- Search icon visible in NowPlaying when source is Spotify; tapping opens SpotifySearch view; back button returns to NowPlaying

### Session takeover behavior
- Always confirm before transferring: dialog shows device name, track title, and artist
- Two actions: "Transfer Here" / "Cancel" — simple confirm/cancel
- Active session check happens only on source selection (no background polling)
- If playback is interrupted mid-session (account used elsewhere): toast notification "Spotify playback interrupted", NowPlaying updates to stopped state, user can manually re-engage

### Claude's Discretion
- Token storage mechanism (QSettings vs separate file)
- Loading states and skeleton designs
- Search debounce timing
- Album/artist detail view layout specifics
- Error handling for API failures
- Exact control button sizing and positioning in NowPlaying
- Playback position polling frequency when Spotify is active

</decisions>

<specifics>
## Specific Ideas

No specific references — open to standard approaches. Key constraint is the 1920x720 touchscreen form factor and kiosk context (walk-up usability, no keyboard).

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `SpotifyConfig` (AppConfig.h): Already has clientId, clientSecret, desiredDeviceName ("Voice of Music")
- `StreamingService::Spotify` enum: ReceiverController already detects Spotify as a streaming service
- `ActiveView::SpotifySearch`: Already defined in ActiveView enum — UI routing ready
- `PlaybackState`: Full metadata properties (title, artist, album, albumArtUrl, position, duration) — Spotify can write directly to this
- `UIState::showToast()`: Toast signal for error notifications (lost session, unconfigured state)
- `media.spotify` logging category: Already defined in Logging.h/cpp

### Established Patterns
- Composition root: AppBuilder constructs full object graph — SpotifyController will be added here
- State layer: Thin Q_PROPERTY bags (ReceiverState, PlaybackState, UIState) registered as QML singletons
- Controller pattern: Business logic in controllers (ReceiverController, CdController), state in state objects
- Platform abstraction: PlatformFactory for hardware-dependent implementations
- Config: All config loaded once from QSettings INI at startup via AppConfig::loadFromSettings()

### Integration Points
- `AppBuilder::build()` — Add SpotifyController construction and signal wiring
- `AppConfig::loadFromSettings()` — SpotifyConfig already loaded
- `PlaybackState` — SpotifyController writes track metadata here when Spotify is active
- `UIState::activeView` — Switch to SpotifySearch view
- `ReceiverController::selectInput()` — Switch receiver to Streaming input for Spotify playback
- QML main.qml — Add NowPlaying and SpotifySearch views (currently just a test harness)

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 08-spotify-integration*
*Context gathered: 2026-02-28*
