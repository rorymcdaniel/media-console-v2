# Feature Research

**Domain:** Embedded music console / kiosk media player
**Researched:** 2026-02-28
**Confidence:** HIGH (feature set defined by working v1 codebase + competitor analysis)

## Feature Landscape

### Table Stakes (Users Expect These)

Features the console must have on day one. Without these, the device does not function as a music console.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Receiver power on/off | Cannot use system without turning on receiver | LOW | eISCP `system-power` command. Must enable "Network Control" on receiver for standby wake. |
| Volume control (touch + encoder) | Primary interaction. Must feel instant and smooth | MEDIUM | Smooth encoder (PEC11R-4020F-S0024) generates continuous events. Requires gesture coalescing -- treat encoder spin as a gesture, send single command after gesture ends, reconcile with receiver response. |
| Volume overlay (local input only) | Visual feedback for volume changes. Users expect to see the level | LOW | Show overlay ONLY for local user input (encoder, touchscreen). Silent update for external changes (e.g., Spotify sets volume to 40). |
| Input source selection | Users need to switch between Spotify, CD, Phono, Bluetooth, Library, Computer | MEDIUM | 3D carousel overlay with auto-select after timeout. Six inputs in v1. Must map MediaSource enum cleanly to receiver input codes. CD and Library share receiver input "02" via S/PDIF. |
| Now Playing display | Core screen when music is playing. Album art, track title, artist, album | MEDIUM | Large album art (left panel), metadata (right panel). Layout must handle missing data gracefully -- Bluetooth has no album art, Phono has no metadata. |
| Playback controls (play/pause/prev/next) | Standard transport controls for CD, Library, Spotify | LOW | Map to appropriate backend per source. CD and Library use local ALSA playback. Spotify uses Web API. |
| Track progress bar | Users expect to see elapsed/total time and scrub position | LOW | Hide for Bluetooth (AVRCP 1.3 does not reliably provide duration). Show for CD, Library, Spotify. |
| Mute toggle | Must be instant. Physical button and touch | LOW | GPIO push switch triggers on both edges -- must fire on ONE edge only to prevent double-toggle bug. |
| Receiver connection status | Users need to know if receiver is connected/disconnected | LOW | Error banner shown when receiver drops. Auto-reconnect. eISCP TCP connection monitoring. |
| CD playback with disc detection | Insert disc, press play, hear music. Core use case | HIGH | Hybrid disc detection (polling + event). ALSA audio streaming at 44100Hz 16-bit stereo via hw:2,0. libcdio/libcdio-paranoia for audio extraction. Single LocalPlaybackController handles ALSA exclusivity. |
| CD metadata display (progressive) | Show disc info. Users expect artist/album/track names | HIGH | Three-tier lookup: MusicBrainz (primary, best data) then GnuDB (fallback, validate responses before caching) then Discogs. Show TOC immediately, fill metadata asynchronously. ALL network I/O must be async -- #1 reliability concern. |
| FLAC library browsing | Navigate local music collection by Artist then Album then Track | MEDIUM | SQLite-backed library database. TagLib for metadata extraction. LibraryScanner indexes on startup. Hierarchical navigation: Artist list then Album list then Track list. |
| FLAC library playback | Play local FLAC files through receiver | HIGH | Shares LocalPlaybackController with CD. ALSA device exclusivity (one source at a time). libsndfile for decoding, libsamplerate for resampling if needed. |
| Spotify integration (OAuth + device transfer) | Spotify is dominant streaming platform. Table stakes for any modern music player | HIGH | OAuth PKCE flow via HTTPS API endpoint (/setup/spotify). Device transfer to receiver. Search, browse, playback control via Spotify Web API. Must handle "phone session" detection (Spotify playing from another device). |
| Bluetooth input with metadata | Connect phone, play audio. Display what receiver reports | LOW | Receiver handles A2DP audio. Console displays AVRCP metadata forwarded by receiver via eISCP. No album art fetching (receiver metadata only). Pairing mode visual state with 60s timeout. |
| Display power management | Screen should dim and turn off when idle to save power and reduce burn-in | MEDIUM | State machine: ACTIVE (full brightness) then DIMMED (25%, 5 min) then OFF (DDC/CI power off, 20 min). Reset on any touch. DDC/CI via ddcutil. Must handle DOOR_CLOSED state from reed switch. |
| HTTP API for external control | Home automation integration (Home Assistant, scripts) | MEDIUM | Endpoints: POST /api/volume, POST /api/input, POST /api/display, GET /api/status. Plus Spotify OAuth endpoints. QHttpServer with optional HTTPS (self-signed cert for Spotify). |
| Full-screen kiosk mode | Must boot directly into app, no desktop, no escape | LOW | systemd service, Qt FramelessWindowHint, fullscreen. 1920x720 fixed resolution. |
| Touch-friendly UI sizing | All targets must be finger-operable on 1920x720 screen | LOW | Minimum 44x44px touch targets per accessibility guidelines. Generous spacing between interactive elements. Visual feedback on touch. |
| Toast notifications | Brief feedback for actions (input changed, CD ejected, etc.) | LOW | Non-modal, auto-dismiss. Must not block interaction. |
| Error recovery | Kiosk must never get stuck. Graceful degradation | MEDIUM | Audio error dialog with retry. Receiver reconnection. Metadata fetch timeouts. No unrecoverable states. |

