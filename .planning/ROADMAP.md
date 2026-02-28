# Roadmap: Media Console v2

## Overview

A 10-phase ground-up rewrite of the Qt6/QML music console application, structured around component dependencies. Foundation and state layer come first to prevent the god-object anti-pattern. Receiver control follows as the dependency root for nearly every feature. The audio pipeline provides shared infrastructure for both CD and FLAC playback. Source-specific subsystems (CD, FLAC, GPIO, Spotify) build on that foundation. Display control, HTTP API, and orchestration wire subsystems together. QML UI and production hardening close the project with full integration and kiosk deployment.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Foundation and Build Infrastructure** - Project skeleton with CMake, interfaces, platform abstraction, composition root, and test framework
- [x] **Phase 2: State Layer and QML Binding Surface** - Thin reactive state objects that QML binds to, registered as singletons
- [x] **Phase 3: Receiver Control** - eISCP TCP connection to Onkyo receiver with volume gesture coalescing and metadata parsing
- [x] **Phase 4: Audio Pipeline** - Unified LocalPlaybackController with ALSA output, audio streams, and playback thread
- [x] **Phase 5: CD Subsystem** - Disc detection, paranoia extraction, three-tier async metadata lookup, and progressive display
- [x] **Phase 6: FLAC Library** - Directory scanning, SQLite-backed library database, hierarchical browse models, and playlist playback
- [x] **Phase 7: GPIO Hardware** - Volume encoder, input encoder, and reed switch monitors via libgpiod v2
- [ ] **Phase 8: Spotify Integration** - OAuth PKCE flow, device transfer, search, playback control, and session takeover
- [ ] **Phase 9: Display, HTTP API, and Orchestration** - Screen timeout state machine, REST API server, PlaybackRouter, and AlbumArtResolver
- [ ] **Phase 10: QML UI and Production Deployment** - All QML components integrated, theme system, kiosk systemd service, and deployment scripts

## Phase Details

### Phase 1: Foundation and Build Infrastructure
**Goal**: A compilable, testable project skeleton where every subsequent component has a defined place to land
**Depends on**: Nothing (first phase)
**Requirements**: FOUND-01, FOUND-02, FOUND-03, FOUND-04, FOUND-05, FOUND-06, FOUND-07, FOUND-08, FOUND-09, FOUND-10
**Success Criteria** (what must be TRUE):
  1. Project compiles with CMake/Ninja on aarch64 targeting Qt6 6.8.2 and a test binary runs successfully
  2. clang-format and clang-tidy run as part of the build workflow and reject non-conforming code
  3. A Google Test suite executes via CTest with at least one test per platform interface using stub implementations
  4. AppBuilder constructs an object graph and returns an AppContext with all platform stubs wired
  5. AppConfig loads settings from an INI file and provides typed access without any code touching QSettings directly
**Plans**: 3 plans in 2 waves

Plans:
- [x] 01-01-PLAN.md — CMake project skeleton, code quality tooling, logging categories, coding standards (`e561041`)
- [x] 01-02-PLAN.md — Platform abstraction interfaces, stub implementations, PlatformFactory (`d36a6a4`)
- [x] 01-03-PLAN.md — AppConfig typed configuration, AppBuilder composition root, Google Test suite (`be964a6`)

### Phase 2: State Layer and QML Binding Surface
**Goal**: QML has a stable, reactive binding surface with thin state objects that expose all properties the UI will need
**Depends on**: Phase 1
**Requirements**: STATE-01, STATE-02, STATE-03, STATE-04, STATE-05
**Success Criteria** (what must be TRUE):
  1. ReceiverState exposes volume, input, power, mute, and metadata as Q_PROPERTYs that emit change signals when updated programmatically
  2. PlaybackState exposes playback mode, position, duration, and track info as Q_PROPERTYs
  3. A QML test harness can bind to all three state objects (ReceiverState, PlaybackState, UIState) registered as singletons and display their values
  4. MediaSource enum is distinct from receiver input hex codes and can be converted between the two
**Plans**: 2 plans in 2 waves

