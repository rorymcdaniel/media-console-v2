# Project Research Summary

**Project:** Media Console v2
**Domain:** Qt6/QML embedded music kiosk (Raspberry Pi 5, Debian Trixie)
**Researched:** 2026-02-28
**Confidence:** HIGH

## Executive Summary

Media Console v2 is a complete rewrite of a working v1 embedded music console: a native Qt6/QML kiosk application running on Raspberry Pi 5 that controls an Onkyo receiver via eISCP, plays CDs and local FLAC files through a Nvdigi S/PDIF HAT, and integrates Spotify. Unlike every competitor (Volumio, MoOde, HiFiBerryOS), this is not a web-based MPD frontend — it is a native application with dedicated physical hardware (two rotary encoders, a reed switch) that fully replaces the receiver remote. The recommended approach is a clean architectural rewrite using Qt6 6.8 LTS (the Debian Trixie package version), C++17, and a strict layered architecture that separates state objects from business logic, with all hardware access behind abstract interfaces and a single composition root (AppBuilder) that wires the entire object graph.

The single most important architectural decision is the decomposition of the v1 god-object AppState (1,400+ lines) into focused, independently testable components: ReceiverState, PlaybackState, and UIState as passive Q_PROPERTY bags; PlaybackRouter, VolumeGestureController, and AlbumArtResolver as active orchestration; and domain services (EiscpReceiverController, LocalPlaybackController, CDMetadataFetcher, SpotifyAuthManager) with abstract interfaces enabling Google Mock-based testing. A unified LocalPlaybackController that accepts any IAudioStream (CdAudioStream or FlacAudioStream) structurally solves ALSA device exclusivity — the source of a whole class of v1 bugs.

The top risks are concentrated in three areas: (1) the Qt event loop — any blocking I/O on the main thread freezes a kiosk with no recovery path for the user; (2) the ALSA S/PDIF pipeline — which has two distinct failure modes (XRUN and EIO) that require different recovery strategies, and strict 44100Hz sample rate enforcement; and (3) the API compatibility cliff — libgpiod v1 to v2 is a breaking change (gpiochip0 becomes gpiochip4 on Pi 5, entirely new request-centric API), and Spotify's implicit grant flow was deprecated in November 2025 (PKCE required). All three risks have well-understood mitigations documented in the reference implementation, so they are execution risks, not research unknowns.

## Key Findings

### Recommended Stack

The target platform is Debian 13 Trixie (Raspberry Pi OS), and all version decisions are locked to Trixie's package repositories — not upstream latest. Qt 6.8.2 is the LTS release available in Trixie (supported through October 2029) and is the correct target; Qt 6.9/6.10 are non-LTS and should not be chased. The build system is CMake 3.31.6 + Ninja 1.12.1 with `qt_standard_project_setup()` and `qt_add_qml_module()`. Audio is handled entirely via direct ALSA (`hw:2,0`, S16_LE, 44100Hz, period=1024, buffer=4096) — Qt Multimedia is explicitly excluded because it cannot guarantee bit-perfect S/PDIF output. Testing uses Google Test 1.16.0 with Google Mock; the abstract interface pattern means all hardware code is testable without real hardware.

See `.planning/research/STACK.md` for complete version matrix, CMake patterns, and installation commands.

**Core technologies:**
- **Qt6 6.8.2 (LTS):** Application framework, QML UI, networking, SQL, HTTP server — Trixie version, NOT upstream latest
- **C++17:** Required by Qt6.8 and TagLib 2.x; structured bindings and std::optional are useful in this codebase
- **CMake 3.31.6 + Ninja:** Only first-class build system for Qt6; QMake is deprecated
- **libasound2 1.2.14 (ALSA):** Direct PCM output to S/PDIF HAT — `hw:2,0` only, no plughw/dmix
- **libgpiod 2.2.1 (v2 API):** BREAKING change from v1; Pi 5 uses gpiochip4; request-centric API required
- **libcdio-paranoia 10.2+2.0.2:** Jitter-corrected CD audio extraction; `cdio_paranoia_read_limited()` with 3 retries
- **TagLib 2.0.2:** FLAC metadata extraction; do NOT use v1 API patterns (major API change at 2.0)
- **libsndfile 1.2.2 + libsamplerate 0.2.2:** FLAC decoding and sample rate conversion
- **Qt6::HttpServer:** Built-in route-based HTTP server (not tech preview since Qt 6.5); handles Spotify OAuth callback
- **Google Test 1.16.0:** System package; `find_package(GTest)` only — no FetchContent needed
- **Spotify OAuth:** PKCE flow with http://127.0.0.1 loopback redirect; implicit grant is dead as of Nov 27, 2025

