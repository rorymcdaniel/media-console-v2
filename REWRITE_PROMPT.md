# Media Console v2 — GSD New Project Prompt

Paste everything below this line into Claude Code after running `/gsd:new-project` in the new empty repo.

---

## Project Description

This is a ground-up rewrite of an existing, working Qt6/QML music console application. The original codebase (~15,400 lines C++, ~5,200 lines QML) runs on a Raspberry Pi 5 controlling an Onkyo TX-8260 receiver via a 1920x720 touchscreen with physical rotary encoders and an integrated CD player. The original repo is located at `~/Code/media-console` and should be used as the reference implementation throughout this project.

The goal is to deliver the same core functionality with a cleaner architecture that eliminates the structural problems that accumulated during organic development. This is NOT a feature expansion — it is a structural rewrite. Some features from the original have been explicitly deferred or removed (see "Deferred / Removed Features" section).

## Reference Implementation

**Original codebase location: `~/Code/media-console`**

This is the working production codebase that defines correct behavior. Agents MUST reference it when:
- Tracing execution paths to understand existing behavior
- Lifting domain logic (eISCP parsing, ALSA config, CD audio streaming, metadata fetching, GPIO monitors)
- Verifying feature parity between old and new implementations
- Understanding non-obvious behavior or edge cases

Key files in the original repo:
- `src/AppState.{h,cpp}` — the god object being decomposed (understand what it does before splitting it)
- `src/controllers/EiscpReceiverController.{h,cpp}` — eISCP protocol implementation to lift
- `src/playback/CdPlaybackController.{h,cpp}` — CD playback architecture to learn from
- `src/playback/FlacPlaybackController.{h,cpp}` — FLAC playback (structurally identical to CD, motivating unification)
- `src/audio/CdAudioStream.{h,cpp}` — buffering and paranoia read logic to lift
- `src/audio/FlacAudioStream.{h,cpp}` — FLAC decode and resample logic to lift
- `src/audio/AlsaOutput.{h,cpp}` — ALSA PCM config to lift (hard-won parameters)
- `src/utils/CDMetadataFetcher.{h,cpp}` — three-tier lookup chain to lift
- `src/utils/SpotifyAuthManager.{h,cpp}` — OAuth flow to lift
- `src/main.cpp` — 325-line wiring to understand before building composition root
- `qml/` — all QML components to migrate with updated bindings

## Target Hardware

- Raspberry Pi 5 (4GB RAM)
- Raspberry Pi OS (Debian 13 Trixie, 64-bit)
- 1920x720 touchscreen with DDC/CI power control
- Onkyo TX-8260 receiver (eISCP network control on TCP port 60128)
- Nvdigi HAT for S/PDIF digital audio output
- External USB slot-loading CD drive (via powered USB hub)
- Volume rotary encoder: PEC11R-4020F-S0024 — 24 pulses/revolution, ZERO detents (smooth-turning, no click stops, continuous event stream during rotation)
- Input rotary encoder: PEC11R-4320F-S0012 — 12 pulses/revolution, 12 detents (clicky, one discrete event per detent, simple 1:1 mapping to input changes)
- Magnetic reed switch for door/display auto-control
- Technics SL-5 turntable (phono input, no digital control)

## Technology Stack (unchanged from original)

- C++17, Qt6 (Core, Gui, Quick, Network, HttpServer, Concurrent, Sql)
- QML for UI with declarative components
- CMake with Ninja
- QSettings (INI-style config at `~/.config/MediaConsole/media-console.conf`)
- Linux-only native deps: libgpiod, libcdio, libcdio-paranoia, libcdio_cdda, libdiscid, libasound (ALSA), TagLib, libsndfile, libsamplerate

## Complete Feature Set to Preserve