Plans:
- [x] 02-01-PLAN.md — State enums with MediaSource hex code conversion, Q_PROPERTY bags, AppBuilder wiring (`3911c0b`, `9d34813`)
- [x] 02-02-PLAN.md — QML singleton registration and test harness (`92996c8`)

### Phase 3: Receiver Control
**Goal**: The application connects to the Onkyo receiver and provides full control over power, volume, input, mute, and metadata with smooth encoder-driven volume gestures
**Depends on**: Phase 2
**Requirements**: RECV-01, RECV-02, RECV-03, RECV-04, RECV-05, RECV-06, RECV-07, RECV-08, RECV-09, RECV-10, RECV-11, RECV-12, RECV-13, ORCH-03
**Success Criteria** (what must be TRUE):
  1. Application connects to the receiver via eISCP, recovers automatically on network errors, and polls state every 2.5 seconds
  2. Volume encoder events are coalesced into gestures — the UI updates optimistically during the gesture and a single command is sent after the gesture ends
  3. All 6 input sources can be selected and the receiver switches to the correct input
  4. Track metadata (title, artist, album, art URL, playback state) from streaming sources populates ReceiverState and is visible in the UI
  5. Volume overlay appears only for local user input (encoder, touchscreen) and does not appear for external volume changes (Spotify Connect, Onkyo app)
**Plans**: 3 plans in 2 waves

Plans:
- [x] 03-01-PLAN.md — eISCP transport layer: packet framing and TCP auto-reconnect (`0c52870`)
- [x] 03-02-PLAN.md — ReceiverController: command/response parsing, state updates, stale detection (`5aa5f59`)
- [x] 03-03-PLAN.md — VolumeGestureController, CommandSource enum, AppBuilder wiring (`4e9b91e`)

### Phase 4: Audio Pipeline
**Goal**: A unified playback controller that accepts any audio stream (CD or FLAC) and outputs bit-perfect audio through the S/PDIF HAT with robust error recovery
**Depends on**: Phase 2
**Requirements**: AUDIO-01, AUDIO-02, AUDIO-03, AUDIO-04, AUDIO-05, AUDIO-06, AUDIO-07, AUDIO-08, AUDIO-09
**Success Criteria** (what must be TRUE):
  1. LocalPlaybackController plays audio from a test AudioStream through ALSA at 44100Hz/16-bit/stereo to hw:2,0 with audible output on the S/PDIF HAT
  2. Playback controls (play, pause, stop, seek, next, previous) work from the main thread without blocking the UI
  3. ALSA EIO errors trigger automatic device close-reopen recovery with up to 3 retries, and emit audioRecoveryFailed when exhausted
  4. Only one audio source can play at a time — starting a new stream stops the current one architecturally (not by convention)
**Plans**: 3 plans in 3 waves

Plans:
- [x] 04-01-PLAN.md — AudioStream interface, AlsaAudioOutput, AudioConfig, PlatformFactory wiring (`336bd99`)
- [x] 04-02-PLAN.md — AudioRingBuffer, AudioBufferStats, LocalPlaybackController with background thread and playback controls (`525c01e`)
- [x] 04-03-PLAN.md — AppBuilder wiring and integration test for LocalPlaybackController (`c0f10c9`)

### Phase 5: CD Subsystem
**Goal**: Users can insert a CD, see track listing immediately, watch metadata fill in progressively, and play any track with paranoia error correction
**Depends on**: Phase 4
**Requirements**: CD-01, CD-02, CD-03, CD-04, CD-05, CD-06, CD-07, CD-08, CD-09, CD-10, CD-11, CD-12
**Success Criteria** (what must be TRUE):
  1. Inserting a CD shows a track listing with durations within seconds, and metadata (artist, album, track titles, album art) fills in asynchronously without freezing the UI
  2. Playing a CD track produces audio through the S/PDIF HAT with paranoia error correction and seek works at sector granularity
  3. A previously-seen disc loads metadata instantly from SQLite cache without network requests
  4. Removing a CD stops playback and clears the track listing; reinserting triggers fresh detection
  5. All metadata network I/O (MusicBrainz, GnuDB, Discogs) runs fully async with no main thread blocking under any failure condition