### Expected Features

The feature set is defined precisely: v2 must achieve parity with v1 before deployment. The hardware is already installed and running. Every P1 feature is a pre-condition for go-live.

See `.planning/research/FEATURES.md` for prioritization matrix and competitor analysis.

**Must have (table stakes for v1 parity):**
- Receiver control (power, volume, input, mute) via eISCP — foundation; everything else depends on this
- Volume encoder + input encoder GPIO integration — the primary physical interface
- CD playback with disc detection and progressive metadata (MusicBrainz -> GnuDB -> Discogs fallback)
- FLAC library browsing and playback via SQLite-backed library database
- Spotify integration (OAuth PKCE, search, device transfer, playback control, takeover dialog)
- Bluetooth metadata display (AVRCP via receiver, no album art fetching)
- Display power management state machine (ACTIVE -> DIMMED -> OFF, DDC/CI, door-triggered via reed switch)
- HTTP API server (volume, input, display, status, Spotify OAuth endpoints)
- Now Playing display with album art, volume overlay, input carousel, kiosk systemd deployment
- Error handling and auto-recovery (receiver reconnect, ALSA recovery, metadata timeouts)

**Should have (competitive differentiators already in v1):**
- Physical hardware integration as primary UX (dual encoders + reed switch)
- Native Qt6/QML at 60fps (not web UI — this is the architectural differentiator vs. all competitors)
- 3D input source carousel with auto-select timeout
- Spotify search with on-screen keyboard (not just Spotify Connect)
- Progressive CD metadata loading (TOC immediately, metadata fills in asynchronously)
- Unified LocalPlaybackController for ALSA device exclusivity

**Defer (v2+):**
- Gapless playback — significant complexity for niche use case
- Lidarr/SpotiFLAC bridge — optional automated music acquisition
- Album art cache-busting — defer until stale art is observed in production
- Service name caching — defer until "Unknown (FF)" flashing is confirmed as real issue
- Additional streaming services — Spotify + Bluetooth covers the need

**Explicitly excluded (anti-features):**
- CD auto-play on insert, multi-room, web radio, equalizer/DSP, remote web UI/mobile app, settings UI, screensaver

### Architecture Approach

The architecture is a strict 5-layer system: QML UI layer binds to thin reactive state objects (ReceiverState, PlaybackState, UIState) via Q_PROPERTY; an orchestration layer (PlaybackRouter, VolumeGestureController, AlbumArtResolver) coordinates between domain services; domain services (EiscpReceiverController, LocalPlaybackController, CDMetadataFetcher, LibraryScanner, SpotifyAuthManager) implement business logic; a platform abstraction layer (IAudioOutput, IGpioMonitor, ICdDrive, IDisplayControl) provides interface boundaries for hardware; and a composition root (AppBuilder) constructs the entire object graph, keeping main.cpp under 20 lines. The key CMake structure uses a library-per-domain model: `media-console-core` (platform-independent) and `media-console-platform` (hardware-specific) as static libraries, with tests linking only against core.

See `.planning/research/ARCHITECTURE.md` for complete directory structure, data flow diagrams, and code examples for each pattern.

**Major components:**
1. **AppBuilder (composition root)** — constructs all objects, wires all signals/slots, returns AppContext; replaces 325-line main.cpp
2. **ReceiverState / PlaybackState / UIState** — passive Q_PROPERTY bags with zero business logic; registered as QML singletons via `qmlRegisterSingletonInstance()`, NOT setContextProperty
3. **EiscpReceiverController** — TCP eISCP protocol with 2.5s polling, auto-reconnect, UTF-8 metadata parsing, hex volume parsing
4. **LocalPlaybackController** — unified CD/FLAC controller with IAudioStream interface; owns ALSA device exclusively; runs playback in `std::thread` with atomic flags
5. **PlaybackRouter** — single dispatch point for play/pause/stop/next/prev; eliminates the 7x duplicated source-routing if/else from v1
6. **VolumeGestureController** — coalesces encoder events into gestures (100ms commit timer); sends single eISCP command per gesture
7. **PlatformFactory** — runtime polymorphism for hardware (real on Linux, stubs on macOS/test); eliminates all `#ifdef __linux__` guards
8. **CDMetadataFetcher** — async three-tier lookup (MusicBrainz -> GnuDB -> Discogs) with SQLite cache; disc ID in `QtConcurrent::run()`
9. **ScreenTimeoutManager** — state machine (ACTIVE/DIMMED/OFF) with DDC/CI via serialized ddcutil command queue
10. **HttpApiServer** — Qt6::HttpServer with route-based REST API; handles Spotify OAuth loopback callback

