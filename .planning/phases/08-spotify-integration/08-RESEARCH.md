# Phase 8: Spotify Integration - Research

**Researched:** 2026-02-28
**Domain:** Spotify Web API + Qt6 NetworkAuth OAuth 2.0 PKCE
**Confidence:** HIGH

## Summary

Phase 8 integrates Spotify into the existing media console via the Spotify Web API (REST) and Qt6's built-in NetworkAuth module for OAuth 2.0 PKCE authentication. The project already has significant scaffolding in place: `SpotifyConfig` with `clientId`, `clientSecret`, and `desiredDeviceName`; `StreamingService::Spotify` detection in ReceiverController; `ActiveView::SpotifySearch` in the view enum; `PlaybackState` with full metadata properties; `UIState::showToast()` for error notifications; and the `media.spotify` logging category.

Qt 6.8+ provides native PKCE S256 support via `QOAuth2AuthorizationCodeFlow::setPkceMethod(PkceMethod::S256)`, automatic token refresh via `setAutoRefresh(true)` with configurable lead time, and `QOAuthHttpServerReplyHandler` for localhost redirect handling — eliminating the need for any third-party OAuth library.

**Primary recommendation:** Use Qt6 NetworkAuth (`QOAuth2AuthorizationCodeFlow` with PKCE S256) for authentication and `QNetworkAccessManager` for all Spotify Web API calls. Build a `SpotifyAuth` class for token lifecycle and a `SpotifyClient` class as a thin REST wrapper over the API endpoints. Build `SpotifyController` as the business logic orchestrator following the existing controller pattern (ReceiverController, CdController, FlacLibraryController).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- External setup only — no login screen on the kiosk touchscreen
- CLI command on the Pi (e.g., `media-console --spotify-auth`) prints a URL
- User opens URL on any browser, authorizes, Spotify redirects back to Pi's localhost
- Tokens persisted and auto-refreshed (5 min pre-expiry per SPOT-01)
- When Spotify tokens are missing or expired beyond recovery: Spotify appears in source list but grayed out, tapping shows toast "Spotify not configured — run setup command"
- Qt VirtualKeyboard module for touchscreen text input
- Mixed results list — single scrollable view with grouped sections (artists, albums, tracks)
- All result rows show album art thumbnails inline
- Tapping a track starts playback immediately (transfers to receiver)
- Tapping an album or artist drills into a detail view showing their tracks
- Suggested/featured playlists appear as default content in search view before user types; replaced by search results when typing
- Spotify transport controls (play/pause/next/prev) integrated into existing NowPlaying view when active source is Spotify
- Touch-only transport buttons — physical knobs continue to control volume and input switching as before
- Search icon visible in NowPlaying when source is Spotify; tapping opens SpotifySearch view; back button returns to NowPlaying
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

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| SPOT-01 | OAuth 2.0 PKCE Authorization Code flow with token persistence and auto-refresh (5 min pre-expiry) | Qt6 NetworkAuth QOAuth2AuthorizationCodeFlow with setPkceMethod(S256), setAutoRefresh(true), setRefreshLeadTime(300s). QOAuthHttpServerReplyHandler for localhost redirect. QSettings for token persistence. |
| SPOT-02 | Device discovery and playback transfer by device name | GET /me/player/devices returns device list with id/name. PUT /me/player with device_ids array transfers playback. Match by SpotifyConfig::desiredDeviceName. |
| SPOT-03 | Playback control: play, pause, next, previous, play by URI, context-aware playback | PUT /me/player/play (with optional uris/context_uri), PUT /me/player/pause, POST /me/player/next, POST /me/player/previous. All require user-modify-playback-state scope. |
| SPOT-04 | Full-text search via Spotify Web API (tracks, artists, albums) | GET /v1/search?q={query}&type=track,artist,album&limit=10. Returns paginated results with images. No special scopes required. |
| SPOT-05 | Queue management: add tracks to queue | POST /me/player/queue?uri={track_uri}. Requires user-modify-playback-state scope. Spotify Premium required. |
| SPOT-06 | Suggested playlists: featured + user recommendations | GET /v1/browse/featured-playlists for featured content. Note: Recommendations API deprecated Nov 2024. Use user's playlists (GET /me/playlists) as fallback for personalized content. |
| SPOT-07 | Active session detection with takeover dialog showing current device and track | GET /me/player returns currently playing device, track, and artist. Check on source selection only (per user decision). Show dialog if playback active on different device. |
| SPOT-08 | Album art: use receiver-provided CGI art for all streaming sources | ReceiverController already parses NJA2 (album art URL) from eISCP. When Spotify is active source, receiver provides album art via its CGI endpoint. No additional Spotify API calls needed for playback art. Search results use Spotify API image URLs directly. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6 NetworkAuth | 6.8+ | OAuth 2.0 PKCE flow (QOAuth2AuthorizationCodeFlow) | Qt's own module; native PKCE S256 support since 6.8; autoRefresh built-in |
| Qt6 Network | 6.8+ | HTTP REST calls (QNetworkAccessManager) | Already in project; handles all Spotify Web API calls |
| Qt6 Core | 6.8+ | JSON parsing (QJsonDocument), timers, settings | Already in project; QSettings for token persistence |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QOAuthHttpServerReplyHandler | 6.8+ | Localhost redirect listener for OAuth callback | During CLI auth flow; listens on localhost:PORT for Spotify redirect |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Qt6 NetworkAuth | Hand-rolled PKCE | Qt handles code verifier generation, S256 challenge, auto-refresh; hand-rolling adds maintenance burden |
| QSettings for tokens | Separate encrypted file | QSettings simpler; tokens are already per-user; clientSecret not truly secret in PKCE flow anyway |
| Polling for playback state | WebSocket/Server-Sent Events | Spotify Web API is REST-only; no push mechanism available; polling is the only option |