### Differentiators (Competitive Advantage)

Features that set this console apart from Volumio, MoOde, and other Raspberry Pi audio players. These are not expected but add significant value.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Physical hardware integration (dual encoders + reed switch) | Tactile, immediate control without touching screen. Detentless volume encoder feels premium. Door-triggered display control is invisible UX | MEDIUM | Most competitors are software-only with web UI. This has dedicated hardware: smooth volume encoder, detented input encoder, magnetic reed switch. The physical interface is the primary differentiator over web-based players like Volumio/MoOde. |
| Native Qt6/QML application (not web UI) | 60fps animations, instant touch response, no browser overhead. True kiosk performance | HIGH | Competitors use web UIs (Chromium/browser-based). Native QML gives GPU-accelerated rendering, smooth carousel transitions, and zero page-load latency. Critical for the "walk up and use immediately" requirement. |
| 3D input carousel | Visually distinctive source selection. Feels like a premium appliance, not a settings menu | LOW | Perspective-transformed carousel with 6 inputs. Auto-selects after 4s timeout. Not available in any competitor. |
| Integrated receiver control | Single interface controls both the player AND the amplifier. No separate remote needed | MEDIUM | Competitors play audio but don't control the receiver. This console replaces the Onkyo remote entirely: power, volume, input, mute. eISCP gives full bidirectional control. |
| Spotify search with on-screen keyboard | Search and play Spotify directly from the touchscreen without a phone | MEDIUM | Most Raspberry Pi players only support Spotify Connect (phone initiates). This console has a built-in search UI with a custom touchscreen keyboard, allowing standalone Spotify browsing. |
| Progressive CD metadata loading | Instant TOC display then metadata fills in asynchronously. No blank screen waiting for network | MEDIUM | Competitors either block on metadata lookup (freezing UI) or show nothing until lookup completes. Progressive display gives immediate feedback and never freezes. |
| Spotify takeover dialog | Graceful handling when Spotify is playing from another device. Ask user whether to transfer playback | LOW | SpotifyTakeoverDialog detects "phone session" and offers to transfer. No competitor handles this edge case. |
| Door-triggered display control | Opening/closing a cabinet door automatically controls the display | LOW | Reed switch GPIO triggers DOOR_CLOSED state, bypassing normal timeout. Unique to physical installations. |
| Eject button in UI | Touch-screen CD eject without reaching for the drive | LOW | Simple but thoughtful. Physical drive may be recessed or hard to reach in cabinet installation. |
| Unified local playback controller | CD and FLAC share one audio pipeline. Clean ALSA exclusivity | HIGH | Architectural differentiator. Eliminates the "two players fighting over audio device" problem. Not user-visible but prevents an entire class of bugs. |

### Anti-Features (Commonly Requested, Often Problematic)

