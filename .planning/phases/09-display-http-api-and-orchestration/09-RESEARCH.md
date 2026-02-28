# Phase 9: Display, HTTP API, and Orchestration - Research

**Researched:** 2026-02-28
**Domain:** DDC/CI display control, Qt6 QHttpServer REST API, playback routing architecture
**Confidence:** HIGH

## Summary

Phase 9 delivers three backend subsystems: (1) LinuxDisplayControl implementing IDisplayControl via ddcutil subprocess calls with a ScreenTimeoutController state machine, (2) HttpApiServer using Qt6's QHttpServer module for REST endpoints and Spotify OAuth pages, (3) PlaybackRouter and AlbumArtResolver for unified playback dispatch and album art resolution.

The existing codebase is well-prepared for this phase. IDisplayControl and StubDisplayControl already exist from Phase 1. PlatformFactory has a TODO comment at line 57 specifically marking where LinuxDisplayControl should be added. AppConfig already includes DisplayConfig and ApiConfig structs with the exact fields needed (dim timeout, off timeout, dim brightness, API port). The reference implementation from media-console v1 provides a complete HttpApiServer blueprint that maps directly to the v2 architecture.

**Primary recommendation:** Build in three parallel streams — display control (DDC/CI + state machine), HTTP API (lift from reference), and orchestration (PlaybackRouter + AlbumArtResolver). All three integrate through AppBuilder and AppContext following established patterns.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- State machine: ACTIVE → DIMMED (5 min default) → OFF (20 min default) with configurable timeouts and dim brightness (default 25%)
- Activity events that reset to ACTIVE: touch events, door open (reed switch), encoder turns/presses, and incoming HTTP API calls
- All active playback sources suppress the timeout — screen stays on while any music is playing regardless of source
- Door open: instant wake to full brightness (ACTIVE state) — someone is physically present
- Door close: immediate transition to DIMMED then OFF — skip the 5-min wait, user is done at the console
- DIMMED state: reduce backlight brightness via DDC/CI, UI content stays visible (no QML fade-to-black)
- LinuxDisplayControl implements IDisplayControl using `ddcutil` subprocess calls — the interface already exists from Phase 1
- Smooth dimming transitions via timed brightness steps (DISP-02)
- QHttpServer on configurable port (default 8080) — lift pattern from reference HttpApiServer
- Core endpoints matching reference: POST /api/volume, POST /api/input, POST /api/display, GET /api/status
- Spotify endpoints matching reference: GET /setup/spotify (HTML auth page), GET /auth/spotify/callback, GET /spotify/status, POST /spotify/play
- Add playback control endpoints: POST /api/playback/play, POST /api/playback/pause, POST /api/playback/next, POST /api/playback/previous — these go through PlaybackRouter
- Optional HTTPS via auto-generated self-signed cert (API-05) — same openssl approach as reference
- No authentication required — API is local network only (behind router firewall)
- No CORS headers needed — API is called from scripts/curl, not browser JS
- PlaybackRouter owns a map of MediaSource → controller (CdController, FlacLibraryController, SpotifyController)
- Active source tracked in ReceiverState::activeSource — PlaybackRouter reads this to dispatch
- Unified Q_INVOKABLE methods: play(), pause(), stop(), next(), previous(), seek(int ms) — dispatches to correct controller
- When a new source activates while another is playing: the new source takes over, old source's controller gets stop() called
- CD and Library share receiver input "02" (S/PDIF via Nvdigi HAT) — PlaybackRouter handles the ambiguity by checking which local controller is actually playing
- Streaming/Bluetooth/Phono: PlaybackRouter sends receiver input switch via ReceiverController, no local audio controller involved
- Spotify: PlaybackRouter calls SpotifyController.activateSpotify() which handles device transfer
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

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DISP-01 | DDC/CI power and brightness control via ddcutil subprocess through IDisplayControl interface | LinuxDisplayControl wraps `ddcutil setvcp` for VCP 0x10 (brightness) and 0xD6 (power) |
| DISP-02 | Smooth dimming transitions, fade to black | QTimer-based step animation in ScreenTimeoutController, 50ms intervals |
| DISP-03 | Auto-detection of display bus | `ddcutil detect --brief` subprocess, parse bus number from output |
| DISP-04 | Screen timeout state machine: ACTIVE -> DIMMED -> OFF (+ DOOR_CLOSED from reed switch) | ScreenTimeoutController with QStateMachine or manual enum + QTimer |
| DISP-05 | Configurable dim timeout (default 5 min), off timeout (default 20 min), dim brightness (default 25%) | Already in AppConfig::DisplayConfig struct |
| DISP-06 | Activity-based reset (touch events reset timeout) | activityDetected() slot resets timers, called from GPIO signals and HTTP API requests |
| DISP-07 | Playback-aware: disable timeout during active music playback | Connect to PlaybackState::playbackModeChanged, suppress timers when Playing |
| DISP-08 | Door sensor integration via reed switch | Connect to UIState::doorOpenChanged, door open = ACTIVE, door close = immediate DIMMED→OFF |
| API-01 | REST API via Qt HttpServer on configurable port (default 8080) | Qt6::HttpServer module, QHttpServer::route() for each endpoint |
| API-02 | Endpoints: volume set, input switch, status query, display power | Direct delegation to ReceiverController and IDisplayControl |
| API-03 | Spotify OAuth setup page and callback endpoint | HTML page with Connect button, code exchange via SpotifyAuth |
| API-04 | Spotify search endpoint | Route to SpotifyController::search() |
| API-05 | Optional HTTPS via auto-generated self-signed certificate | QSslServer + openssl subprocess for cert generation (from reference) |
| ORCH-01 | PlaybackRouter owns input->controller routing | QMap<MediaSource, handler function> dispatch table |
| ORCH-02 | AlbumArtResolver: receiver CGI art for streaming sources, local cached art for CD and Library | Switch on PlaybackState::activeSource, delegate to appropriate provider |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6::HttpServer | 6.8.2 | REST API server | Official Qt module for HTTP services, QHttpServer with route() API |
| ddcutil | 2.x | DDC/CI display control | Standard Linux DDC/CI tool, works with Raspberry Pi HDMI I2C |
| QProcess | Qt6 Core | Subprocess execution | Wrap ddcutil calls, openssl cert generation |
| QTimer | Qt6 Core | State machine timing | Dim/off timeouts, dimming animation steps |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QSslServer | Qt6 Network | HTTPS support | Optional SSL for API-05, ifdef QT_SSL_AVAILABLE |
| QJsonDocument/QJsonObject | Qt6 Core | API request/response | JSON parsing for POST body, JSON response construction |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| ddcutil subprocess | libddcutil C API | Direct linking avoids subprocess overhead but adds build dependency; subprocess is simpler and matches reference pattern |
| QStateMachine | Manual enum + QTimer | QStateMachine is heavyweight for 3-state FSM; manual enum with QTimer is simpler and more testable |