### Critical Pitfalls

See `.planning/research/PITFALLS.md` for all 17 pitfalls with verification tests and recovery strategies.

1. **Blocking the Qt event loop** — Any `waitFor*()`, `QProcess::waitForFinished()`, or synchronous network call on the main thread freezes the kiosk permanently from the user's perspective. Mitigation: establish the async pattern in the foundation phase; enforce in code review with "zero waitFor calls" rule. This is the #1 reliability concern.

2. **ALSA S/PDIF EIO stale handle** — `-EIO` from `snd_pcm_writei()` cannot be recovered with `snd_pcm_recover()`; requires full close-reopen with exponential backoff (3 attempts: 500ms/1000ms/2000ms). Distinct from XRUN (`-EPIPE`). Must be built into AlsaOutput from day one; the reference implementation's pattern at `AlsaOutput.cpp:104-148` is battle-tested.

3. **ALSA sample rate mismatch** — `snd_pcm_hw_params_set_rate_near()` silently negotiates the wrong rate; S/PDIF output then produces silence with no error. After `snd_pcm_hw_params()`, always verify actual negotiated rate matches requested rate and abort if they differ. Disable ALSA resampler explicitly.

4. **libgpiod v1 API on Trixie** — libgpiod v1 API functions (`gpiod_chip_get_line`, `gpiod_line_request_*`) do not exist on Trixie. Pi 5 uses gpiochip4 (not gpiochip0). The entire GPIO monitoring code must use the v2 request-centric API. This is a complete rewrite, not a port.

5. **Volume encoder event storm** — The detentless PEC11R encoder generates 10-50 events per gesture. Each event cannot trigger an eISCP command or the receiver is flooded. The VolumeGestureController's 100ms commit timer pattern is mandatory, not optional.

6. **God object AppState** — The v1 AppState reached 1,400 lines. The rewrite must decompose this in the foundation phase. Adding a single feature to a god object creates cascading rework. This is the primary architectural risk of the rewrite.

7. **Spotify implicit grant flow** — Dead as of November 27, 2025. Only PKCE with http://127.0.0.1 loopback redirect works for public clients. Any code copying v1 OAuth patterns will not function.

## Implications for Roadmap

The architecture research provides an explicit 10-phase build order based on component dependencies. The roadmap should follow this order closely, as each phase enables the next. The feature dependency tree from FEATURES.md reinforces this ordering: receiver control is the foundation for almost everything else.

### Phase 1: Foundation and Build Infrastructure
**Rationale:** Every other component depends on the build system, CMake target structure, interfaces, platform abstraction, and composition root being in place first. This phase is the only one with zero hardware dependencies. Pitfalls 6 (god object) and 17 (GTest/moc compatibility) must be addressed here — they cannot be retrofitted later.
**Delivers:** Compilable project skeleton with all abstract interfaces defined, PlatformFactory with stubs, AppBuilder composition root, AppConfig typed struct, logging infrastructure, Google Test wiring, CI-ready build
**Addresses:** Project scaffold, testing infrastructure
**Avoids:** God object AppState (P6), `#ifdef __linux__` guards (Technical Debt), GTest/moc incompatibility (P17), cross-compilation moc tool selection (P16)

### Phase 2: State Layer and QML Binding Surface
**Rationale:** QML cannot be developed until the state objects it binds to exist and expose stable Q_PROPERTY interfaces. This phase produces the binding surface that all subsequent phases write to. Separating state from logic here prevents the god object pattern from re-emerging.
**Delivers:** ReceiverState, PlaybackState, UIState as thin Q_PROPERTY bags; registered as QML singletons (not setContextProperty); MediaSource enum; testable with QSignalSpy
**Addresses:** Now Playing display (partial), volume overlay (partial)
**Avoids:** setContextProperty anti-pattern (Anti-Pattern 2), god object AppState (P6)

