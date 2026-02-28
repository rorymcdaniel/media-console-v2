# Media Console v2

## What This Is

A ground-up rewrite of a Qt6/QML music console application that runs on a Raspberry Pi 5, controlling an Onkyo TX-8260 receiver via a 1920x720 touchscreen with physical rotary encoders and an integrated CD player. The rewrite delivers the same core functionality with a cleaner architecture, eliminating structural problems that accumulated during organic development of the original ~20,600-line codebase.

## Core Value

The touchscreen kiosk must always be responsive — no freezes, no unrecoverable states. A visitor should be able to walk up, select a source, and hear music within seconds.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Receiver control via eISCP protocol (volume, input, power, mute, metadata)
- [ ] CD playback with native ALSA/libcdio audio streaming
- [ ] CD metadata lookup (MusicBrainz → GnuDB → Discogs) with progressive display
- [ ] CD disc monitoring with hybrid detection
- [ ] FLAC library browsing (Artist → Album → Track) and playback
- [ ] Spotify integration (OAuth, device transfer, search, playback control)
- [ ] Bluetooth input with receiver-provided metadata display
- [ ] GPIO hardware integration (volume encoder, input encoder, reed switch)
- [ ] Display control (DDC/CI power, dimming, screen timeout state machine)
- [ ] HTTP API server (volume, input, status, display, Spotify OAuth)
- [ ] QML UI with full touch interface on 1920x720 display
- [ ] Production deployment with kiosk mode systemd service

### Out of Scope

- CD auto-play on insert — always user-initiated, no configuration option
- Bluetooth album art fetching via Discogs — display receiver metadata only
- StreamingAlbumArtFetcher — no external art fetching for streaming sources
- Spotify settings legacy migration — use canonical lowercase keys from start
- Spotify album art via API — deferred, receiver CGI art is sufficient
- Album art cache-busting timestamps — deferred until stale art observed
- Service name caching — deferred until "Unknown (FF)" flashing is a real problem
- Lidarr / SpotiFLAC Bridge — deferred to optional late phase
- Feature expansion — this is a structural rewrite, not new functionality

## Context

### Reference Implementation

The original working codebase is at `~/Code/media-console` (~15,400 lines C++, ~5,200 lines QML). It defines correct behavior and should be referenced for:
- Execution path tracing and edge cases
- Domain logic lifting (eISCP parsing, ALSA config, CD audio streaming, metadata fetching, GPIO monitors)
- Feature parity verification

### Critical Domain Knowledge

1. **Volume encoder (smooth/detentless)**: PEC11R-4020F-S0024 generates continuous event streams. Must treat input as gesture — coalesce events, send single command after gesture ends, reconcile with receiver after.
2. **CD + Library share receiver input "02"**: Both use S/PDIF via Nvdigi HAT. Unified LocalPlaybackController solves this architecturally.
3. **ALSA device exclusivity**: One source at a time. Single controller handles this.
4. **CD metadata can freeze the app**: ALL network I/O must be fully async. #1 reliability concern for kiosk.
5. **CD metadata should show progressively**: Show TOC immediately, fill in metadata asynchronously.
6. **GnuDB response validation**: Validate before caching — reject malformed responses.
7. **ALSA configuration**: Working params (hw:2,0, 44100Hz, 16-bit, stereo) are hard-won — preserve them.
8. **eISCP metadata timing**: Fields arrive separately, not atomically. Buffer partial data.
9. **Spotify sets volume to 40**: Unsolicited receiver volume changes should update silently, no overlay.
10. **Volume overlay**: Only for local user input (encoder, touchscreen). External changes are silent.
11. **Mute button double-toggle bug**: GPIO push switch triggers on both edges. Must trigger on ONE edge only.
12. **Receiver telnet server**: Research as potential event-driven alternative to eISCP polling.

### Target Hardware

- Raspberry Pi 5 (4GB RAM), Raspberry Pi OS (Debian 13 Trixie, 64-bit)
- 1920x720 touchscreen with DDC/CI
- Onkyo TX-8260 (eISCP TCP port 60128)
- Nvdigi HAT (S/PDIF digital audio)
- External USB CD drive
- Volume encoder: PEC11R-4020F-S0024 (24 pulses/rev, smooth, no detents)
- Input encoder: PEC11R-4320F-S0012 (12 pulses/rev, 12 detents)
- Magnetic reed switch for door/display auto-control

## Constraints

- **Tech stack**: C++17, Qt6, QML, CMake/Ninja — unchanged from original
- **Platform**: Linux-only native deps (libgpiod, libcdio, libcdio-paranoia, ALSA, TagLib, libsndfile, libsamplerate)
- **Display**: Fixed 1920x720 resolution, touch-only input plus encoders
- **Hardware**: Must work on Raspberry Pi 5 with 4GB RAM
- **Architecture**: Decomposed AppState, unified playback controller, composition root, platform abstraction via runtime injection
- **Testing**: Google Test required, mock interfaces for hardware deps
- **Code quality**: clang-format, clang-tidy, no business logic in AppState
- **Scope**: Structural rewrite only — no feature expansion

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Decompose AppState into thin state + focused objects | Original is 1,400-line god object doing 7 jobs | — Pending |
| Separate MediaSource enum from receiver input codes | Original conflates user intent with hardware addressing | — Pending |
| Unify CD/FLAC into single LocalPlaybackController | Structurally identical controllers, solves ALSA exclusivity | — Pending |
| Composition root (AppBuilder) replaces 325-line main.cpp | Clear ownership, testable wiring | — Pending |
| Centralized AppConfig struct | Replace scattered QSettings reads | — Pending |
| Platform abstraction via runtime injection | Enables testing on macOS, replaces #ifdef guards | — Pending |
| Google Test + mock interfaces | Platform-independent testing of hardware-dependent code | — Pending |
| Research receiver telnet server | May solve volume overlay problem and enable event-driven updates | — Pending |

---
*Last updated: 2026-02-28 after initialization*