**CMake addition:**
```cmake
find_package(Qt6 REQUIRED COMPONENTS NetworkAuth)
target_link_libraries(media-console-lib PUBLIC Qt6::NetworkAuth)
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── spotify/
│   ├── SpotifyAuth.h/cpp        # OAuth 2.0 PKCE lifecycle, token persistence, CLI auth flow
│   ├── SpotifyClient.h/cpp      # Thin REST wrapper over Spotify Web API endpoints
│   └── SpotifyController.h/cpp  # Business logic orchestrator (search, playback, session mgmt)
```

### Pattern 1: Auth/Client/Controller Separation
**What:** Three-layer architecture separating authentication, API access, and business logic.
**When to use:** Always — this maps to the project's existing controller pattern.

- **SpotifyAuth** owns the `QOAuth2AuthorizationCodeFlow`, manages token lifecycle (persist/restore/refresh), exposes `isAuthenticated()` and `accessToken()`. Emits signals on auth state changes.
- **SpotifyClient** owns a `QNetworkAccessManager`, takes the access token from SpotifyAuth, provides typed methods for each API endpoint (`search()`, `getDevices()`, `transferPlayback()`, `play()`, `pause()`, etc.). Returns results via signals or callbacks.
- **SpotifyController** orchestrates the user-facing workflow: search with debounce, device discovery + transfer, playback control, session takeover detection. Writes to `PlaybackState` and `UIState`. Constructed and wired in `AppBuilder`.

### Pattern 2: QSettings Token Persistence
**What:** Store OAuth tokens (access_token, refresh_token, expiry) in QSettings under `[spotify]` group.
**When to use:** On every token grant and refresh.
**Example:**
```cpp
void SpotifyAuth::saveTokens() {
    QSettings settings;
    settings.beginGroup("spotify");
    settings.setValue("access_token", m_oauth.token());
    settings.setValue("refresh_token", m_oauth.refreshToken());
    settings.setValue("expiry", m_oauth.expirationAt().toString(Qt::ISODate));
    settings.endGroup();
}

void SpotifyAuth::restoreTokens() {
    QSettings settings;
    settings.beginGroup("spotify");
    QString accessToken = settings.value("access_token").toString();
    QString refreshToken = settings.value("refresh_token").toString();
    // ... restore to QOAuth2AuthorizationCodeFlow
    settings.endGroup();
}
```

### Pattern 3: CLI Auth Mode
**What:** `media-console --spotify-auth` runs a headless auth flow: starts localhost HTTP server, prints authorization URL, waits for callback.
**When to use:** One-time setup on the Pi.
**Example flow:**
1. Parse `--spotify-auth` in main.cpp before QML engine starts
2. Create SpotifyAuth, start QOAuthHttpServerReplyHandler on a fixed port (e.g., 8888)
3. Print authorization URL to stdout: "Open this URL in a browser: https://accounts.spotify.com/authorize?..."
4. Wait for redirect callback, exchange code for tokens
5. Save tokens to QSettings, print success, exit

