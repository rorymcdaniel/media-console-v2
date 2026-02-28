---
phase: 09-display-http-api-and-orchestration
verified: 2026-02-28T22:00:00Z
status: human_needed
score: 15/15 must-haves verified
re_verification: true
  previous_status: gaps_found
  previous_score: 14/15
  gaps_closed:
    - "API-04 — GET /spotify/search implemented at HttpApiServer.cpp lines 474-499, calls SpotifyController::search(query), test SpotifySearchWhenNotAuthenticatedReturnsError added"
  gaps_remaining: []
  regressions: []
human_verification:
  - test: "Run on target Raspberry Pi hardware with ddcutil installed"
    expected: "LinuxDisplayControl autoDetects display bus, setBrightness controls backlight, setPower controls DPMS"
    why_human: "ddcutil calls require real DDC/CI-capable display; all macOS tests exercise guard/failure paths only"
  - test: "Access http://{pi-ip}:8080/setup/spotify in browser"
    expected: "HTML page loads with auth instructions; Spotify OAuth flow begins"
    why_human: "Requires live network, real Spotify credentials, and OAuth redirect"
  - test: "Leave kiosk idle for dim_timeout_seconds with no input"
    expected: "Screen gradually dims, then powers off after off_timeout_seconds"
    why_human: "Timer behaviour and display transitions require real hardware and real elapsed time"
---

# Phase 9: Display, HTTP API, and Orchestration — Verification Report

**Phase Goal:** Screen dims and powers off on inactivity, an HTTP API enables remote control, and PlaybackRouter eliminates source-routing duplication
**Verified:** 2026-02-28T22:00:00Z
**Status:** human_needed
**Re-verification:** Yes — after gap closure (previous status: gaps_found, previous score: 14/15)

---

## Re-Verification Summary

**Gap closed:** API-04 (Spotify search endpoint) is now implemented.

- `GET /spotify/search` route registered at `src/api/HttpApiServer.cpp` lines 474-499
- Route parses `?q=` query parameter, validates authentication, calls `m_spotifyController->search(searchQuery)` (line 495)
- `SpotifyController::search(const QString& query)` exists in `src/spotify/SpotifyController.h` line 36
- Test `SpotifySearchWhenNotAuthenticatedReturnsError` added at `tests/test_HttpApiServer.cpp` line 441
- All 9 routes now registered: POST /api/volume, POST /api/input, POST /api/display, GET /api/status, GET /setup/spotify, GET /auth/spotify/callback, GET /spotify/status, GET /spotify/search, POST /spotify/play
- REQUIREMENTS.md marks API-04 Complete — now accurate