Features to explicitly NOT build. These are documented in PROJECT.md "Out of Scope" or would compromise the core value.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| CD auto-play on insert | "Convenience" -- just insert and listen | Violates user-initiated-only principle. Unexpected audio is worse than one extra tap. Can startle visitors. Cannot be undone once audio starts playing through the amplifier. | Always show CD metadata screen. User taps play. |
| Bluetooth album art via Discogs | Empty art space looks incomplete | Discogs lookup is unreliable for streaming tracks, adds network dependency for a display-only feature, and receiver AVRCP metadata is often incomplete (wrong artist for compilations). False art is worse than no art. | Show Bluetooth icon or gradient placeholder. Display receiver-provided metadata only. |
| External album art fetching for streaming | "Album art everywhere" | Adds fragile external API dependency. Receiver CGI art endpoint already provides art when available. Additional fetching adds latency and failure modes for marginal improvement. | Use receiver-provided album art via CGI endpoint. |
| Multi-room / multi-zone | Competitors have it (Volumio Premium) | Massively increases complexity. This is a single-room console controlling one receiver. Multi-room requires sync protocols, zone management, and UI for zone selection. | Single room, single receiver. Use separate instances for other rooms. |
| Web radio / internet radio | "All the competitors have it" | Adds streaming infrastructure, station management, URL handling, metadata parsing for streams. Low usage in a home with Spotify. Scope creep. | Use Spotify for discovery. Use Bluetooth from phone for niche radio apps. |
| Equalizer / DSP processing | "Audiophile feature" | Adds audio processing pipeline complexity. The Onkyo TX-8260 has its own DSP. Processing audio before S/PDIF output adds latency and potential quality issues. | Use receiver's built-in tone controls and sound modes. |
| Gapless playback (CD/FLAC) | "Essential for live albums" | Requires pre-buffering next track, crossfade logic, and careful ALSA buffer management. Significant complexity for a niche use case on a kiosk. | Standard track-by-track playback. Acceptable gap between tracks. |
| TIDAL / Qobuz / other streaming | "More streaming options" | Each streaming service requires separate OAuth, API integration, and UI. Spotify has dominant market share. Adding more fragments the UX. | Spotify for streaming. Bluetooth as passthrough for any other service from phone. |
| Remote web UI / mobile app | "Control from the couch" | Building a second UI (web or mobile) doubles development and testing surface. The console IS the interface -- it's an appliance, not a server. | HTTP API enables Home Assistant integration for basic remote control (volume, input, power). The touchscreen is the primary interface. |
| Lidarr / SpotiFLAC Bridge | Automated music acquisition | Complex integration with external services. Niche use case. Adds maintenance burden for Lidarr API changes. | Defer to optional late phase per PROJECT.md. Manual library management. |
| Album art cache-busting | "Art sometimes gets stale" | Adds timestamp parameters to art URLs, complicating caching. Not observed as a real problem yet. | Defer until stale art is actually observed in production per PROJECT.md. |
| Service name caching | "Unknown (FF) flashing" | Adds caching layer for service name resolution. Not confirmed as a real user-visible problem. | Defer until "Unknown (FF)" flashing is actually observed per PROJECT.md. |
| Settings / configuration UI | "Users should configure the system" | This is an appliance, not an application. Configuration is deployment-time (config file, systemd environment). Adding settings UI adds screens, state management, and edge cases. | AppConfig struct loaded at startup. Change config file and restart for changes. |
| Screensaver / clock mode | "Show something when idle" | Adds another display mode with its own state management. The display power management (dim then off) already handles idle state. A clock/screensaver keeps the display on, increasing power usage and burn-in risk. | Dim to 25% then power off via DDC/CI. Touch to wake. |

## Feature Dependencies