### 1. Receiver Control (eISCP Protocol)
- Direct TCP implementation of Onkyo eISCP protocol
- **RESEARCH ITEM**: The Onkyo TX-8260 also has a telnet server. This should be explored during the research phase as a potential event-driven alternative or supplement to eISCP polling. A telnet event stream could enable: distinguishing user-initiated vs. external volume changes (solving the overlay problem), eliminating the 2.5s polling interval, and detecting receiver-initiated state changes in real-time. The current poll-based approach loses context about who initiated a change.
- Volume control: 0-200 range, displayed as 0.0-100.0 with decimal precision
- Volume control for smooth (detentless) encoder: the PEC11R-4020F-S0024 has no detents, generating a continuous stream of events during rotation. The original used a 100ms throttle timer which was insufficient — volume would "fight" and jump back during rapid adjustment. The rewrite should treat encoder input as a continuous gesture, coalescing all events until the user stops turning, then sending a single absolute volume command and reconciling with receiver state only after the gesture ends.
- Optimistic UI updates: volume display updates immediately, command sent after gesture completes
- 6 input sources: Streaming/NET (2B), Phono (22), CD (02), Computer (05), Bluetooth (2E), Library (02 — shares hardware input with CD)
- Power on/off, mute toggle/on/off
- State polling every 2.5s with auto-reconnect on network errors
- Track metadata parsing from eISCP: NTM (time), NJA2 (album art URL), NFI (file info), NMS (service detection), NTI (title), NAT (artist), NAL (album)
- Streaming service detection: Spotify, Pandora, AirPlay, Amazon Music, Chromecast
- Playback state tracking: playing, paused, stopped via NST command
- Stale data detection: warns after 30s without receiver updates during active playback

### 2. CD Playback (Native ALSA/libcdio)
- Direct CD audio streaming via libcdio with paranoia error correction
- ALSA PCM output to S/PDIF device (hw:2,0)
- Background thread playback loop with atomic flag control (stop, pause, pending track, pending seek)
- Intelligent audio buffering: 8-second buffer, 1-second prefill target, max 3 retries with 50ms backoff
- Buffer statistics: xrun tracking, per-read latency (avg/max microseconds), read error counting
- Full playback control: play, pause, stop, next, previous, seek (sector-based)
- TOC reading via CdDrive (libcdio wrapper)
- Idle timer: stops CD spindle after period of inactivity
- Spin-up timer: handles drive spin-up delay on play
- EIO recovery: attempts recovery on ALSA I/O errors, emits audioRecoveryFailed when exhausted

### 3. CD Metadata System
- Three-tier async lookup chain: MusicBrainz (via libdiscid) → GnuDB (CDDB protocol) → Discogs (REST API)
- SQLite metadata cache: instant lookup for previously-seen discs by disc ID
- Album art downloading from CoverArtArchive and Discogs, cached to disk
- Front and back cover support
- Graceful degradation: falls back to generic "Audio CD" metadata if all sources fail
- GnuDB provides artist/album hints used by Discogs search when MusicBrainz has no results
- Cache eviction: poisoned GnuDB error text is detected and evicted on load

### 4. CD Monitoring
- Hybrid disc presence detection: QFileSystemWatcher + ioctl polling
- Configurable: device path, poll interval, audio-only filter
- Debounced state changes
- CD playback is ALWAYS user-initiated — no auto-play feature (see "Deferred / Removed Features")
- Signals: cdInserted(isAudioCD), cdRemoved

### 5. FLAC Library System
- Recursive directory scanner using TagLib for metadata extraction
- SQLite database indexing: title, artist, album artist, album, track/disc number, year, genre, duration, sample rate, bit depth, file path, modification time
- Incremental scanning: skips unchanged files based on mtime
- Album art extraction from FLAC picture blocks, cached via SHA-1(artist+album) filename
- MIME type auto-detection (JPEG/PNG) for cached art
- Async scanning via QtConcurrent
- Three hierarchical QAbstractListModel subclasses for QML browse UI:
  - LibraryArtistModel: all artists with album counts
  - LibraryAlbumModel: albums for selected artist (filtered by artist property)
  - LibraryTrackBrowseModel: tracks for artist+album with full metadata
- FLAC playback via libsndfile decoding + libsamplerate resampling to 44100Hz/16-bit/stereo
- Playlist-based playback with next/previous navigation
- Playlist can carry associated LibraryTrack metadata for rich display

