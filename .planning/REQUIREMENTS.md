# Requirements: Media Console v2

**Defined:** 2026-02-28
**Core Value:** The touchscreen kiosk must always be responsive — no freezes, no unrecoverable states. A visitor should be able to walk up, select a source, and hear music within seconds.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Foundation

- [ ] **FOUND-01**: Project builds with CMake/Ninja targeting Qt6 6.8.2 on Raspberry Pi OS Trixie (aarch64)
- [ ] **FOUND-02**: clang-format enforced via .clang-format config with pre-commit hook
- [ ] **FOUND-03**: clang-tidy integrated into CMake build with modernize-*, bugprone-*, performance-*, readability-* checks
- [ ] **FOUND-04**: Google Test framework integrated with CTest, test discovery via gtest_discover_tests()
- [ ] **FOUND-05**: Platform abstraction interfaces defined (IAudioOutput, IGpioMonitor, ICdDrive, IDisplayControl) with stub implementations
- [ ] **FOUND-06**: PlatformFactory provides real or stub implementations based on runtime detection
- [ ] **FOUND-07**: AppConfig struct loaded once at startup from QSettings INI, passed by const reference — no scattered QSettings reads
- [ ] **FOUND-08**: Composition root (AppBuilder) constructs full object graph with clear ownership and signal/slot wiring
- [ ] **FOUND-09**: CODING_STANDARDS.md created and enforced from first commit
- [ ] **FOUND-10**: Logging categories defined (media.app, media.spotify, media.receiver, media.audio, media.http, media.lidarr, media.gpio, media.cd) with configurable levels

### State Layer

- [x] **STATE-01**: ReceiverState as thin Q_PROPERTY bag exposing volume, input, power, mute, metadata — no business logic
- [x] **STATE-02**: PlaybackState as thin Q_PROPERTY bag exposing playback mode, position, duration, track info — no business logic
- [x] **STATE-03**: UIState as thin Q_PROPERTY bag exposing overlay visibility, active view, error states — no business logic
- [x] **STATE-04**: MediaSource enum separates user-facing sources from receiver input hex codes
- [x] **STATE-05**: All state objects registered as QML singletons via qmlRegisterSingletonInstance()

### Receiver Control

- [x] **RECV-01**: eISCP TCP connection to Onkyo TX-8260 with auto-reconnect on network errors
- [x] **RECV-02**: Volume control: 0-200 range displayed as 0.0-100.0 with decimal precision
- [x] **RECV-03**: Volume gesture coalescing for smooth encoder: coalesce continuous events, send single absolute command after gesture ends, reconcile with receiver after
- [x] **RECV-04**: Optimistic UI updates: volume display updates immediately during gesture
- [x] **RECV-05**: 6 input sources selectable: Streaming/NET (2B), Phono (22), CD (02), Computer (05), Bluetooth (2E), Library (02)
- [x] **RECV-06**: Power on/off control
- [x] **RECV-07**: Mute toggle/on/off control (toggleMute in receiver interface, not requiring downcast)
- [x] **RECV-08**: State polling every 2.5s
- [x] **RECV-09**: Track metadata parsing from eISCP: NTM (time), NJA2 (album art URL), NFI (file info), NMS (service detection), NTI (title), NAT (artist), NAL (album)
- [x] **RECV-10**: Streaming service detection: Spotify, Pandora, AirPlay, Amazon Music, Chromecast
- [x] **RECV-11**: Playback state tracking via NST command (playing, paused, stopped)
- [x] **RECV-12**: Stale data detection: warn after 30s without receiver updates during active playback
- [x] **RECV-13**: Volume overlay appears ONLY for local user input (encoder, touchscreen). External changes (receiver auto-volume, Onkyo app, Spotify Connect) update silently.

### Audio Pipeline