### Anti-Patterns to Avoid
- **Polling playback state from SpotifyController:** Only poll when Spotify is the active source. Stop polling when user switches away.
- **Storing clientSecret as a real secret:** PKCE flow does not require a client secret. The Spotify app can be configured as a "public" client. However, the existing SpotifyConfig has clientSecret — it can be sent but is not required for PKCE.
- **Blocking the event loop on API calls:** All QNetworkAccessManager calls are async. Never use `waitForFinished()`.
- **Hardcoding redirect URI port:** Use a configurable port (default 8888) in SpotifyConfig so it can be changed if needed.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| OAuth 2.0 PKCE flow | Custom HTTP auth + code verifier generation | Qt6 QOAuth2AuthorizationCodeFlow with setPkceMethod(S256) | Handles code verifier, S256 challenge, token exchange, auto-refresh |
| Token refresh timer | Custom QTimer-based refresh scheduling | QOAuth2AuthorizationCodeFlow::setAutoRefresh(true) + setRefreshLeadTime(300s) | Qt handles refresh scheduling, expiry tracking, error recovery |
| Localhost redirect server | Custom QTcpServer for OAuth callback | QOAuthHttpServerReplyHandler | Handles HTTP parsing, callback path routing, browser response |
| JSON parsing | Custom string parsing | QJsonDocument::fromJson() | Qt's built-in JSON parser handles all Spotify API responses |
| URL encoding | Manual percent-encoding | QUrlQuery for query parameters | Handles special characters, spaces in search queries |

**Key insight:** Qt6 NetworkAuth provides a complete OAuth 2.0 PKCE implementation. The only custom code needed is token persistence (save/restore to QSettings) and the CLI auth mode entry point.

## Common Pitfalls