### 6. Spotify Integration
- OAuth 2.0 Authorization Code flow with token persistence and auto-refresh
- Device discovery and playback transfer by device name
- Playback control: play, pause, next, previous, play by URI, context-aware playback
- Full-text search via Spotify Web API (tracks, artists, albums)
- Queue management: add tracks to queue
- Album art: use receiver-provided art for all streaming sources (see "Deferred / Removed Features")
- Suggested playlists: featured + user recommendations
- Active session detection: warns when another device is playing, offers takeover dialog

### 7. Bluetooth Input
- Display whatever metadata the receiver provides via eISCP (title, artist, album)
- Show music note placeholder for album art (no external art fetching — see "Deferred / Removed Features")
- Seldom-used input; functional completeness is sufficient, no special treatment needed

### 8. GPIO Hardware Integration
- Volume rotary encoder (PEC11R-4020F-S0024, smooth/detentless): GPIO 27 (A), 22 (B), 23 (switch) — quadrature decoding, requires gesture-based coalescing due to continuous event stream
  - **KNOWN BUG: mute button double-toggles.** Pressing the push switch toggles mute on then immediately back off. Mute via touchscreen works perfectly — this is purely a GPIO input handling issue. Most likely cause: the code acts on both press AND release edges, so press→mute on, release→mute off. The rewrite must trigger mute toggle on ONE edge only (falling or rising, not both). Verify by checking libgpiod event types.
- Input rotary encoder (PEC11R-4320F-S0012, 12 detents): GPIO 16 (A), 20 (B), 5 (switch) — quadrature decoding, 250ms switch debounce, one detent = one input change (simple 1:1 mapping)
- Reed switch: GPIO 17 — magnets apart = display on, together = off, 500ms debounce
- All monitors run in background threads via libgpiod event monitoring
- Linux-only with no-op stubs on other platforms

### 9. Display Control
- DDC/CI power and brightness control via ddcutil subprocess
- Smooth dimming transitions, fade to black
- Auto-detection of display bus
- Screen timeout state machine: ACTIVE → DIMMED → OFF (also DOOR_CLOSED state from reed switch)
- Configurable dim timeout (default 5 min), off timeout (default 20 min), dim brightness (default 25%)
- Activity-based reset (touch events reset timeout)
- Playback-aware: disables timeout during active music playback
- Door sensor integration via reed switch

### 10. HTTP API Server (Port 8080)
- REST API via Qt HttpServer
- Endpoints: volume set, input switch, status query, display power, Spotify OAuth setup page, OAuth callback, Spotify search
- Optional HTTPS via auto-generated self-signed certificate (when QT_SSL_AVAILABLE)
- Serves Spotify redirect URI dynamically based on server URL

### 11. Lidarr / SpotiFLAC Bridge Integration
- Forwards Spotify track notifications to external SpotiFLAC Bridge via HTTP POST to /api/notify
- Track buffering: accumulates title/artist/album from async receiver messages, emits when complete
- 2-second buffer timeout for incomplete data (receiver sends metadata fields separately with timing gaps)
- Deduplication: suppresses repeated emissions for same artist+album, clears every 10 minutes
- Only active when [SpotiFLACBridge]/url is configured