- [x] **AUDIO-01**: AudioStream interface (open, readFrames, totalFrames, close, seek) implemented by CdAudioStream and FlacAudioStream
- [x] **AUDIO-02**: Single LocalPlaybackController parameterized by AudioStream — replaces separate CD and FLAC controllers
- [x] **AUDIO-03**: ALSA PCM output to S/PDIF device (hw:2,0) at 44100Hz, 16-bit, stereo via IAudioOutput interface
- [x] **AUDIO-04**: Background thread playback loop with atomic flag control (stop, pause, pending track, pending seek)
- [x] **AUDIO-05**: Intelligent audio buffering: 8-second buffer, 1-second prefill target, max 3 retries with 50ms backoff
- [x] **AUDIO-06**: Buffer statistics: xrun tracking, per-read latency (avg/max microseconds), read error counting
- [x] **AUDIO-07**: EIO recovery: close and reopen ALSA device on I/O errors, emit audioRecoveryFailed when exhausted
- [x] **AUDIO-08**: Full playback control: play, pause, stop, next, previous, seek (sector-based for CD, sample-based for FLAC)
- [x] **AUDIO-09**: ALSA device exclusivity enforced architecturally — one controller, one device, mutually exclusive sources

### CD Subsystem

- [x] **CD-01**: CdAudioStream wraps libcdio with paranoia error correction, implementing AudioStream interface
- [x] **CD-02**: TOC reading via CdDrive (libcdio wrapper) through ICdDrive interface
- [ ] **CD-03**: Hybrid disc presence detection: QFileSystemWatcher + ioctl polling with debounced state changes
- [ ] **CD-04**: CD playback is ALWAYS user-initiated — no auto-play on insert or startup
- [ ] **CD-05**: Three-tier async metadata lookup: MusicBrainz (via libdiscid) -> GnuDB (CDDB) -> Discogs (REST API) — ALL fully async, no event loop blocking
- [ ] **CD-06**: Progressive metadata display: show TOC immediately (Track 1, Track 2 with durations), fill in titles/artist/album art asynchronously
- [ ] **CD-07**: SQLite metadata cache: instant lookup for previously-seen discs by disc ID
- [ ] **CD-08**: Album art downloading from CoverArtArchive and Discogs, cached to disk with front and back cover support
- [ ] **CD-09**: GnuDB response validation before caching — reject malformed responses
- [ ] **CD-10**: Graceful degradation: fall back to generic "Audio CD" metadata if all sources fail
- [ ] **CD-11**: Idle timer stops CD spindle after period of inactivity
- [ ] **CD-12**: Spin-up timer handles drive spin-up delay on play

### FLAC Library

- [ ] **FLAC-01**: FlacAudioStream wraps libsndfile decoding + libsamplerate resampling to 44100Hz/16-bit/stereo, implementing AudioStream interface
- [ ] **FLAC-02**: Recursive directory scanner using TagLib for metadata extraction
- [ ] **FLAC-03**: SQLite database indexing: title, artist, album artist, album, track/disc number, year, genre, duration, sample rate, bit depth, file path, modification time
- [ ] **FLAC-04**: Incremental scanning: skip unchanged files based on mtime
- [ ] **FLAC-05**: Album art extraction from FLAC picture blocks, cached via SHA-1(artist+album) filename with MIME type auto-detection
- [ ] **FLAC-06**: Async scanning via QtConcurrent
- [ ] **FLAC-07**: Three hierarchical QAbstractListModel subclasses: LibraryArtistModel, LibraryAlbumModel, LibraryTrackBrowseModel
- [ ] **FLAC-08**: Playlist-based playback with next/previous navigation and associated LibraryTrack metadata

### GPIO Hardware

- [x] **GPIO-01**: Volume rotary encoder monitor (PEC11R-4020F-S0024): GPIO 27/22/23, quadrature decoding via libgpiod v2 API, gesture-based coalescing
- [x] **GPIO-02**: Input rotary encoder monitor (PEC11R-4320F-S0012): GPIO 16/20/5, quadrature decoding, 250ms switch debounce, 1:1 detent-to-input mapping
- [x] **GPIO-03**: Reed switch monitor: GPIO 17, 500ms debounce, magnets apart = display on, together = off
- [x] **GPIO-04**: All monitors run in background threads via libgpiod v2 event monitoring on /dev/gpiochip4
- [x] **GPIO-05**: Linux-only with no-op stubs on other platforms (via IGpioMonitor interface)
- [x] **GPIO-06**: Mute button triggers on ONE edge only (falling or rising, not both) — fixes known double-toggle bug