### Pitfall 1: Spotify Premium Requirement
**What goes wrong:** Playback control endpoints (play, pause, next, previous, transfer) return 403 Forbidden for free accounts.
**Why it happens:** Spotify restricts active playback control to Premium subscribers.
**How to avoid:** Document Premium requirement. Handle 403 responses gracefully with a descriptive toast message.
**Warning signs:** 403 status code on any /me/player/* PUT/POST endpoint.

### Pitfall 2: Token Rotation on Refresh
**What goes wrong:** Spotify may return a new refresh_token alongside the access_token during refresh. If you don't save the new refresh_token, subsequent refreshes fail.
**Why it happens:** OAuth 2.0 spec allows token rotation for security.
**How to avoid:** Always check for and persist a new refresh_token in the refresh response. The Qt NetworkAuth auto-refresh handles this, but you must connect to `refreshTokenChanged` signal to persist.
**Warning signs:** Auth works for 1 hour, then silently fails on refresh.

### Pitfall 3: Rate Limiting (429 Too Many Requests)
**What goes wrong:** Spotify returns 429 with a `Retry-After` header when rate limits are exceeded.
**Why it happens:** Too many API calls in a short period (especially search while typing).
**How to avoid:** Implement search debounce (300-500ms). Respect `Retry-After` header. Don't poll playback state faster than every 2-3 seconds.
**Warning signs:** 429 responses in logs, inconsistent search results.

### Pitfall 4: Device Not Found After Transfer
**What goes wrong:** Device appears in device list but transfer fails or playback doesn't start.
**Why it happens:** Device went offline between discovery and transfer; device name mismatch.
**How to avoid:** Match by device name (desiredDeviceName), handle transfer failure gracefully, retry device discovery on failure.
**Warning signs:** PUT /me/player returns 404 or playback doesn't start after successful transfer response.

### Pitfall 5: Stale Playback State
**What goes wrong:** PlaybackState shows outdated track info after user changes track on another device.
**Why it happens:** No push notifications from Spotify; relies on polling.
**How to avoid:** Poll GET /me/player every 3-5 seconds when Spotify is active source. Detect "playback interrupted" by checking if the active device changed.
**Warning signs:** Track info doesn't update for 30+ seconds.

### Pitfall 6: Featured Playlists Access
**What goes wrong:** GET /v1/browse/featured-playlists returns 403 or empty for new applications.
**Why it happens:** Spotify restricted several browse/recommendation endpoints in November 2024 for new API consumers.
**How to avoid:** Use user's own playlists (GET /me/playlists) as primary fallback. Featured playlists may work if the app has extended mode access. Handle gracefully if neither works — show empty state with "Search for music" prompt.
**Warning signs:** 403 on browse endpoints; empty playlists response.

## Code Examples

### Qt6 OAuth 2.0 PKCE Setup
```cpp
// Source: Qt 6.8 official docs (doc.qt.io/qt-6/qoauth2authorizationcodeflow.html)
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

SpotifyAuth::SpotifyAuth(const SpotifyConfig& config, QObject* parent)
    : QObject(parent)
{
    m_oauth.setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
    m_oauth.setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));
    m_oauth.setClientIdentifier(config.clientId);
    // PKCE S256 — no client secret needed
    m_oauth.setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256);

    // Scopes needed for playback control, device management, search, user playlists
    m_oauth.setRequestedScopeTokens({
        "user-read-playback-state",
        "user-modify-playback-state",
        "user-read-currently-playing",
        "playlist-read-private",
        "playlist-read-collaborative"
    });

    // Auto-refresh 5 minutes before expiry (per SPOT-01)
    m_oauth.setAutoRefresh(true);
    m_oauth.setRefreshLeadTime(std::chrono::seconds(300));

    // Localhost redirect handler
    auto* handler = new QOAuthHttpServerReplyHandler(8888, this);
    handler->setCallbackText("Authorization complete. You can close this tab.");
    m_oauth.setReplyHandler(handler);

    connect(&m_oauth, &QAbstractOAuth::granted, this, &SpotifyAuth::onGranted);
    connect(&m_oauth, &QAbstractOAuth2::refreshTokenChanged,
            this, &SpotifyAuth::saveTokens);
}
```

### Spotify API Search Call
```cpp
// Source: Spotify Web API docs (developer.spotify.com/documentation/web-api/reference/search)
void SpotifyClient::search(const QString& query, int limit) {
    QUrlQuery params;
    params.addQueryItem("q", query);
    params.addQueryItem("type", "track,artist,album");
    params.addQueryItem("limit", QString::number(limit));

    QUrl url("https://api.spotify.com/v1/search");
    url.setQuery(params);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());

    auto* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit searchFailed(reply->errorString());
            return;
        }
        auto json = QJsonDocument::fromJson(reply->readAll());
        emit searchResultsReady(json.object());
    });
}
```

### Device Transfer
```cpp
// Source: Spotify Web API docs (developer.spotify.com/documentation/web-api/reference/transfer-a-users-playback)
void SpotifyClient::transferPlayback(const QString& deviceId, bool startPlaying) {
    QUrl url("https://api.spotify.com/v1/me/player");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["device_ids"] = QJsonArray{deviceId};
    body["play"] = startPlaying;

    auto* reply = m_networkManager->put(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit transferFailed(reply->errorString());
            return;
        }
        emit transferComplete();
    });
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Implicit Grant OAuth | Authorization Code + PKCE | Spotify deprecated Implicit Grant migration guide active | PKCE is mandatory for new apps; no client secret exposure |
| Get Recommendations API | Deprecated/restricted | November 2024 | New apps cannot use recommendations; use featured playlists or user playlists instead |
| Qt5 manual PKCE | Qt6 NetworkAuth built-in PKCE | Qt 6.8 (2024) | setPkceMethod(S256) eliminates manual code verifier/challenge generation |
| refreshAccessToken() | refreshTokens() | Qt 6.9 | Old method deprecated, scheduled for removal in 6.13 |

**Deprecated/outdated:**
- Spotify Recommendations endpoint: deprecated Nov 2024 for new apps
- Qt `refreshAccessToken()`: deprecated in 6.9, use `refreshTokens()` instead
- Spotify Implicit Grant flow: migration away recommended by Spotify

## Open Questions

1. **Featured playlists access for new apps**
   - What we know: Spotify restricted browse endpoints in Nov 2024 for new API consumers
   - What's unclear: Whether a new Spotify developer app can access GET /v1/browse/featured-playlists
   - Recommendation: Implement featured playlists call but gracefully fall back to user's own playlists (GET /me/playlists) if 403 received. If both fail, show "Search for music" as default content.

2. **Qt6 NetworkAuth availability on Raspberry Pi OS Trixie**
   - What we know: Qt 6.8.2 is the target; NetworkAuth is a separate Qt module
   - What's unclear: Whether the qt6-networkauth-dev package is available in Trixie repos
   - Recommendation: Check package availability on Pi. If not packaged, it can be built from source as part of the Qt6 install. Add `find_package(Qt6 REQUIRED COMPONENTS NetworkAuth)` and fail early if missing.

## Sources

### Primary (HIGH confidence)
- Spotify Web API official docs — OAuth PKCE flow, player endpoints, search endpoint, scopes
- Qt 6.8+ official docs — QOAuth2AuthorizationCodeFlow, PKCE S256, QOAuthHttpServerReplyHandler, autoRefresh, refreshLeadTime

### Secondary (MEDIUM confidence)
- Spotify developer blog (Nov 2024) — API changes, recommendations deprecation
- Qt source code (GitHub qt/qtnetworkauth) — PkceMethod enum, implementation details

### Tertiary (LOW confidence)
- Spotify Community forums — featured playlists access restrictions for new apps

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Qt6 NetworkAuth is well-documented, Spotify Web API is stable
- Architecture: HIGH — follows existing project patterns (controller + state + composition root)
- Pitfalls: HIGH — well-documented API limitations (Premium requirement, rate limits, token rotation)

**Research date:** 2026-02-28
**Valid until:** 2026-03-28 (stable APIs, 30-day validity)