### 12. QML UI (1920x720)
- Deep blue color theme (#0a1628 primary, #162844 secondary, #2e5c8a accent)
- Theme.qml singleton with design tokens: colors, typography (6 sizes 12-48px), spacing (8/16/24px), touch targets (44/64px), animation durations (150/300/500ms)
- Main layout: left panel (input icon + service label), right panel (NowPlaying or LibraryBrowser), top status bar
- NowPlaying: album art (front/back carousel), track info, progress bar with seek, playback controls
- InputCarousel: 3D perspective carousel with 6 inputs, 4-second auto-select timeout, encoder-driven navigation
- LibraryBrowser: StackView drill-down (Artists → Albums → Tracks), artist A-Z quick scroll sidebar, split layout on track page (album art left, track list right)
- SpotifySearch: fullscreen overlay with on-screen QWERTY keyboard (SimpleKeyboard), search results with album art
- SpotifyTakeoverDialog: modal confirmation for session transfer showing current device and track
- AudioErrorDialog: modal for ALSA recovery failures with retry option
- ToastNotification: bottom-center 3-second auto-dismiss (info/success/error types)
- VolumeOverlay: large modal with numeric display, auto-dismiss after 2s
- VolumeIndicator: persistent top-right display with draggable slider
- EjectButton: visible only when CD present
- SearchButton: visible only when on Spotify input
- ErrorBanner: shown when receiver disconnected
- TimeDisplay: current time, updates every minute
- PowerButton, MuteButton with visual state indicators
- Global MouseArea for touch activity detection (resets screen timeout)

### 13. Logging
- 8 Qt logging categories: media.app, media.spotify, media.receiver, media.audio, media.http, media.lidarr, media.gpio, media.cd
- Configurable via QSettings (logging/level, logging/rules) or QT_LOGGING_RULES env var
- Default: Info level, debug suppressed
- High-frequency events (position updates, polling, search typing) are Debug level

### 14. Configuration (QSettings INI)
- [receiver] host, port
- [cd] device, poll_interval_ms, audio_only, metadata_on_insert
- [display] id, timeout_to_dim_seconds, timeout_to_off_seconds, dim_brightness, timeout_enabled
- [Audio] Device, SampleRate, Channels, PeriodSizeFrames, BufferSizeFrames
- [spotify] client_id, client_secret, desired_device_name
- [discogs] api_key, api_secret
- [SpotiFLACBridge] url
- [api] port
- [logging] level, rules
- [library] root
- Note: service name caching ([streamingService]/lastKnown) is NOT being reimplemented initially — display whatever the receiver reports (see "Deferred / Removed Features")

### 15. Production Deployment
- Kiosk mode via systemd service with auto-start, auto-restart on crash
- Auto-login, hidden cursor (unclutter), disabled screen blanking
- install-kiosk.sh / uninstall-kiosk.sh scripts

## Architectural Decisions for the Rewrite

These decisions have been made and should NOT be revisited:

### 1. Decompose AppState into focused objects
The original AppState is a 310-line header / 1,100-line implementation doing 7 jobs: receiver state proxying, playback command routing, album art strategy, Lidarr track buffering, service name caching, UI overlay state, and configuration access. It must be split into:
- **Thin reactive AppState** — Q_PROPERTY bag for QML binding only, no business logic
- **PlaybackRouter** — owns input→controller routing (eliminates the same if/else chain duplicated 7 times in play/pause/playPause/stop/next/previous/seek)
- **AlbumArtResolver** — simplified: receiver CGI art for all streaming sources, local cached art for CD and Library. No Spotify CDN fetching, no Bluetooth Discogs fetching, no cache-busting timestamps (add later only if stale art becomes a real problem)
- **LidarrTrackBuffer** — deferred to late in project (see "Deferred / Removed Features"), but when implemented should be its own class, not embedded in AppState

### 2. Separate "media source" from "receiver input"
The original conflates what the user wants to listen to (Spotify, Phono, CD, Library) with what physical receiver input to select (eISCP hex codes). CD and Library share hardware input "02", requiring an ActiveSource enum as disambiguation. In the rewrite:
- Define a `MediaSource` enum for user-facing sources
- Keep receiver input mapping (source → eISCP hex code) inside the eISCP implementation only
- Eliminate the ActiveSource disambiguation layer

### 3. Unify playback controllers into a single LocalPlaybackController
CdPlaybackController and FlacPlaybackController are structurally identical: both own an audio stream + AlsaOutput, run a playback loop in std::thread, use the same atomic flag pattern. Create:
- An `AudioStream` interface (Open, ReadFrames, TotalFrames, Close, etc.)
- A single `LocalPlaybackController` parameterized by AudioStream
- CdAudioStream and FlacAudioStream as AudioStream implementations
- This also naturally solves ALSA device exclusivity (one controller, one device)

### 4. Clean up AudioController interface
Remove vestigial methods from the MPD era:
- `connectToAudioDaemon(host, port)` — neither CD nor FLAC uses network params
- `updateLibrary()` — MPD concept, not applicable to local playback
Add `toggleMute()` to ReceiverController base interface (currently requires downcast to EiscpReceiverController).

### 5. Restructure source tree by domain
```
src/
├── app/            # AppState (thin), AppBuilder/composition root, AppConfig
├── audio/          # AlsaOutput, AudioStream interface, CdAudioStream, FlacAudioStream
├── playback/       # LocalPlaybackController, PlaybackRouter
├── receiver/       # ReceiverController interface, EiscpReceiverController
├── cd/             # CdDrive, CDMonitor, CDMetadataFetcher, CDMetadataCache
├── display/        # DisplayControl, ScreenTimeoutManager
├── gpio/           # ReedSwitchMonitor, VolumeEncoderMonitor, InputEncoderMonitor
├── library/        # LibraryDatabase, LibraryScanner, browse models, AlbumArtCache
├── metadata/       # MetadataParsers, AlbumArtResolver
├── spotify/        # SpotifyAuthManager, SpotifySettings
├── lidarr/         # LidarrController, LidarrTrackBuffer, LidarrConfig
├── api/            # HttpApiServer
├── config/         # AppConfig (typed config struct, loaded once at startup)
└── logging/        # Logging categories and initialization
```

### 6. Introduce a composition root
Replace the 325-line main.cpp wiring with an AppBuilder/AppContext that:
- Reads all configuration once into a typed AppConfig struct
- Constructs the object graph with clear ownership
- Wires signals/slots by subsystem
- Makes it possible to construct test harnesses with mock components

### 7. Centralize configuration
Replace scattered QSettings reads (currently in main.cpp, AppState, SpotifyAuthManager, CDMetadataFetcher, LidarrConfig, etc.) with a single AppConfig struct loaded at startup and passed by const reference.

### 8. Platform abstraction via runtime injection
Replace `#ifdef __linux__` compile-time guards with runtime interface injection. A PlatformFactory returns real or stub implementations. This enables meaningful integration testing on macOS development machines.

### 9. Consistent naming
Standardize on Qt conventions: camelCase for methods, PascalCase for classes. Eliminate mixed PascalCase methods (PlaybackLoop, SectorToSeconds) inherited from C-style API wrapping.

## Code Reuse Strategy

The following should be lifted from the original with minimal changes:
- eISCP protocol parsing and packet construction
- CdAudioStream buffering and paranoia read logic
- FlacAudioStream decoding and resampling
- AlsaOutput PCM configuration and write loop
- CDMetadataFetcher lookup chain (MusicBrainz, GnuDB, Discogs) — but with response validation before caching, and fully async with no main-thread blocking
- CDMetadataCache SQLite schema and operations
- GPIO monitor implementations (ReedSwitch, VolumeEncoder, InputEncoder)
- LibraryDatabase SQLite schema and queries
- LibraryScanner TagLib extraction logic
- All QML components (with updated property bindings)
- Theme.qml, icons, assets
- Kiosk mode scripts

The following are NOT carried over:
- StreamingAlbumArtFetcher (removed)
- Spotify album art CDN fetching (deferred)
- SpotifySettings legacy migration (removed)
- CD auto-play logic (removed)
- Service name caching (deferred)
- Lidarr integration (deferred)

The following must be rewritten:
- AppState (decomposed into thin state + focused objects)
- main.cpp (replaced by composition root)
- All signal/slot wiring (reflects new module boundaries)
- Playback controllers (unified into single parameterized controller)
- AudioController interface (cleaned up)
- ReceiverController interface (source/input separation, add toggleMute)

## Deferred / Removed Features

These features are explicitly NOT part of the initial rewrite. They may be added later as separate phases:

### Removed (do not implement)
- **CD auto-play**: CD playback is always user-initiated. No auto-play on insert, no auto-play on startup. No configuration option for it.
- **Bluetooth album art fetching**: No Discogs lookups for Bluetooth. Show music note placeholder. Display only what the receiver provides.
- **StreamingAlbumArtFetcher class**: Entirely dropped. No external art fetching for any streaming source.
- **Spotify settings legacy migration**: No [Spotify] → [spotify] migration layer. Use canonical lowercase keys from the start.

### Deferred (implement in later phases)
- **Spotify album art via API**: The receiver CGI art works well enough. If stale art becomes a problem in practice, add per-track Spotify CDN fetching later.
- **Album art cache-busting timestamps**: Don't add `?t=<timestamp>` to receiver CGI URLs. Add only if stale art is observed in real usage.
- **Service name caching**: Don't cache last-known service name. Display whatever the receiver reports. Add caching only if "Unknown (FF)" flashing becomes a real UX annoyance.
- **Lidarr / SpotiFLAC Bridge integration**: Entire feature deferred to a late optional phase. When implemented, the track buffering logic (accumulating title/artist/album from async eISCP messages, 2-second timeout, deduplication) must be its own class, not embedded in AppState.

## Critical Domain Knowledge

These behaviors exist for non-obvious reasons. They MUST be preserved or specifically addressed:

1. **Volume control for smooth encoder**: The PEC11R-4020F-S0024 has 24 pulses/revolution and ZERO detents — it's smooth-turning with no click stops. Rapid turning generates a continuous stream of events (potentially 30-50+ in under 100ms). The original 100ms throttle-and-ignore approach caused a "fighting" UX where volume would jump back during rapid adjustment. The rewrite must treat encoder input as a continuous gesture: update UI optimistically, coalesce all events until the user stops turning, send a single absolute volume command, and only reconcile with receiver state after the gesture ends. The receiver always lands on the correct final value — the problem is purely transient UX during adjustment.

2. **CD and Library sharing receiver input "02"**: Both use the S/PDIF hardware output (Nvdigi HAT). The receiver has one physical input for this. The unified LocalPlaybackController naturally handles this — one controller, one ALSA device, mutually exclusive sources. The user will never want CD and Library playing simultaneously.

3. **ALSA device exclusivity**: Only one source can use the ALSA device at a time. The unified LocalPlaybackController solves this architecturally — switching audio streams within a single controller rather than stopping one controller and starting another.

4. **CD metadata fetch can block/freeze the app**: The original implementation has a known bug where metadata fetching (network lookups to MusicBrainz/GnuDB/Discogs) can freeze the entire UI. This was observed in production — the app became completely unresponsive during CD insertion, requiring a full reboot. ALL network I/O in the rewrite must be fully async with no path that can block the Qt event loop. This is the #1 reliability concern. The system is a kiosk shown to visitors — freezes are unacceptable.

5. **CD metadata should show progressively**: The 15-20 second delay from CD insertion to metadata display is the network lookup time. The rewrite should show TOC data immediately (Track 1, Track 2, etc. with durations calculated from sector counts) and fill in titles/artist/album art asynchronously as they arrive from the lookup chain. The user should never stare at a blank screen waiting for metadata.

6. **GnuDB response validation**: GnuDB can return error text instead of metadata (observed with obscure CDs not in MusicBrainz). Rather than caching bad data and building eviction logic, validate responses BEFORE caching — reject anything that doesn't contain proper track listings.

7. **CD playback latency**: ~2-3 seconds from pressing play to hearing audio is acceptable (hardware spin-up + buffer prefill). Do not over-optimize this — it matches the experience of vintage CD players.

8. **ALSA configuration difficulty**: Finding the correct ALSA device string and PCM parameters for the Nvdigi HAT's S/PDIF output was extremely difficult in the original implementation. The working configuration (hw:2,0, 44100Hz, 16-bit, stereo) should be preserved and well-documented. Initial playback was catastrophically slow (>1 minute startup) until major refactoring; the current buffering parameters are hard-won and should be carried over.

9. **Receiver eISCP metadata timing**: The receiver sends track metadata fields (title, artist, album) as separate eISCP messages that arrive at different times, not atomically. Any feature that needs complete track metadata (e.g., future Lidarr integration) must account for this — buffer partial data and emit only when complete.

10. **Spotify playback sets volume to 40 and shows overlay**: When initiating Spotify playback from idle (via search or playlist tile, when nothing else is already playing), the receiver sends an unsolicited volume change to 40 (decimal). The original code treats this like any user-initiated volume change — it applies the volume AND shows the volume overlay. This is a receiver behavior, not a code bug, but the code should handle it: volume changes originating from the receiver (not from the encoder or touchscreen) should be applied silently without showing the overlay. The volume overlay should ONLY appear for user-initiated changes (encoder rotation, touchscreen drag). Investigating the receiver's telnet server may provide event context to distinguish these cases.

11. **Spotify playback pausing unexpectedly**: Spotify playback would sometimes pause on its own. Root cause unknown — could be Spotify Connect session management, receiver behavior, or a bug in the playback control code. Should be investigated during implementation.

12. **Volume overlay should not appear for external changes**: The Onkyo app can change volume without the overlay appearing (confirmed working in original). The rule should be: overlay appears ONLY for local user input (encoder, touchscreen). All other volume changes (receiver auto-volume, Onkyo app, Spotify Connect initialization) should update the displayed volume silently.

## Suggested Agent Usage During Development

- Use `feature-dev:code-explorer` as a **Behavior Tracer**: before implementing each module, trace the complete execution path through the OLD codebase at ~/Code/media-console to produce acceptance criteria
- Use `superpowers:code-reviewer` as a **Decision Enforcer**: after each module is implemented, review against DECISIONS.md and architectural requirements
- Use `feature-dev:code-architect` for **interface design**: when designing each module's interface before implementation
- Use `gsd-verifier` + `gsd-integration-checker` for **parity checking**: verify feature parity between old and new repos periodically

## Coding Standards

A `CODING_STANDARDS.md` file must be created in the repo root as one of the first project actions. All agents must read and adhere to it. The standards document should cover the following, at minimum:

### Language & Style
- **C++17** standard, no compiler-specific extensions
- **Qt conventions**: camelCase for methods and variables, PascalCase for class names, `m_` prefix for member variables
- No mixed naming styles — eliminate PascalCase methods (like `PlaybackLoop`, `SectorToSeconds`) that crept in from C-style API wrapping in the original codebase
- Header files use `#pragma once`
- Include order: corresponding header first, then Qt headers, then system headers, then project headers — each group separated by a blank line

### Formatting
- **clang-format** enforced via pre-commit hook with a `.clang-format` config checked into the repo
- The project `.clang-format` should be created early and all code must conform from the first commit
- No formatting debates — the tool decides

### Linting
- **clang-tidy** with a `.clang-tidy` config checked into the repo
- Integrated into CMake build (CMAKE_CXX_CLANG_TIDY)
- Warnings treated as errors in CI — code must pass clang-tidy cleanly
- Minimum checks enabled: modernize-*, bugprone-*, performance-*, readability-* (tune as needed to avoid false positives)

### Testing
- **Google Test (gtest)** as the primary test framework
- Qt::Test utilities may be used alongside gtest when testing signal/slot behavior
- **All new code must have corresponding unit tests** — no module is merged without tests
- Tests live in a `tests/` directory mirroring the `src/` structure (e.g., `tests/audio/`, `tests/receiver/`, `tests/cd/`)
- Test files named `test_<ClassName>.cpp` (e.g., `test_LocalPlaybackController.cpp`)
- Mock interfaces for hardware dependencies (ALSA, GPIO, CD drive) to enable testing on any platform
- Integration tests for signal/slot wiring where practical

### Architecture Rules
- No god objects — classes should have a single, clear responsibility
- No business logic in AppState — it is a reactive property bag for QML binding only
- All network I/O must be fully async — no path may block the Qt event loop
- Hardware access through interfaces, never directly — enables testing with mocks
- Configuration read once at startup into a typed struct, passed by const reference — no scattered QSettings reads
- Platform-specific code behind runtime interfaces, not `#ifdef` guards

### Documentation
- Public class interfaces must have brief Doxygen-style `@brief` comments
- Non-obvious behavior must have inline comments explaining WHY, not WHAT
- No commented-out code — delete it, git has history

### Git Discipline
- Atomic commits — each commit compiles and passes tests
- Descriptive commit messages: imperative mood, explain the why
- No commits without tests passing

## Non-Commercial License

The project uses a non-commercial license. See the original repo's LICENSE file.