### Spotify

- [x] **SPOT-01**: OAuth 2.0 PKCE Authorization Code flow with token persistence and auto-refresh (5 min pre-expiry)
- [x] **SPOT-02**: Device discovery and playback transfer by device name
- [x] **SPOT-03**: Playback control: play, pause, next, previous, play by URI, context-aware playback
- [x] **SPOT-04**: Full-text search via Spotify Web API (tracks, artists, albums)
- [x] **SPOT-05**: Queue management: add tracks to queue
- [x] **SPOT-06**: Suggested playlists: featured + user recommendations
- [x] **SPOT-07**: Active session detection with takeover dialog showing current device and track
- [x] **SPOT-08**: Album art: use receiver-provided CGI art for all streaming sources

### Display Control

- [ ] **DISP-01**: DDC/CI power and brightness control via ddcutil subprocess through IDisplayControl interface
- [ ] **DISP-02**: Smooth dimming transitions, fade to black
- [ ] **DISP-03**: Auto-detection of display bus
- [ ] **DISP-04**: Screen timeout state machine: ACTIVE -> DIMMED -> OFF (+ DOOR_CLOSED from reed switch)
- [ ] **DISP-05**: Configurable dim timeout (default 5 min), off timeout (default 20 min), dim brightness (default 25%)
- [ ] **DISP-06**: Activity-based reset (touch events reset timeout)
- [ ] **DISP-07**: Playback-aware: disable timeout during active music playback
- [ ] **DISP-08**: Door sensor integration via reed switch

### HTTP API

- [ ] **API-01**: REST API via Qt HttpServer on configurable port (default 8080)
- [ ] **API-02**: Endpoints: volume set, input switch, status query, display power
- [ ] **API-03**: Spotify OAuth setup page and callback endpoint
- [ ] **API-04**: Spotify search endpoint
- [ ] **API-05**: Optional HTTPS via auto-generated self-signed certificate

### QML UI