**Installation:**
```bash
# Qt6 HttpServer (on Raspberry Pi OS Trixie)
sudo apt install qt6-httpserver-dev

# ddcutil
sudo apt install ddcutil

# Grant I2C access (required for ddcutil without root)
sudo usermod -aG i2c $USER
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── display/
│   ├── LinuxDisplayControl.h      # IDisplayControl via ddcutil
│   ├── LinuxDisplayControl.cpp
│   ├── ScreenTimeoutController.h  # ACTIVE/DIMMED/OFF state machine
│   └── ScreenTimeoutController.cpp
├── api/
│   ├── HttpApiServer.h            # QHttpServer REST routes
│   └── HttpApiServer.cpp
├── orchestration/
│   ├── PlaybackRouter.h           # Unified dispatch
│   ├── PlaybackRouter.cpp
│   ├── AlbumArtResolver.h         # Source-aware art resolution
│   └── AlbumArtResolver.cpp
```

### Pattern 1: IDisplayControl via Subprocess
**What:** LinuxDisplayControl wraps ddcutil subprocess calls, implementing the existing IDisplayControl interface
**When to use:** All DDC/CI operations (brightness, power, detection)
**Example:**
```cpp
// LinuxDisplayControl::setBrightness
bool LinuxDisplayControl::setBrightness(int percent)
{
    QProcess process;
    process.start("ddcutil", {"setvcp", "10", QString::number(percent),
                               "--bus", QString::number(m_busNumber)});
    if (!process.waitForFinished(kDdcTimeoutMs)) {
        qCWarning(mediaApp) << "ddcutil brightness timeout";
        return false;
    }
    if (process.exitCode() == 0) {
        m_brightness = percent;
        emit brightnessChanged(percent);
        return true;
    }
    return false;
}
```