**Regressions:** None. All previously-verified artifacts unchanged (line counts match prior verification).

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | LinuxDisplayControl sets brightness via ddcutil setvcp 10 and power via setvcp d6 | VERIFIED | `LinuxDisplayControl.cpp` lines 99-100 (`setvcp 10 {percent}`) and 67 (`setvcp d6 {01|04}`) |
| 2 | LinuxDisplayControl auto-detects display bus via ddcutil detect --brief | VERIFIED | `LinuxDisplayControl.cpp` lines 16-52: parses `/dev/i2c-(\d+)` regex from detect output |
| 3 | ScreenTimeoutController transitions ACTIVE -> DIMMED -> OFF on timer expiry | VERIFIED | `ScreenTimeoutController.cpp`: onDimTimeout() -> Dimming, onDimStep() -> Dimmed, onOffTimeout() -> Off. 14 passing tests |
| 4 | Activity events (touch, door open, encoder, API call) reset state to ACTIVE | VERIFIED | `activityDetected()` in STC restores power+brightness. GPIO signals connected in AppBuilder lines 140-147. HttpApiServer.activityDetected() connected at AppBuilder line 174 |
| 5 | Active music playback suppresses screen timeout | VERIFIED | `onPlaybackModeChanged()` sets m_playbackSuppressing, stops all timers when Playing |
| 6 | Door close triggers immediate DIMMED -> OFF transition | VERIFIED | `onDoorOpenChanged(false)` starts immediate dim step with m_doorCloseMode=true, uses 2s off delay |
| 7 | Configurable dim timeout, off timeout, and dim brightness from AppConfig | VERIFIED | Constructor reads config.dimTimeoutSeconds*1000, offTimeoutSeconds*1000, dimBrightness, timeoutEnabled |
| 8 | HTTP API starts on configurable port and responds to requests | VERIFIED | HttpApiServer.start() listens on m_port, QHttpServer bound to QTcpServer. 26+ tests pass |
| 9 | POST /api/volume, POST /api/input, POST /api/display, GET /api/status functional | VERIFIED | All 4 routes implemented in setupRoutes() with full JSON parsing, controller delegation, error responses |
| 10 | Spotify OAuth HTML pages served (GET /setup/spotify, GET /auth/spotify/callback) | VERIFIED | Both routes implemented with HTML responses (lines 368-457). SpotifyAuth::startAuthFlow() called on setup |
| 11 | GET /spotify/status and POST /spotify/play functional | VERIFIED | Both routes implemented (lines 460-518). Status returns `{"authenticated": bool}`, play checks auth and calls SpotifyController::play() |
| 12 | GET /spotify/search functional (API-04) | VERIFIED | Route registered at lines 474-499. Parses `?q=` param, validates auth, calls `m_spotifyController->search(searchQuery)`. Test at line 441 |
| 13 | Optional HTTPS via self-signed certificate when Qt SSL available | VERIFIED | Full SSL setup in setupSSL() / generateSelfSignedCertificate() under #ifdef QT_SSL_AVAILABLE |
| 14 | PlaybackRouter dispatches play/pause/stop/next/previous/seek to correct controller | VERIFIED | Switch-based dispatch in PlaybackRouter.cpp covering Streaming (Spotify), CD, Library (HAS_SNDFILE guard), no-ops for Phono/BT/Computer/None |
| 15 | AppBuilder constructs and wires all Phase 9 objects | VERIFIED | AppBuilder.cpp lines 135-207: ScreenTimeoutController, PlaybackRouter, AlbumArtResolver, HttpApiServer constructed, connected, started, and exposed via AppContext |