### Phase 3: Receiver Control (eISCP)
**Rationale:** Receiver control is the dependency root for volume, input, power, mute, and metadata — the majority of P1 features. Nothing else can be functionally tested end-to-end without an eISCP connection. Must include volume gesture coalescing because it is tightly coupled to eISCP and its absence causes observable damage (event storm).
**Delivers:** EiscpReceiverController with TCP, auto-reconnect, polling; EiscpProtocol packet parsing; VolumeGestureController; ReceiverState populated; basic Now Playing with receiver metadata
**Addresses:** Receiver power/volume/input/mute, Now Playing display, receiver connection status
**Avoids:** Volume event storm (P5), eISCP metadata non-atomicity (P11), hex volume parsing gotcha, UTF-8 metadata encoding gotcha, NSVQSTN polling interference gotcha

### Phase 4: Audio Pipeline (ALSA + LocalPlaybackController)
**Rationale:** The unified LocalPlaybackController with IAudioStream/IAudioOutput interfaces is architectural infrastructure that both CD playback and FLAC playback depend on. Building it once correctly is significantly cheaper than building two separate controllers that then need to be unified. All ALSA pitfalls (EIO recovery, sample rate verification, ring buffer thread safety) must be addressed here.
**Delivers:** AlsaOutput (real + stub), IAudioStream interface, CdAudioStream, FlacAudioStream, LocalPlaybackController with playback thread, ring buffer with mutex/condition variable; ALSA EIO recovery and sample rate verification built in
**Addresses:** CD playback (partial), FLAC playback (partial), ALSA exclusivity
**Avoids:** ALSA EIO stale handle (P2), ALSA sample rate mismatch (P3), CD audio ring buffer thread safety (P12), blocking main thread with audio I/O (P1)

### Phase 5: CD Subsystem
**Rationale:** CD is a core use case and the most technically complex single feature (drive monitoring + paranoia extraction + async metadata pipeline + progressive display + SQLite cache). It depends on Phase 4's LocalPlaybackController and Phase 3's ReceiverState. Builds CDMonitor, CdMetadataFetcher, and the three-tier metadata lookup chain.
**Delivers:** CdDrive, CdMonitor (disc detection), CdAudioStream integration, CDMetadataFetcher (async, QtConcurrent), CDMetadataCache (SQLite WAL), progressive metadata display
**Addresses:** CD playback with disc detection, CD metadata display (progressive)
**Avoids:** CD metadata freeze (P7), GnuDB validation gotcha, MusicBrainz User-Agent header requirement, libcdio speed negotiation failure (P15), SQLite thread safety (P8)

### Phase 6: FLAC Library Subsystem
**Rationale:** Shares the audio pipeline from Phase 4 but requires its own database and scanning infrastructure. Independent of CD subsystem — can be built in parallel if schedule requires. LibraryScanner uses TagLib and QtConcurrent; must follow the one-connection-per-thread SQLite pattern.
**Delivers:** LibraryDatabase, LibraryScanner, LibraryArtistModel/AlbumModel/TrackBrowseModel (QAbstractListModel for QML), AlbumArtCache
**Addresses:** FLAC library browsing and playback
**Avoids:** SQLite thread safety violations (P8), TagLib 1.x API patterns (use 2.x PropertyMap interface)

### Phase 7: GPIO Hardware Integration
**Rationale:** Can be built in parallel with CD and Library phases since GPIO pins are independent. However, GPIO is required for volume encoder and input encoder — the primary physical interface. Must use libgpiod v2 API throughout. Builds all three monitors (volume encoder, input encoder, reed switch).
**Delivers:** VolumeEncoderMonitor, InputEncoderMonitor, ReedSwitchMonitor using libgpiod v2 (gpiochip4); quadrature decoding; hardware debounce via `gpiod_line_settings_set_debounce_period_us()`; edge filtering for mute switch
**Addresses:** Volume encoder, input encoder, reed switch / door-triggered display
**Avoids:** libgpiod v1 API (P4 — will not compile on Trixie), GPIO double-toggle mute bug (P4), wrong chip path (must be gpiochip4 on Pi 5)