### Pattern 2: Screen Timeout State Machine
**What:** Three states (ACTIVE, DIMMED, OFF) driven by timers and activity signals
**When to use:** ScreenTimeoutController manages display lifecycle
**Example:**
```cpp
enum class ScreenState { Active, Dimmed, Off };

// Activity resets to Active
void ScreenTimeoutController::activityDetected()
{
    if (m_state != ScreenState::Active) {
        m_displayControl->setBrightness(100);
        m_displayControl->setPower(true);
        setState(ScreenState::Active);
    }
    m_dimTimer.start(m_dimTimeoutMs);
    m_offTimer.stop();
}
```

### Pattern 3: PlaybackRouter Dispatch Table
**What:** Map of MediaSource → handler, eliminating per-command if/else chains
**When to use:** All playback commands (play, pause, stop, next, previous, seek)
**Example:**
```cpp
// Single dispatch method used by all commands
void PlaybackRouter::play()
{
    auto source = m_playbackState->activeSource();
    switch (source) {
    case MediaSource::CD:
        m_cdController->play();
        break;
    case MediaSource::Library:
        m_flacLibraryController->play();
        break;
    case MediaSource::Streaming:
        m_spotifyController->play();
        break;
    default:
        // Phono, Bluetooth, Computer: no local control
        break;
    }
}
```

### Pattern 4: HTTP API Route Setup (from reference)
**What:** QHttpServer with route() lambdas, JSON request/response
**When to use:** All REST endpoints
**Example:**
```cpp
// Source: reference implementation HttpApiServer.cpp
m_server->route("/api/volume", QHttpServerRequest::Method::Post,
    [this](const QHttpServerRequest& request) {
        auto json = QJsonDocument::fromJson(request.body()).object();
        int volume = json["volume"].toInt();
        m_receiverController->setVolume(volume);
        m_screenTimeoutController->activityDetected(); // API call = activity
        return QJsonObject{{"status", "ok"}};
    });
```

### Anti-Patterns to Avoid
- **Duplicated routing logic:** Do NOT write separate if/else chains in each playback method. PlaybackRouter exists to centralize this.
- **Blocking subprocess calls on main thread:** ddcutil can take 1-2 seconds. Use QProcess async or accept brief blocking since DDC/CI calls are infrequent.
- **Polling for display state:** Trust the ScreenTimeoutController state machine; don't poll ddcutil for current brightness.
- **Hardcoded DDC/CI bus number:** Always auto-detect via `ddcutil detect` and cache the result.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| HTTP server | Custom TCP socket handler | Qt6::HttpServer (QHttpServer) | Route matching, request parsing, response formatting all built-in |
| DDC/CI protocol | Raw I2C communication | ddcutil subprocess | DDC/CI has complex timing and retry requirements; ddcutil handles hardware quirks |
| SSL certificate | Manual OpenSSL API | openssl CLI subprocess | Reference pattern works; OpenSSL C API is error-prone |
| State machine | Custom event loop | Enum + QTimer pair | 3 states is too simple for QStateMachine; enum + timer is clear and testable |

**Key insight:** The ddcutil subprocess approach is correct for this project. The overhead of a 1-2 second subprocess call is irrelevant for display control that happens once every 5-20 minutes.

## Common Pitfalls

### Pitfall 1: DDC/CI Timing Issues
**What goes wrong:** Rapid successive DDC/CI commands can fail because the display's I2C bus has minimum command spacing
**Why it happens:** DDC/CI standard requires delays between commands (typically 40-200ms depending on display)
**How to avoid:** For smooth dimming, space `setvcp` calls at 50-100ms intervals via QTimer. Never burst multiple DDC commands.
**Warning signs:** ddcutil returning exit code 1 intermittently