```
Receiver Control (eISCP)
    |-- Volume control (requires eISCP connection)
    |-- Input selection (requires eISCP connection)
    |-- Power control (requires eISCP connection)
    |-- Mute toggle (requires eISCP connection)
    |-- Now Playing metadata (eISCP provides track info for streaming/BT)
    |-- Receiver connection status (monitors eISCP TCP socket)

Display Control (DDC/CI)
    |-- Screen timeout manager (requires display control)
    |-- Brightness management (requires display control)
    |-- Door-triggered display (requires display control + reed switch GPIO)

Local Playback Controller (ALSA)
    |-- CD playback (requires ALSA output + libcdio)
    |   |-- CD metadata display (requires CD disc detection)
    |   |   |-- MusicBrainz lookup (requires network + disc ID)
    |   |   |-- GnuDB fallback (requires network + disc ID)
    |   |   |-- Metadata cache (requires SQLite)
    |   |-- CD disc monitoring (requires libcdio + polling)
    |-- FLAC playback (requires ALSA output + libsndfile)
    |   |-- Library browsing (requires library database)
    |   |   |-- Library scanner (requires TagLib + filesystem access)
    |   |   |-- Library database (requires SQLite)

Spotify Integration
    |-- OAuth flow (requires HTTPS API server)
    |   |-- HTTPS server (requires self-signed cert generation)
    |-- Device transfer (requires OAuth tokens + Spotify Web API)
    |-- Search UI (requires OAuth tokens + Spotify Web API)
    |-- Playback control (requires OAuth tokens + Spotify Web API)
    |-- Takeover dialog (requires playback state detection)

GPIO Hardware
    |-- Volume encoder monitor (requires libgpiod)
    |-- Input encoder monitor (requires libgpiod)
    |-- Reed switch monitor (requires libgpiod)
    |   |-- Door-triggered display (requires reed switch + display control)

HTTP API Server
    |-- Volume endpoint (requires receiver control)
    |-- Input endpoint (requires receiver control)
    |-- Display endpoint (requires display control)
    |-- Status endpoint (requires receiver state)
    |-- Spotify OAuth endpoints (requires Spotify auth manager)

QML UI
    |-- Now Playing screen (requires receiver metadata + source state)
    |-- Input carousel (requires input source model)
    |-- Volume overlay (requires volume state + input source detection)
    |-- Library browser (requires library database models)
    |-- Spotify search (requires Spotify auth + Web API)
    |-- Playback controls (requires source-specific backend)
    |-- Error banner (requires connection state)
    |-- Toast notifications (requires event system)
```

### Dependency Notes

- **Local Playback Controller is the linchpin:** Both CD and FLAC playback depend on it, and it enforces ALSA device exclusivity. Must be built before either playback feature.
- **Receiver control is the foundation:** Almost every feature depends on eISCP connectivity. Build and stabilize first.
- **Spotify OAuth requires HTTPS:** The API server with SSL support must be working before Spotify can authenticate.
- **GPIO monitors are independent:** Volume encoder, input encoder, and reed switch can be built in parallel since they use separate GPIO pins.
- **Library scanner and CD metadata are independent:** Both use network/filesystem but don't interact. Can be built in parallel.
- **UI depends on backend state:** QML components bind to backend properties. Backend must expose stable property interfaces before UI work begins.

## MVP Definition

### Launch With (v1.0 -- Feature Parity)

Minimum to replace the v1 console. The physical hardware is already installed. The v1 app is running. v2 must match v1's capabilities before deployment.

- [ ] Receiver control (power, volume, input, mute) via eISCP -- foundation for everything
- [ ] Volume and input encoder GPIO integration -- primary physical interface
- [ ] Now Playing display with album art and metadata -- core screen
- [ ] Input source carousel -- switching between sources
- [ ] CD playback with disc detection and progressive metadata -- core use case
- [ ] FLAC library browsing and playback -- second local source
- [ ] Spotify integration (OAuth, search, device transfer, playback) -- primary streaming
- [ ] Bluetooth metadata display -- passthrough source
- [ ] Display power management (dim/off/door) -- kiosk reliability
- [ ] HTTP API (volume, input, display, status, Spotify OAuth) -- home automation
- [ ] Volume overlay (local input only) -- visual feedback
- [ ] Error handling (receiver reconnect, audio errors, metadata timeouts) -- kiosk reliability
- [ ] Kiosk systemd service deployment -- production operation

### Add After Validation (v1.x)