### Phase 8: Spotify Integration
**Rationale:** High user value but dependent on Phase 3 (receiver control for device transfer) and requires HTTPS HTTP server capability. Complex OAuth lifecycle must be robust from first implementation — token expiry at 1 hour means it will be encountered during any extended test session.
**Delivers:** SpotifyAuthManager (PKCE OAuth, token refresh with 5-minute pre-expiry scheduling, QSettings persistence), Spotify playback control via Web API, device discovery and transfer, SpotifyTakeoverDialog, Spotify search UI with on-screen keyboard
**Addresses:** Spotify integration (OAuth, search, device transfer, playback)
**Avoids:** Spotify implicit grant flow (P9 — dead Nov 2025), Spotify token expiry silent failure (P9), device_id parameter on queue requests gotcha, play retry on 404/403 gotcha

### Phase 9: Display Control, HTTP API, and System Integration
**Rationale:** Display control and HTTP API are relatively self-contained but depend on the state layer being stable (Phase 2) to report accurate status. ScreenTimeoutManager depends on music-playing state from LocalPlaybackController. This phase also wires all subsystems together through AppBuilder, which cannot be complete until all subsystems exist.
**Delivers:** DisplayControl (DDC/CI via serialized ddcutil queue), ScreenTimeoutManager (state machine with music-playing inhibit, door-triggered via reed switch), HttpApiServer (REST endpoints), complete AppBuilder wiring of all subsystems
**Addresses:** Display power management, HTTP API server, error handling / recovery, kiosk systemd deployment
**Avoids:** ddcutil I2C bus contention from concurrent processes (P10), CD eject blocking UI (P1), display dimming while music plays

### Phase 10: QML UI Completion and Production Hardening
**Rationale:** QML development can proceed in parallel from Phase 2 onward with stub data, but final integration and visual polish happen here. Production hardening addresses the kiosk-specific concerns (24-hour soak tests, memory leak detection, systemd watchdog).
**Delivers:** All QML components fully integrated (NowPlaying, InputCarousel, LibraryBrowser, SpotifySearch, VolumeOverlay, PlaybackControls, VolumeIndicator), Theme.qml design tokens, kiosk deployment scripts, systemd service with watchdog, 24-hour soak test passing
**Addresses:** Touch-friendly UI sizing, toast notifications, eject button, full-screen kiosk mode, error banner
**Avoids:** QML binding loops (P13), QML image memory leak on long-running kiosk (P14), setContextProperty (already avoided in Phase 2)

### Phase Ordering Rationale

- **Phases 1-2 are immovable prerequisites.** The composition root and state layer must exist before anything else. Skipping them results in the god object pattern re-emerging.
- **Phase 3 (receiver) before Phases 5-8 (features).** Nearly every feature depends on eISCP connectivity. Stabilizing the receiver layer first enables correct end-to-end integration testing for all subsequent phases.
- **Phase 4 (audio pipeline) before Phases 5-6 (CD/FLAC).** The unified LocalPlaybackController is shared infrastructure. Building it once correctly is the architectural win of the rewrite.
- **Phases 5, 6, 7 can overlap.** CD subsystem, FLAC library, and GPIO are independent — they use separate hardware and share only the audio pipeline interface. Schedule parallelism is possible here.
- **Phase 8 (Spotify) after Phase 3.** Device transfer requires a working receiver controller. Spotify search requires a working HTTP server, built in Phase 9 — so Spotify OAuth infrastructure can be built in Phase 8 but the full Spotify search UI lands in Phase 10.
- **Phase 9 (integration) after all subsystems.** AppBuilder wiring cannot be complete until all components exist.
- **Phase 10 (QML/hardening) is gated on Phase 9.** Final UI integration and 24-hour soak tests are the production qualification gate.

### Research Flags

Phases needing deeper research during planning:
- **Phase 5 (CD Subsystem):** GnuDB response validation patterns are underdocumented; test against real discs during implementation. The three-tier fallback chain has edge cases (disc IDs that return results in MusicBrainz but no recordings, GnuDB returning valid structure but wrong data). The reference implementation's validation logic should be lifted directly.
- **Phase 7 (GPIO):** Pi 5 gpiochip4 path is MEDIUM confidence (Raspberry Pi Forums, not official docs). Verify on actual hardware before assuming. The v2 libgpiod API for quadrature decoding requires careful edge detection configuration.
- **Phase 9 (Display Control):** ddcutil behavior on Pi 5 is MEDIUM confidence — there is a known issue where `setvcp` may not correctly update values. The `--sleep-multiplier` option may be needed. Verify DDC/CI capability of the specific display before assuming it works.