### Pitfall 2: QHttpServer Binding Order
**What goes wrong:** Server fails to start because QTcpServer::listen() and QHttpServer::bind() are called in wrong order
**Why it happens:** QHttpServer needs a listening QTcpServer to bind to
**How to avoid:** Follow reference pattern: create QTcpServer, call listen(), then call QHttpServer::bind(tcpServer). The reference implementation does this correctly.
**Warning signs:** "Failed to bind HTTP server" log message

### Pitfall 3: Playback Source Ambiguity (CD vs Library)
**What goes wrong:** CD and Library share the same receiver input hex code (0x23 / S/PDIF). PlaybackRouter can't distinguish them from receiver state alone.
**Why it happens:** Both local sources output audio through the same S/PDIF connection to the receiver
**How to avoid:** PlaybackRouter checks which local controller is actively playing (CdController or FlacLibraryController) rather than relying solely on ReceiverState::currentInput. The `PlaybackState::activeSource` property already tracks this correctly.
**Warning signs:** Wrong controller receiving commands when switching between CD and Library

### Pitfall 4: Thread Safety in Activity Detection
**What goes wrong:** Multiple sources (GPIO signals, HTTP requests, timer callbacks) calling activityDetected() concurrently
**Why it happens:** GPIO monitor runs on background thread; HTTP requests come from QHttpServer's thread; timers fire on main thread
**How to avoid:** ScreenTimeoutController lives on the main thread. GPIO and HTTP signals are auto-connected with Qt::AutoConnection (queued when cross-thread), so timer operations are safe. Verify all connections are to the main thread object.
**Warning signs:** Timer restarts happening from wrong thread (Qt warning about timers from non-owner thread)

### Pitfall 5: ddcutil Permissions on Raspberry Pi
**What goes wrong:** ddcutil fails with permission denied when running as regular user
**Why it happens:** I2C bus access requires i2c group membership
**How to avoid:** Deployment script must add user to i2c group: `sudo usermod -aG i2c $USER`. LinuxDisplayControl should log a clear error and fall back to StubDisplayControl behavior if ddcutil fails.
**Warning signs:** "DDC/CI bus not found" or permission errors in log

## Code Examples

Verified patterns from existing codebase and reference implementation:

### QHttpServer Route with JSON Response
```cpp
// Source: Qt6 official docs — QHttpServer route() API
m_server->route("/api/status", QHttpServerRequest::Method::Get,
    [this]() {
        return QJsonObject{
            {"volume", m_receiverState->volume()},
            {"powered", m_receiverState->powered()},
            {"muted", m_receiverState->muted()},
            {"input", static_cast<int>(m_receiverState->currentInput())},
            {"displayPowered", m_displayControl->isPowered()},
            {"displayBrightness", m_displayControl->brightness()}
        };
    });
```

### Spotify OAuth HTML Page (from reference)
```cpp
// Source: media-console v1 HttpApiServer.cpp
m_server->route("/setup/spotify", QHttpServerRequest::Method::Get,
    [this]() {
        m_spotifyAuth->startAuthFlow();
        // SpotifyAuth emits authorizationUrlReady
        // Return HTML page with Connect button
        return QString(
            "<!DOCTYPE html><html><head><title>Spotify Setup</title></head>"
            "<body style='font-family: sans-serif; max-width: 600px; margin: 50px auto; text-align: center;'>"
            "<h1>Spotify Authentication</h1>"
            "<p>Click the button below to authenticate with Spotify</p>"
            "<a href='%1' style='display: inline-block; padding: 15px 30px; "
            "background: #1DB954; color: white; text-decoration: none; "
            "border-radius: 25px; font-weight: bold;'>Connect to Spotify</a>"
            "</body></html>"
        ).arg(m_authUrl);
    });
```

### ddcutil Display Detection
```bash
# Auto-detect display bus
ddcutil detect --brief
# Output: Display 1
#   I2C bus:  /dev/i2c-1
#   ...

# Set brightness (VCP 0x10)
ddcutil setvcp 10 75 --bus 1

# Set power mode (VCP 0xD6): 01=on, 04=off
ddcutil setvcp d6 01 --bus 1

# Get current brightness
ddcutil getvcp 10 --bus 1
```