Features to add once v2 is stable and deployed, driven by observed issues.

- [ ] Album art cache-busting -- add only if stale art is observed in production
- [ ] Service name caching -- add only if "Unknown (FF)" flashing is a real problem
- [ ] Receiver telnet server integration -- research as event-driven alternative to eISCP polling, may solve volume overlay problem

### Future Consideration (v2+)

Features explicitly deferred per PROJECT.md scope rules.

- [ ] Lidarr / SpotiFLAC Bridge -- optional automated music acquisition
- [ ] Additional streaming services -- only if Spotify becomes insufficient
- [ ] Gapless playback -- only if gap between tracks is a real user complaint

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Receiver control (eISCP) | HIGH | MEDIUM | P1 |
| Volume control (encoder + touch) | HIGH | MEDIUM | P1 |
| Input source selection | HIGH | MEDIUM | P1 |
| Now Playing display | HIGH | MEDIUM | P1 |
| CD playback + metadata | HIGH | HIGH | P1 |
| FLAC library browse + play | HIGH | HIGH | P1 |
| Spotify integration | HIGH | HIGH | P1 |
| Display power management | HIGH | MEDIUM | P1 |
| Mute toggle | MEDIUM | LOW | P1 |
| Power control | MEDIUM | LOW | P1 |
| Bluetooth metadata display | MEDIUM | LOW | P1 |
| HTTP API server | MEDIUM | MEDIUM | P1 |
| Input encoder monitor | MEDIUM | MEDIUM | P1 |
| Reed switch / door control | MEDIUM | LOW | P1 |
| Error handling / recovery | HIGH | MEDIUM | P1 |
| Volume overlay | MEDIUM | LOW | P1 |
| Kiosk deployment | HIGH | LOW | P1 |
| Spotify search UI | MEDIUM | MEDIUM | P1 |
| Toast notifications | LOW | LOW | P1 |
| Eject button | LOW | LOW | P1 |
| Spotify takeover dialog | LOW | LOW | P2 |
| Album art cache-busting | LOW | LOW | P3 |
| Service name caching | LOW | LOW | P3 |
| Receiver telnet research | MEDIUM | MEDIUM | P3 |
| Lidarr integration | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for launch (feature parity with v1)
- P2: Should have, add when stable
- P3: Nice to have, future consideration

## Competitor Feature Analysis

| Feature | Volumio | MoOde Audio | HiFiBerryOS | RuneAudio | Media Console v2 |
|---------|---------|-------------|-------------|-----------|-------------------|
| Web-based UI | Yes (primary) | Yes (primary) | Yes (primary) | Yes (primary) | No -- native Qt6/QML |
| Touchscreen support | Plugin (Touch Display) | Browser-based | No native | No native | Native, primary interface |
| Local file playback | Yes (MPD) | Yes (MPD) | Yes (MPD) | Yes (MPD) | Yes (custom ALSA pipeline) |
| CD playback | Yes (Volumio 4) | No | No | No | Yes (core feature) |
| Spotify Connect | Yes (Premium) | Yes | Yes | No | Yes (+ search + device transfer) |
| AirPlay | Yes | Yes | Yes | Yes | No (not needed -- BT covers iOS) |
| Bluetooth input | Yes (Volumio 4) | Yes | No | No | Yes (receiver passthrough) |
| Multi-room | Yes (Premium) | No | No | No | No (single room by design) |
| Internet radio | Yes | Yes | Yes | Yes | No (use Spotify or BT) |
| TIDAL/Qobuz | Yes (Premium) | No | No | No | No (Spotify is sufficient) |
| Receiver control | No | No | No | No | Yes (full eISCP integration) |
| Hardware encoders | No | No | No | No | Yes (volume + input) |
| REST API | Yes | Yes | Limited | No | Yes |
| Home Assistant | Yes (official) | Community | No | No | Via HTTP API |
| DSP/Equalizer | Plugin (FusionDSP) | Yes (CamillaDSP) | Yes (HiFiBerry DSP) | No | No (use receiver DSP) |
| Gapless playback | Partial | Yes | Yes | Yes | No (deferred) |
| DSD support | Yes | Yes | No | Yes | No (not needed) |
| Album art display | Yes | Yes | No | Yes | Yes (prominent, source-aware) |
| Display dim/sleep | Plugin | No | No | No | Yes (state machine with DDC/CI) |
| Physical door sensor | No | No | No | No | Yes (reed switch GPIO) |