**Score:** 15/15 truths verified

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/display/LinuxDisplayControl.h` | DDC/CI display control via ddcutil | VERIFIED | Implements all IDisplayControl virtual methods |
| `src/display/LinuxDisplayControl.cpp` | Subprocess calls to ddcutil | VERIFIED | QProcess-based autoDetectDisplay, setBrightness, setPower; 129 lines |
| `src/display/ScreenTimeoutController.h` | ACTIVE/DIMMED/OFF state machine | VERIFIED | 4-state enum, all private slots declared, Q_PROPERTY screenState |
| `src/display/ScreenTimeoutController.cpp` | Timer-based state transitions | VERIFIED | Full implementation with dimming animation, playback suppression, door sensor; 279 lines |
| `src/api/HttpApiServer.h` | HTTP API server with REST routes | VERIFIED | All dependencies declared, activityDetected signal, optional SSL under ifdefs |
| `src/api/HttpApiServer.cpp` | Route implementations for all 9 endpoints | VERIFIED | All 9 routes registered including GET /spotify/search (previously missing); 651 lines |
| `src/orchestration/PlaybackRouter.h` | Unified playback dispatch | VERIFIED | Q_INVOKABLE methods, all controller pointers, source change slot |
| `src/orchestration/PlaybackRouter.cpp` | Source-aware command routing | VERIFIED | Switch dispatch for all 6 commands, source change auto-stop; 281 lines |
| `src/orchestration/AlbumArtResolver.h` | Source-aware album art resolution | VERIFIED | Q_PROPERTY albumArtUrl with NOTIFY signal |
| `src/orchestration/AlbumArtResolver.cpp` | Delegates to correct art provider | VERIFIED | Signal-driven resolve() for all sources, initial resolve in constructor; 68 lines |
| `tests/test_LinuxDisplayControl.cpp` | Unit tests for display control | VERIFIED | 6 tests, all pass (guard paths on macOS without ddcutil); 80 lines |
| `tests/test_ScreenTimeoutController.cpp` | Unit tests for state machine | VERIFIED | 14 tests covering all transitions, door, playback suppression; 263 lines |
| `tests/test_HttpApiServer.cpp` | Unit tests for HTTP API | VERIFIED | 26+ tests including SpotifySearchWhenNotAuthenticatedReturnsError; 451 lines |
| `tests/test_PlaybackRouter.cpp` | Unit tests for playback dispatch | VERIFIED | 11 tests covering null safety, no-op sources, source switching; 156 lines |
| `tests/test_AlbumArtResolver.cpp` | Unit tests for art resolution | VERIFIED | 10 tests covering per-source resolution and signal reactivity; 171 lines |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `ScreenTimeoutController.cpp` | `IDisplayControl` | `setBrightness()` / `setPower()` | WIRED | `m_displayControl->setBrightness()` at lines 173, 208; `m_displayControl->setPower()` at lines 70, 149 |
| `ScreenTimeoutController.cpp` | `PlaybackState::playbackModeChanged` | `onPlaybackModeChanged` slot | WIRED | `connect(m_playbackState, &PlaybackState::playbackModeChanged, ...)` at line 42-44 |
| `ScreenTimeoutController.cpp` | `UIState::doorOpenChanged` | `onDoorOpenChanged` slot | WIRED | `connect(m_uiState, &UIState::doorOpenChanged, ...)` at line 49-50 |
| `HttpApiServer.cpp` | `ReceiverController` | `setVolume`, `selectInput` | WIRED | `m_receiverController->setVolume()` line 202; `m_receiverController->selectInput()` line 236 |
| `HttpApiServer.cpp` | `SpotifyAuth` | `startAuthFlow`, `isAuthenticated` | WIRED | `m_spotifyAuth->startAuthFlow()` line 379; `m_spotifyAuth->isAuthenticated()` lines 479, 470 |
| `HttpApiServer.cpp` | `IDisplayControl` | `setPower`, `brightness` | WIRED | `m_displayControl->setPower()` line 261; `m_displayControl->isPowered()` line 357; `m_displayControl->brightness()` line 358 |
| `HttpApiServer.cpp` | `SpotifyController::search` | `GET /spotify/search` route | WIRED | `m_spotifyController->search(searchQuery)` at line 495 |
| `PlaybackRouter.cpp` | `PlaybackState::activeSource` | reads active source for dispatch | WIRED | `m_playbackState->activeSource()` called in all 6 dispatch methods |
| `PlaybackRouter.cpp` | `CdController`, `SpotifyController` | calls stop/play/next/etc | WIRED | `m_spotifyController->play()` line 71; `m_cdController->stop()` line 134; `m_spotifyController->deactivateSpotify()` line 130 |
| `AlbumArtResolver.cpp` | `ReceiverState::albumArtUrl` | streaming art from receiver CGI | WIRED | `m_receiverState->albumArtUrl()` line 48 |
| `AppBuilder.cpp` | All Phase 9 objects | composition root wiring | WIRED | `std::make_unique<ScreenTimeoutController>` line 136; `std::make_unique<HttpApiServer>` line 169; `std::make_unique<PlaybackRouter>` line 152; `std::make_unique<AlbumArtResolver>` line 164 |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| DISP-01 | 09-01 | DDC/CI power and brightness control via ddcutil subprocess | SATISFIED | LinuxDisplayControl wraps ddcutil setvcp 10 (brightness) and setvcp d6 (power) |
| DISP-02 | 09-01 | Smooth dimming transitions, fade to black | SATISFIED | 20-step animation in onDimStep() at 50ms intervals (~1 second) |
| DISP-03 | 09-01 | Auto-detection of display bus | SATISFIED | autoDetectDisplay() parses I2C bus from ddcutil detect --brief |
| DISP-04 | 09-01 | Screen timeout state machine: ACTIVE -> DIMMED -> OFF | SATISFIED | Full 4-state machine (Active/Dimming/Dimmed/Off) with configurable timers |
| DISP-05 | 09-01 | Configurable dim/off timeouts and dim brightness | SATISFIED | DisplayConfig.dimTimeoutSeconds, offTimeoutSeconds, dimBrightness, timeoutEnabled consumed |
| DISP-06 | 09-01 | Activity-based reset | SATISFIED | activityDetected() slot; GPIO signals wired in AppBuilder |
| DISP-07 | 09-01 | Playback-aware: disable timeout during active music | SATISFIED | onPlaybackModeChanged() suppresses timers when Playing |
| DISP-08 | 09-01 | Door sensor integration via reed switch | SATISFIED | onDoorOpenChanged(): open -> activityDetected(), close -> immediate dim+off |
| API-01 | 09-02 | REST API via Qt HttpServer on configurable port | SATISFIED | HttpApiServer using Qt6::HttpServer, port from ApiConfig |
| API-02 | 09-02 | Endpoints: volume set, input switch, status query, display power | SATISFIED | POST /api/volume, POST /api/input, POST /api/display, GET /api/status all implemented |
| API-03 | 09-02 | Spotify OAuth setup page and callback endpoint | SATISFIED | GET /setup/spotify and GET /auth/spotify/callback implemented |
| API-04 | 09-02 | Spotify search endpoint | SATISFIED | GET /spotify/search implemented at lines 474-499; calls SpotifyController::search(); test exists |
| API-05 | 09-02 | Optional HTTPS via auto-generated self-signed certificate | SATISFIED | Full SSL setup under #ifdef QT_SSL_AVAILABLE with openssl subprocess certificate generation |
| ORCH-01 | 09-03 | PlaybackRouter owns input->controller routing | SATISFIED | PlaybackRouter eliminates if/else chains across all 6 playback commands |
| ORCH-02 | 09-03 | AlbumArtResolver: receiver CGI art for streaming, local for CD/Library | SATISFIED | AlbumArtResolver.resolve() delegates correctly per source |

**Orphaned requirements check:** No Phase 9 requirements in REQUIREMENTS.md are absent from the plans.
**All 15 requirement IDs (DISP-01 through DISP-08, API-01 through API-05, ORCH-01 through ORCH-02) are SATISFIED.**

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/orchestration/PlaybackRouter.cpp` | 75-79 | CD play/pause are no-ops with log messages | Info | Intentional design: CdController has no play()/pause() methods; user initiates via track selection |
| `src/orchestration/PlaybackRouter.cpp` | 223 | Spotify seek not implemented | Info | Intentional: Phase 8 SpotifyController has no seek slot. No functional regression |