### AlbumArtResolver Source Selection
```cpp
void AlbumArtResolver::onActiveSourceChanged(MediaSource source)
{
    switch (source) {
    case MediaSource::Streaming:
    case MediaSource::Bluetooth:
        // Art from receiver CGI endpoint (already in ReceiverState::albumArtUrl)
        setAlbumArtUrl(m_receiverState->albumArtUrl());
        break;
    case MediaSource::CD:
        // Art from CdAlbumArtProvider (local cache)
        setAlbumArtUrl(m_cdController->currentAlbumArtPath());
        break;
    case MediaSource::Library:
        // Art from LibraryAlbumArtProvider (local cache)
        setAlbumArtUrl(m_flacLibraryController->currentAlbumArtPath());
        break;
    default:
        // Phono, None, Computer: no art
        setAlbumArtUrl(QString());
        break;
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Custom HTTP parsing | QHttpServer (Qt6) | Qt 6.4 (2022) | No need for third-party HTTP libs; first-party Qt support |
| libddcutil C API | ddcutil CLI subprocess | Ongoing | CLI is simpler, no linking; libddcutil2 available but overkill |
| Per-method if/else routing | Dispatch table / switch | N/A (design pattern) | Eliminates O(n) code duplication across playback commands |

**Deprecated/outdated:**
- QHttpServer was tech preview in Qt 6.4-6.5; it's fully supported since Qt 6.6+
- ddcutil 1.x used different command syntax; ddcutil 2.x is current on Raspberry Pi OS Trixie

## Open Questions

1. **ddcutil on Pi 5 with Bookworm/Trixie kernel**
   - What we know: Known issue (#356) where brightness stays at 0 on Pi 5 with certain displays
   - What's unclear: Whether this affects the specific display used with this kiosk
   - Recommendation: Test `ddcutil detect` and `ddcutil setvcp 10 50` on actual hardware first; if fails, implement LinuxDisplayControl with error logging and fallback to StubDisplayControl behavior

2. **SpotifyAuth OAuth flow for HTTP API callback**
   - What we know: SpotifyAuth uses QOAuthHttpServerReplyHandler on port 8888 (from config). HttpApiServer is on port 8080.
   - What's unclear: Whether the /auth/spotify/callback should be on the HttpApiServer or the existing QOAuthHttpServerReplyHandler
   - Recommendation: Use the existing QOAuthHttpServerReplyHandler for the actual OAuth token exchange (it handles PKCE verification). The HttpApiServer's /setup/spotify endpoint just serves the HTML page that redirects to Spotify; the callback goes to port 8888 where QOAuthHttpServerReplyHandler listens. The /auth/spotify/callback route on HttpApiServer can be a simple redirect or informational page.

## Sources

### Primary (HIGH confidence)
- Qt6 6.8 official docs — QHttpServer class, route() API, colorpalette example (Context7 /websites/doc_qt_io_qt-6_8)
- media-console v1 reference implementation — HttpApiServer.h/cpp (~/Code/media-console/src/api/)
- Existing codebase — IDisplayControl.h, StubDisplayControl.h, PlatformFactory.cpp, AppBuilder.cpp, AppContext.h, AppConfig.h

### Secondary (MEDIUM confidence)
- ddcutil official documentation — setvcp, getvcp, detect commands (https://www.ddcutil.com/)
- Qt6 HttpServer CMake integration — find_package(Qt6 COMPONENTS HttpServer)

### Tertiary (LOW confidence)
- ddcutil Pi 5 issue #356 — brightness behavior on specific hardware combination (needs validation on actual hardware)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Qt6 QHttpServer is well-documented, ddcutil is the standard tool
- Architecture: HIGH — Patterns follow existing codebase conventions (IDisplayControl, PlatformFactory, AppBuilder)
- Pitfalls: HIGH — Reference implementation validates HTTP patterns; DDC/CI pitfalls documented in ddcutil docs

**Research date:** 2026-02-28
**Valid until:** 2026-03-28 (stable — no fast-moving dependencies)