**Plans**: 4 plans in 3 waves

Plans:
- [x] 05-01-PLAN.md — LibcdioCdDrive with libcdio/libdiscid, CdAudioStream with paranoia extraction (`7a3840c`, `ffe041b`)
- [x] 05-02-PLAN.md — CdMetadataCache SQLite, CdMetadataProvider three-tier lookup, CdAlbumArtProvider (`e4427eb`, `b927c0c`)
- [x] 05-03-PLAN.md — CdController lifecycle orchestrator: detection, metadata chain, idle timer (`0fa594c`)
- [x] 05-04-PLAN.md — AppBuilder wiring: CdController into composition root and AppContext (`33e4830`)

### Phase 6: FLAC Library
**Goal**: Users can browse a local FLAC library by Artist, Album, and Track, and play any track or album with next/previous navigation
**Depends on**: Phase 4
**Requirements**: FLAC-01, FLAC-02, FLAC-03, FLAC-04, FLAC-05, FLAC-06, FLAC-07, FLAC-08
**Success Criteria** (what must be TRUE):
  1. Library scanner discovers FLAC files recursively, extracts metadata via TagLib, and populates a SQLite database with incremental scanning (unchanged files skipped)
  2. Three hierarchical QAbstractListModel subclasses provide Artist, Album, and Track data to QML for drill-down browsing
  3. Playing a track from the library produces audio through the S/PDIF HAT with correct sample rate conversion to 44100Hz
  4. Album art is extracted from FLAC picture blocks and cached to disk with correct filenames and MIME types
**Plans**: 4 plans in 3 waves

Plans:
- [x] 06-01-PLAN.md — FlacAudioStream (libsndfile+libsamplerate) and LibraryDatabase (SQLite) (`cce6332`)
- [x] 06-02-PLAN.md — LibraryScanner (TagLib+QtConcurrent) and LibraryAlbumArtProvider (FLAC picture blocks) (`7160ecd`)
- [x] 06-03-PLAN.md — Three browse models (Artist, Album, Track) and FlacLibraryController orchestrator (`c608c2b`)
- [x] 06-04-PLAN.md — Wire FlacLibraryController into AppBuilder and AppContext (`4da3730`)

### Phase 7: GPIO Hardware
**Goal**: Physical rotary encoders and reed switch drive the application — volume knob controls volume, input knob switches sources, door controls display
**Depends on**: Phase 3
**Requirements**: GPIO-01, GPIO-02, GPIO-03, GPIO-04, GPIO-05, GPIO-06
**Success Criteria** (what must be TRUE):
  1. Turning the volume encoder changes receiver volume smoothly with gesture coalescing (no event flooding)
  2. Turning the input encoder steps through sources one-per-detent and the push button toggles mute on exactly one edge (no double-toggle)
  3. Reed switch state changes (door open/close) are debounced and drive display power state
  4. All GPIO monitors run in background threads and the application runs with no-op stubs on non-Linux platforms
**Plans**: 3 plans in 3 waves

Plans:
- [x] 07-01-PLAN.md — IGpioMonitor interface evolution (volumeChanged delta signal), UIState doorOpen, GpioConfig, CMake libgpiodcxx (`11753bb`, `45d4acb`)
- [x] 07-02-PLAN.md — QuadratureDecoder and LinuxGpioMonitor implementation (libgpiod v2 edge monitoring) (`cb6cd1b`)
- [x] 07-03-PLAN.md — PlatformFactory HAS_GPIOD conditional, AppBuilder GPIO signal wiring (`fcb44ca`, `626d2c1`)

### Phase 8: Spotify Integration
**Goal**: Users can authenticate with Spotify, search for music, transfer playback to the receiver, and manage active sessions
**Depends on**: Phase 3
**Requirements**: SPOT-01, SPOT-02, SPOT-03, SPOT-04, SPOT-05, SPOT-06, SPOT-07, SPOT-08
**Success Criteria** (what must be TRUE):
  1. User completes Spotify OAuth PKCE flow via browser, tokens persist across restarts, and auto-refresh happens before expiry
  2. Playback transfers to the receiver by device name and play/pause/next/previous controls work
  3. Full-text search returns tracks, artists, and albums with album art displayed
  4. When Spotify is already playing on another device, a takeover dialog shows the current device and track before transferring