No blocker anti-patterns remain. The previously-flagged blocker (missing /spotify/search) is resolved.

---

### Human Verification Required

#### 1. DDC/CI Display Hardware Control

**Test:** On Raspberry Pi with a DDC/CI-capable display, run the application and trigger ScreenTimeoutController::activityDetected() followed by waiting for dim_timeout to expire.
**Expected:** Display brightness fades from 100% to dimBrightness over ~1 second via ddcutil setvcp 10, then powers off via setvcp d6 04 after off_timeout.
**Why human:** All macOS tests exercise only the guard paths (m_busNumber < 0 returns false). ddcutil subprocess calls require real I2C hardware.

#### 2. Spotify OAuth Flow via Browser

**Test:** Access `http://{pi-ip}:8080/setup/spotify` in a browser.
**Expected:** HTML page loads with instructions; Spotify redirects the user to authorize; callback page shows success.
**Why human:** Requires live Spotify API credentials, real OAuth redirect over HTTPS, and network access.

#### 3. Real-Time Activity Screen Wake

**Test:** Allow screen to reach Off state, then trigger GPIO input (volume encoder or door sensor).
**Expected:** Screen immediately powers on (setPower(true)) and brightness restores to 100%.
**Why human:** Requires real GPIO hardware signals; timer-based transitions need real elapsed time.

---

### Gaps Summary

No gaps remain. The single gap from initial verification (API-04 — missing /spotify/search endpoint) has been resolved.

All 15 must-have truths are verified. All 15 requirement IDs are satisfied. All artifacts are substantive and wired. The phase goal is achieved in code:

- Screen dims and powers off on inactivity: ScreenTimeoutController fully implemented and tested (14 tests)
- HTTP API enables remote control: HttpApiServer with all 9 endpoints implemented and tested (26+ tests)
- PlaybackRouter eliminates source-routing duplication: Switch-based dispatch covers all 6 commands and all sources (11 tests)

Remaining items are hardware-only verifications that cannot be automated.

---

_Verified: 2026-02-28T22:00:00Z_
_Verifier: Claude (gsd-verifier)_
_Re-verification: Yes (previous gaps_found -> now human_needed)_