### Key Competitive Observations

1. **All competitors are MPD-based web UIs.** Media Console v2 is unique in being a native application with dedicated hardware controls. This is not a "better Volumio" -- it's a different product category (appliance vs. software).

2. **No competitor controls the amplifier/receiver.** This is the single biggest differentiator. Volumio/MoOde play audio, but users still need a separate remote for volume, input, and power. Media Console v2 replaces the remote entirely.

3. **CD playback is rare.** Only Volumio 4 added CD support recently. Most Pi audio players ignore optical media. This is a genuine differentiator for the use case.

4. **Spotify search from the device is rare.** Most competitors only support Spotify Connect (phone-initiated). Having a built-in search UI with on-screen keyboard is a meaningful upgrade.

5. **Display power management is underdeveloped.** Volumio has a touch display plugin with timeout/brightness. No competitor has door-triggered display control or a proper state machine.

## Sources

- [Volumio 4 announcement](https://volumio.com/volumio-4-os/) -- Volumio 4 features including CD playback, Bluetooth rewrite
- [MoOde Audio Player](https://moodeaudio.org/) -- MoOde features: Spotify Connect, AirPlay, REST API, bit-perfect playback
- [HiFiBerryOS](https://www.hifiberry.com/hifiberryos/) -- HiFiBerryOS: minimal streaming distribution, DSP support
- [RuneAudio](https://www.runeaudio.com/) -- RuneAudio features: web UI, bitperfect/gapless, DSD
- [Volumio REST API](https://developers.volumio.com/api/rest-api) -- Volumio API documentation
- [Volumio Home Assistant Integration](https://www.home-assistant.io/integrations/volumio/) -- Home automation integration pattern
- [Volumio Touch Display Plugin](https://github.com/volumio/volumio-plugins-sources/blob/master/touch_display/README.md) -- Touchscreen plugin with screensaver and brightness control
- [MusicBrainz API](https://wiki.musicbrainz.org/MusicBrainz_API) -- CD metadata lookup, REST API with XML/JSON
- [FreeDB to MusicBrainz](https://musicbrainz.org/doc/FreeDB) -- CDDB/FreeDB migration to MusicBrainz
- [Spotify Connect Basics](https://developer.spotify.com/documentation/commercial-hardware/implementation/guides/connect-basics) -- Spotify embedded device integration
- [Spotify Web API](https://developer.spotify.com/documentation/web-api) -- Spotify API for search, playback control
- [Onkyo eISCP Protocol](https://github.com/miracle2k/onkyo-eiscp) -- eISCP command reference and protocol implementation
- [DPMS - ArchWiki](https://wiki.archlinux.org/title/Display_Power_Management_Signaling) -- Linux display power management
- [DDC/CI Display Control - ArchWiki](https://wiki.archlinux.org/title/Display_control) -- DDC/CI for programmatic display control
- [DDC/CI Raspberry Pi Touch Monitor Control](https://kravemir.org/how-to/automatically-turn-off-on-touch-screen-based-on-activity-via-ddc-ci-with-rapsberry-pi/) -- Activity-based DDC/CI display power on Raspberry Pi
- [Rotary Encoder Volume Control](https://gist.github.com/savetheclocktower/9b5f67c20f6c04e65ed88f2e594d43c1) -- GPIO rotary encoder patterns for Raspberry Pi
- [AVIXA Kiosk UX Design Checklist](https://xchange.avixa.org/posts/kiosk-ux-ui-design-checklist) -- Touch kiosk interface design patterns
- [AVRCP Bluetooth Metadata](https://en.wikipedia.org/wiki/List_of_Bluetooth_profiles) -- AVRCP 1.3+ metadata capabilities

---
*Feature research for: Embedded music console / kiosk media player*
*Researched: 2026-02-28*