Phases with well-documented patterns (skip or minimize research-phase):
- **Phase 1 (Foundation):** CMake + Qt6 + Google Test integration is comprehensively documented in official Qt and CMake docs. Patterns are established.
- **Phase 3 (Receiver Control):** eISCP protocol is fully documented in the reference implementation. All parsing patterns and command codes are known.
- **Phase 4 (Audio Pipeline):** ALSA PCM API is well-documented; the specific EIO recovery pattern is proven in the reference implementation and should be copied, not researched fresh.
- **Phase 8 (Spotify):** Spotify PKCE flow is fully documented. Token lifecycle patterns are clear. The reference implementation's SpotifyAuthManager is the template.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All versions verified against Debian Trixie package repositories directly. No guesswork on version numbers. |
| Features | HIGH | Feature set defined by working v1 codebase + competitor analysis. MVP is clear: feature parity with v1. |
| Architecture | HIGH | Architecture patterns verified against Qt6 official docs, Qt 6.10 deprecation notices, and direct analysis of v1 codebase anti-patterns. |
| Pitfalls | HIGH | 12 of 17 pitfalls are directly sourced from the v1 reference implementation's production experience. Not theoretical — these bugs existed in v1 and have verified fixes. |

**Overall confidence: HIGH**

The research quality here is unusually high because: (1) a working v1 implementation exists and was analyzed directly, (2) all platform versions are pinned to a specific OS release (Trixie), and (3) the most dangerous API changes (libgpiod v1->v2, Spotify OAuth) have high-confidence primary sources.

### Gaps to Address

- **ddcutil Pi 5 compatibility:** MEDIUM confidence only. Verify DDC/CI works on the specific display hardware before committing Phase 9 timeline estimates. The fallback if DDC/CI is broken is a different display power approach.
- **Pi 5 gpiochip4 path:** MEDIUM confidence (forum post, not official docs). Verify with `gpiodetect` on the actual hardware during Phase 7 setup.
- **GnuDB sustainability:** GnuDB is active but has documented funding concerns. The fallback (Discogs tertiary lookup + local TOC data) is sufficient for an offline-usable player, but metadata quality degrades. Monitor during deployment.
- **CD drive model:** The specific USB CD drive's behavior with libcdio-paranoia's speed negotiation (`-405` error) is known to be drive-dependent. Test the installed drive during Phase 5 to confirm the reference implementation's workaround applies.
- **Receiver telnet server:** PROJECT.md notes the receiver may support a telnet server as an event-driven alternative to polling. This is deferred to v1.x but could solve the volume overlay reconciliation problem. Not required for v1.0.

## Sources

### Primary (HIGH confidence)

- Debian Trixie package search (packages.debian.org/trixie) — all version numbers
- Qt 6.8 LTS release docs (doc.qt.io/qt-6/qt-releases.html) — LTS support timeline
- Qt 6.10 documentation — setContextProperty deprecation, qmlRegisterSingletonInstance, qt_add_qml_module
- Spotify OAuth migration blog (2025-02-12) — PKCE requirement, implicit grant deprecation Nov 2025
- Spotify redirect URI docs — http://127.0.0.1 loopback exemption confirmed
- libgpiod documentation (libgpiod.readthedocs.io) — v2 API reference
- Qt QSqlDatabase docs — thread safety constraints
- ALSA PCM API docs — xrun handling, hardware parameters
- Qt HttpServer docs — fully supported module since Qt 6.5
- Reference implementation `~/Code/media-console/src/` — direct analysis of all subsystems, EIO recovery, ring buffer, OAuth flow

### Secondary (MEDIUM confidence)

- Raspberry Pi Forums — Pi 5 uses gpiochip4 (RP1 southbridge)
- ddcutil GitHub issues — Pi 5 setvcp reliability concerns
- libcdio-paranoia GitHub issues — USB drive speed negotiation -405 error
- GnuDB website — active but sustainability concerns noted
- Discogs API docs — rate limits 60/min authenticated
- Raymii.org Qt/QML benchmark — setContextProperty is 23% slower than registered types

### Tertiary (LOW confidence)

- GnuDB long-term sustainability — single source, monitoring required
- ddcutil `--sleep-multiplier` behavior on Pi 5 display — community reports, needs hardware verification

---
*Research completed: 2026-02-28*
*Ready for roadmap: yes*