**Plans**: TBD

Plans:
- [ ] 08-01: TBD
- [ ] 08-02: TBD
- [ ] 08-03: TBD

### Phase 9: Display, HTTP API, and Orchestration
**Goal**: Screen dims and powers off on inactivity, an HTTP API enables remote control, and PlaybackRouter eliminates source-routing duplication
**Depends on**: Phase 3, Phase 4, Phase 7, Phase 8
**Requirements**: DISP-01, DISP-02, DISP-03, DISP-04, DISP-05, DISP-06, DISP-07, DISP-08, API-01, API-02, API-03, API-04, API-05, ORCH-01, ORCH-02
**Success Criteria** (what must be TRUE):
  1. Display dims after 5 minutes of inactivity, powers off after 20 minutes, and any touch or door-open event brings it back immediately
  2. Display timeout is suppressed during active music playback so the screen stays on while music plays
  3. HTTP API responds to volume, input, status, and display power requests on the configured port
  4. Spotify OAuth setup page and search endpoint are accessible via HTTP API
  5. PlaybackRouter dispatches play/pause/stop/next/previous/seek to the correct controller based on active source without duplicated if/else chains
**Plans**: TBD

Plans:
- [ ] 09-01: TBD
- [ ] 09-02: TBD
- [ ] 09-03: TBD

### Phase 10: QML UI and Production Deployment
**Goal**: The complete touch interface runs as a kiosk on the 1920x720 display with all components visually integrated and production-ready
**Depends on**: Phase 2, Phase 3, Phase 5, Phase 6, Phase 8, Phase 9
**Requirements**: UI-01, UI-02, UI-03, UI-04, UI-05, UI-06, UI-07, UI-08, UI-09, UI-10, UI-11, UI-12, UI-13, UI-14, UI-15, UI-16, UI-17, UI-18, PROD-01, PROD-02, PROD-03
**Success Criteria** (what must be TRUE):
  1. Main layout renders correctly on 1920x720 with left panel (input carousel), right panel (NowPlaying or LibraryBrowser), and top status bar
  2. InputCarousel shows 3D perspective carousel with 6 inputs, auto-selects after 4 seconds, and responds to encoder navigation
  3. NowPlaying displays album art, track info, progress bar with seek, and playback controls for all sources (CD, FLAC, Spotify, Bluetooth)
  4. LibraryBrowser provides drill-down from Artists to Albums to Tracks with quick-scroll sidebar and SpotifySearch provides fullscreen search with on-screen keyboard
  5. Application starts automatically on boot via systemd, auto-restarts on crash, runs with hidden cursor, and can be installed/uninstalled with deployment scripts

**Plans**: TBD

Plans:
- [x] 10-01-PLAN.md --- Theme.qml singleton, production main.qml layout, QML singleton registrations (`c17da97`)
- [ ] 10-02: TBD
- [ ] 10-03: TBD
- [ ] 10-04: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9 -> 10

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Foundation and Build Infrastructure | 3/3 | Complete | 2026-02-28 |
| 2. State Layer and QML Binding Surface | 2/2 | Complete | 2026-02-28 |
| 3. Receiver Control | 3/3 | Complete | 2026-02-28 |
| 4. Audio Pipeline | 3/3 | Complete | 2026-02-28 |
| 5. CD Subsystem | 4/4 | Complete | 2026-02-28 |
| 6. FLAC Library | 4/4 | Complete | 2026-02-28 |
| 7. GPIO Hardware | 3/3 | Complete | 2026-02-28 |
| 8. Spotify Integration | 3/3 | Complete | 2026-02-28 |
| 9. Display, HTTP API, and Orchestration | 3/3 | Complete | 2026-02-28 |
| 10. QML UI and Production Deployment | 3/4 | In Progress|  |
