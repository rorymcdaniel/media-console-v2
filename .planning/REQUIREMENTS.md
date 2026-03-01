# Requirements: Media Console v2 — v1.1 UI Polish

**Defined:** 2026-03-01
**Core Value:** The touchscreen kiosk must always be responsive — no freezes, no unrecoverable states.

## v1.1 Requirements

The original QML (~5,200 lines) is the reference benchmark. Every requirement here is a quality gap between the v1.0 UI and the original. No new backend features.

### Input Carousel

- [ ] **CAR-01**: InputCarousel rotation animation is smooth and continuous — no jank or frame drops on Pi 5
- [ ] **CAR-02**: Carousel snaps to the selected input with a natural deceleration curve
- [ ] **CAR-03**: Non-selected inputs are visually de-emphasised (scale, opacity) to reinforce the 3D wheel illusion
- [ ] **CAR-04**: Physical encoder rotation drives the carousel with 1:1 feel — no lag between turn and scroll

### Now Playing

- [ ] **NOW-01**: Album art fills its container with correct aspect ratio and smooth fade-in on load/change
- [ ] **NOW-02**: Track title, artist, and album text are legible and sized consistently across all sources (CD, FLAC, Spotify, streaming)
- [ ] **NOW-03**: Playback controls (play/pause, prev, next, seek) have immediate visual feedback on press
- [ ] **NOW-04**: Progress bar updates smoothly at ≥10 fps — no visible stepping
- [ ] **NOW-05**: Flipable card transitions are smooth — flip duration and easing match the original

### View Transitions

- [ ] **TRANS-01**: Switching between NowPlaying, LibraryBrowser, and SpotifySearch is animated — no hard cuts
- [ ] **TRANS-02**: View transitions complete in ≤300 ms and do not block touch input during animation
- [ ] **TRANS-03**: LibraryBrowser drill-down (Artist → Album → Track) uses a consistent push/pop animation

### Visual Design

- [ ] **VIS-01**: Typography hierarchy (title, subtitle, body, caption) is consistent across all views
- [ ] **VIS-02**: Color palette and contrast ratios are consistent — no elements near-invisible against the dark background
- [ ] **VIS-03**: Interactive elements have a clear visual affordance distinguishing them from static content
- [ ] **VIS-04**: Status bar layout is balanced — no crowding or truncation at 1920px width

### Touch Responsiveness

- [ ] **TOUCH-01**: Every tappable element responds within one frame — no perceived delay between touch and visual change
- [ ] **TOUCH-02**: Scroll velocity in LibraryBrowser feels natural — fling gesture carries appropriate momentum
- [ ] **TOUCH-03**: AlphabetSidebar scroll-to-letter works reliably with single-finger touch

## v2 Requirements

Deferred — not in scope for v1.1.

- **CAR-05**: Haptic feedback on carousel snap (requires hardware investigation)
- **NOW-06**: Lyrics display panel
- **VIS-05**: Light/dark theme toggle

## Out of Scope

| Feature | Reason |
|---------|--------|
| New data sources or backends | v1.1 is UI-only — backend is complete |
| New QML views or screens | Polish existing views, no new ones |
| Bluetooth metadata improvements | Backend concern, not UI polish |
| HTTP API changes | Backend complete |

## Traceability

*(Populated during roadmap creation)*

| Requirement | Phase | Status |
|-------------|-------|--------|
| CAR-01 | — | Pending |
| CAR-02 | — | Pending |
| CAR-03 | — | Pending |
| CAR-04 | — | Pending |
| NOW-01 | — | Pending |
| NOW-02 | — | Pending |
| NOW-03 | — | Pending |
| NOW-04 | — | Pending |
| NOW-05 | — | Pending |
| TRANS-01 | — | Pending |
| TRANS-02 | — | Pending |
| TRANS-03 | — | Pending |
| VIS-01 | — | Pending |
| VIS-02 | — | Pending |
| VIS-03 | — | Pending |
| VIS-04 | — | Pending |
| TOUCH-01 | — | Pending |
| TOUCH-02 | — | Pending |
| TOUCH-03 | — | Pending |

**Coverage:**
- v1.1 requirements: 19 total
- Mapped to phases: 0
- Unmapped: 19 (roadmap not yet created)

---
*Requirements defined: 2026-03-01*