- [ ] **UI-01**: Deep blue color theme (#0a1628 primary, #162844 secondary, #2e5c8a accent)
- [ ] **UI-02**: Theme.qml singleton with design tokens: colors, typography (6 sizes 12-48px), spacing (8/16/24px), touch targets (44/64px), animation durations (150/300/500ms)
- [ ] **UI-03**: Main layout: left panel (input icon + service label), right panel (NowPlaying or LibraryBrowser), top status bar
- [ ] **UI-04**: NowPlaying: album art (front/back carousel), track info, progress bar with seek, playback controls
- [ ] **UI-05**: InputCarousel: 3D perspective carousel with 6 inputs, 4-second auto-select timeout, encoder-driven navigation
- [ ] **UI-06**: LibraryBrowser: StackView drill-down (Artists -> Albums -> Tracks), artist A-Z quick scroll sidebar, split layout on track page
- [ ] **UI-07**: SpotifySearch: fullscreen overlay with on-screen QWERTY keyboard (SimpleKeyboard), search results with album art
- [ ] **UI-08**: SpotifyTakeoverDialog: modal confirmation for session transfer
- [ ] **UI-09**: AudioErrorDialog: modal for ALSA recovery failures with retry option
- [ ] **UI-10**: ToastNotification: bottom-center 3-second auto-dismiss (info/success/error types)
- [ ] **UI-11**: VolumeOverlay: large modal with numeric display, auto-dismiss after 2s — only shown for local user input
- [ ] **UI-12**: VolumeIndicator: persistent top-right display with draggable slider
- [ ] **UI-13**: EjectButton: visible only when CD present
- [ ] **UI-14**: SearchButton: visible only on Spotify input
- [ ] **UI-15**: ErrorBanner: shown when receiver disconnected
- [ ] **UI-16**: TimeDisplay: current time, updates every minute
- [ ] **UI-17**: PowerButton, MuteButton with visual state indicators
- [ ] **UI-18**: Global MouseArea for touch activity detection (resets screen timeout)

### Orchestration

- [ ] **ORCH-01**: PlaybackRouter owns input->controller routing, eliminating duplicated if/else chains across play/pause/stop/next/previous/seek
- [ ] **ORCH-02**: AlbumArtResolver: receiver CGI art for streaming sources, local cached art for CD and Library
- [x] **ORCH-03**: VolumeGestureController: coalesces encoder events, manages optimistic UI, sends commands after gesture ends

### Production

- [ ] **PROD-01**: Kiosk mode via systemd service with auto-start, auto-restart on crash
- [ ] **PROD-02**: Auto-login, hidden cursor (unclutter), disabled screen blanking
- [ ] **PROD-03**: install-kiosk.sh / uninstall-kiosk.sh deployment scripts

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Lidarr Integration

- **LIDARR-01**: Forward Spotify track notifications to SpotiFLAC Bridge via HTTP POST
- **LIDARR-02**: Track buffering: accumulate title/artist/album from async eISCP messages, emit when complete
- **LIDARR-03**: 2-second buffer timeout for incomplete data
- **LIDARR-04**: Deduplication: suppress repeated emissions for same artist+album, clear every 10 minutes
- **LIDARR-05**: LidarrTrackBuffer as its own class, not embedded in AppState

### Enhanced Album Art

- **ART-01**: Spotify album art via Spotify CDN API (per-track)
- **ART-02**: Album art cache-busting timestamps on receiver CGI URLs
- **ART-03**: Service name caching to avoid "Unknown (FF)" flashing

### Receiver Telnet

- **TEL-01**: Investigate receiver telnet server as event-driven alternative to eISCP polling
- **TEL-02**: Distinguish user-initiated vs external volume changes via telnet event context

## Out of Scope

| Feature | Reason |
|---------|--------|
| CD auto-play | Always user-initiated — explicit design decision, no config option |
| Bluetooth album art (Discogs) | Seldom-used input, receiver metadata is sufficient |
| StreamingAlbumArtFetcher | No external art fetching for streaming sources — receiver CGI is sufficient |
| Spotify settings migration | No [Spotify] -> [spotify] key migration — use lowercase from start |
| Multi-room audio | Not a distributed system — single receiver, single display |
| Web radio / internet streams | Receiver handles streaming via built-in services |
| DSP / equalizer | Receiver handles audio processing |
| Gapless playback | Low priority for CD/FLAC kiosk use case |
| Additional streaming services | Receiver natively supports these; no app integration needed |
| Mobile app / remote UI | Kiosk-only interface with HTTP API for basic remote control |
| Cross-compilation | Build natively on Pi 5 — sufficient performance |
| Feature expansion | This is a structural rewrite, not new functionality |

## Traceability

Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| FOUND-01 | Phase 1 | Pending |
| FOUND-02 | Phase 1 | Pending |
| FOUND-03 | Phase 1 | Pending |
| FOUND-04 | Phase 1 | Pending |
| FOUND-05 | Phase 1 | Pending |
| FOUND-06 | Phase 1 | Pending |
| FOUND-07 | Phase 1 | Pending |
| FOUND-08 | Phase 1 | Pending |
| FOUND-09 | Phase 1 | Pending |
| FOUND-10 | Phase 1 | Pending |
| STATE-01 | Phase 2 | Complete |
| STATE-02 | Phase 2 | Complete |
| STATE-03 | Phase 2 | Complete |
| STATE-04 | Phase 2 | Complete |
| STATE-05 | Phase 2 | Complete |
| RECV-01 | Phase 3 | Complete |
| RECV-02 | Phase 3 | Complete |
| RECV-03 | Phase 3 | Complete |
| RECV-04 | Phase 3 | Complete |
| RECV-05 | Phase 3 | Complete |
| RECV-06 | Phase 3 | Complete |
| RECV-07 | Phase 3 | Complete |
| RECV-08 | Phase 3 | Complete |
| RECV-09 | Phase 3 | Complete |
| RECV-10 | Phase 3 | Complete |
| RECV-11 | Phase 3 | Complete |
| RECV-12 | Phase 3 | Complete |
| RECV-13 | Phase 3 | Complete |
| ORCH-03 | Phase 3 | Complete |
| AUDIO-01 | Phase 4 | Complete |
| AUDIO-02 | Phase 4 | Complete |
| AUDIO-03 | Phase 4 | Complete |
| AUDIO-04 | Phase 4 | Complete |
| AUDIO-05 | Phase 4 | Complete |
| AUDIO-06 | Phase 4 | Complete |
| AUDIO-07 | Phase 4 | Complete |
| AUDIO-08 | Phase 4 | Complete |
| AUDIO-09 | Phase 4 | Complete |
| CD-01 | Phase 5 | Complete |
| CD-02 | Phase 5 | Complete |
| CD-03 | Phase 5 | Pending |
| CD-04 | Phase 5 | Pending |
| CD-05 | Phase 5 | Pending |
| CD-06 | Phase 5 | Pending |
| CD-07 | Phase 5 | Pending |
| CD-08 | Phase 5 | Pending |
| CD-09 | Phase 5 | Pending |
| CD-10 | Phase 5 | Pending |
| CD-11 | Phase 5 | Pending |
| CD-12 | Phase 5 | Pending |
| FLAC-01 | Phase 6 | Pending |
| FLAC-02 | Phase 6 | Pending |
| FLAC-03 | Phase 6 | Pending |
| FLAC-04 | Phase 6 | Pending |
| FLAC-05 | Phase 6 | Pending |
| FLAC-06 | Phase 6 | Pending |
| FLAC-07 | Phase 6 | Pending |
| FLAC-08 | Phase 6 | Pending |
| GPIO-01 | Phase 7 | Complete |
| GPIO-02 | Phase 7 | Complete |
| GPIO-03 | Phase 7 | Complete |
| GPIO-04 | Phase 7 | Complete |
| GPIO-05 | Phase 7 | Complete |
| GPIO-06 | Phase 7 | Complete |
| SPOT-01 | Phase 8 | Complete |
| SPOT-02 | Phase 8 | Complete |
| SPOT-03 | Phase 8 | Complete |
| SPOT-04 | Phase 8 | Complete |
| SPOT-05 | Phase 8 | Complete |
| SPOT-06 | Phase 8 | Complete |
| SPOT-07 | Phase 8 | Complete |
| SPOT-08 | Phase 8 | Complete |
| DISP-01 | Phase 9 | Pending |
| DISP-02 | Phase 9 | Pending |
| DISP-03 | Phase 9 | Pending |
| DISP-04 | Phase 9 | Pending |
| DISP-05 | Phase 9 | Pending |
| DISP-06 | Phase 9 | Pending |
| DISP-07 | Phase 9 | Pending |
| DISP-08 | Phase 9 | Pending |
| API-01 | Phase 9 | Pending |
| API-02 | Phase 9 | Pending |
| API-03 | Phase 9 | Pending |
| API-04 | Phase 9 | Pending |
| API-05 | Phase 9 | Pending |
| ORCH-01 | Phase 9 | Pending |
| ORCH-02 | Phase 9 | Pending |
| UI-01 | Phase 10 | Pending |
| UI-02 | Phase 10 | Pending |
| UI-03 | Phase 10 | Pending |
| UI-04 | Phase 10 | Pending |
| UI-05 | Phase 10 | Pending |
| UI-06 | Phase 10 | Pending |
| UI-07 | Phase 10 | Pending |
| UI-08 | Phase 10 | Pending |
| UI-09 | Phase 10 | Pending |
| UI-10 | Phase 10 | Pending |
| UI-11 | Phase 10 | Pending |
| UI-12 | Phase 10 | Pending |
| UI-13 | Phase 10 | Pending |
| UI-14 | Phase 10 | Pending |
| UI-15 | Phase 10 | Pending |
| UI-16 | Phase 10 | Pending |
| UI-17 | Phase 10 | Pending |
| UI-18 | Phase 10 | Pending |
| PROD-01 | Phase 10 | Pending |
| PROD-02 | Phase 10 | Pending |
| PROD-03 | Phase 10 | Pending |

**Coverage:**
- v1 requirements: 108 total
- Mapped to phases: 108
- Unmapped: 0

---
*Requirements defined: 2026-02-28*
*Last updated: 2026-02-28 after roadmap creation*
