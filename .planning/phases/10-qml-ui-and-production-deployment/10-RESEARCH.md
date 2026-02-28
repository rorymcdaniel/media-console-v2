# Phase 10: QML UI and Production Deployment - Research

**Researched:** 2026-02-28
**Domain:** Qt6 QML UI development, systemd kiosk deployment
**Confidence:** HIGH

## Summary

Phase 10 replaces the test-harness `main.qml` with a complete touch interface for a 1920x720 kiosk display. The backend is fully built: three state singletons (ReceiverState, PlaybackState, UIState) already registered as QML singletons, PlaybackRouter with Q_INVOKABLE methods, AlbumArtResolver with Q_PROPERTY binding, SpotifyController with search/playback Q_PROPERTYs, and three library browse models (Artist, Album, Track) as QAbstractListModel subclasses. The QML layer binds directly to these C++ objects — no new C++ code is needed except registering additional singletons in main.cpp.

Production deployment uses systemd for auto-start/restart and shell scripts for install/uninstall. The Raspberry Pi OS Trixie (aarch64) target with EGLFS platform plugin provides the kiosk environment.

**Primary recommendation:** Build QML in layers — Theme singleton first, then main layout skeleton, then individual components, then overlays/dialogs, and finally production deployment. Register PlaybackRouter, AlbumArtResolver, SpotifyController, and FlacLibraryController as QML singletons alongside the existing state objects.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Soft glass/blur aesthetic — frosted glass panels, backdrop blur behind overlays, subtle translucency (iOS control center / KDE Plasma feel)
- Smooth and fluid transitions — 300-500ms with eased slide/scale and spring physics. Polished and deliberate, not snappy.
- Dynamic accent colors extracted from album art — dominant color drives progress bar, accent highlights, and subtle background tint. Shifts mood with the music.
- Generous touch targets — 64-80px minimum. Larger hit areas with more padding between controls. Forgiving for casual/unfamiliar kiosk users.
- Vertical wheel style input carousel — items stack vertically like a slot machine wheel. Selected item centered and full-size, neighbors shrink above/below.
- Icon-heavy carousel items — large recognizable icons (vinyl, CD, Bluetooth symbol, etc.) with small label below.
- Progress ring countdown for 4-second auto-select — circular progress indicator around the selected icon fills over 4 seconds.
- Always-visible carousel — stays permanently in the left panel.
- Dominant album art — ~60% of the right panel. Large, immersive, front-and-center.
- Tap to flip album art between front and back cover — 3D card-flip animation.
- Adaptive playback controls — show only controls that work for the active source.
- Idle state shows last played — display the last played track's art and info dimmed.
- Album art grid — 2-3 columns of album covers when browsing an artist's albums.
- Drag scrub A-Z sidebar — vertical letter strip supports both tap-to-jump and drag-to-scrub.
- Split track page — album art + album info on the left third, scrollable track list on the right two-thirds.
- Fullscreen overlay with on-screen QWERTY keyboard for Spotify search.
- Live results with debounce — results update as you type, keyboard stays visible at the bottom.

### Claude's Discretion
- Exact Theme.qml token values (specific spacing, font weights, border radii) beyond what requirements specify
- Album art dominant color extraction algorithm
- Spring physics parameters for animations
- Loading skeleton designs
- Error state handling and retry UX
- Exact left panel width vs right panel proportions
- Album art grid column count (2 vs 3) based on available space
- Debounce timing for Spotify search
- Production deployment script details (systemd unit file specifics, unclutter config)

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| UI-01 | Deep blue color theme (#0a1628 primary, #162844 secondary, #2e5c8a accent) | Theme.qml singleton with QtObject pragma |
| UI-02 | Theme.qml singleton with design tokens: colors, typography, spacing, touch targets, animations | QtObject pragma Singleton pattern in QML |
| UI-03 | Main layout: left panel, right panel, top status bar | RowLayout/ColumnLayout on 1920x720 Window |
| UI-04 | NowPlaying: album art, track info, progress bar with seek, playback controls | Binds to PlaybackState, AlbumArtResolver, PlaybackRouter |
| UI-05 | InputCarousel: 3D perspective carousel with 6 inputs, 4s auto-select, encoder | PathView or custom ListView with transforms |
| UI-06 | LibraryBrowser: StackView drill-down, A-Z quick scroll, split track page | StackView + ListView + GridView binding to library models |
| UI-07 | SpotifySearch: fullscreen overlay with on-screen QWERTY keyboard | Overlay with TextInput + custom keyboard grid |
| UI-08 | SpotifyTakeoverDialog: modal confirmation for session transfer | Modal Dialog binding to SpotifyController.activeSessionDetected |
| UI-09 | AudioErrorDialog: modal for ALSA recovery failures with retry | Binds to UIState.audioError |
| UI-10 | ToastNotification: bottom-center 3-second auto-dismiss | UIState.showToast signal drives visibility with Timer |
| UI-11 | VolumeOverlay: large modal with numeric display, auto-dismiss 2s | UIState.volumeOverlayVisible + ReceiverState.volume |
| UI-12 | VolumeIndicator: persistent top-right display with draggable slider | ReceiverState.volume binding + MouseArea drag |
| UI-13 | EjectButton: visible only when CD present | Conditional on PlaybackState.activeSource === CD |
| UI-14 | SearchButton: visible only on Spotify input | Conditional on ReceiverState.currentInput === Streaming |
| UI-15 | ErrorBanner: shown when receiver disconnected | UIState.receiverConnected === false |
| UI-16 | TimeDisplay: current time, updates every minute | QML Timer + Qt.formatTime(new Date()) |
| UI-17 | PowerButton, MuteButton with visual state indicators | ReceiverState.powered/muted bindings |
| UI-18 | Global MouseArea for touch activity detection | Root MouseArea propagating to ScreenTimeoutController |
| PROD-01 | Kiosk mode via systemd service | systemd user service unit with Restart=always |
| PROD-02 | Auto-login, hidden cursor, disabled screen blanking | getty autologin, unclutter, xset/DPMS disable |
| PROD-03 | install-kiosk.sh / uninstall-kiosk.sh deployment scripts | Shell scripts managing systemd enable/disable |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Quick 6.8 | 6.8.2 | Declarative UI framework | Already in project, all state singletons registered |
| Qt Quick Controls | 6.8.2 | Slider, StackView, SwipeView | Standard Qt6 controls for touch UI |
| Qt Quick Layouts | 6.8.2 | RowLayout, ColumnLayout | Responsive layout management |
| Qt5Compat.GraphicalEffects | 6.8.2 | GaussianBlur, FastBlur | Frosted glass aesthetic (blur effects) |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QtQuick.Shapes | 6.8.2 | Custom path-based shapes | Progress ring for carousel auto-select |
| systemd | N/A | Service management | Kiosk auto-start/restart |
| unclutter | N/A | Cursor hiding | Touch kiosk (no mouse cursor) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Qt5Compat.GraphicalEffects | Qt Quick MultiEffect (6.5+) | MultiEffect is newer but Qt5Compat more widely documented |
| Custom keyboard | Qt Virtual Keyboard | Qt VK requires extra module and is harder to style, custom grid is simpler |
| PathView carousel | ListView with transforms | PathView has built-in 3D path support but ListView with scale/opacity transforms gives more control |

## Architecture Patterns

### Recommended QML Structure
```
src/qml/
├── main.qml                    # Root Window, layout skeleton, global MouseArea
├── Theme.qml                   # Singleton with design tokens
├── components/
│   ├── InputCarousel.qml       # Left panel carousel
│   ├── NowPlaying.qml          # Right panel - now playing view
│   ├── LibraryBrowser.qml      # Right panel - library browse view
│   ├── SpotifySearch.qml       # Fullscreen overlay
│   ├── StatusBar.qml           # Top status bar
│   └── PlaybackControls.qml    # Reusable playback buttons
├── overlays/
│   ├── VolumeOverlay.qml       # Large volume display
│   ├── ToastNotification.qml   # Bottom toast
│   ├── ErrorBanner.qml         # Top error banner
│   ├── SpotifyTakeoverDialog.qml
│   └── AudioErrorDialog.qml
└── controls/
    ├── VolumeIndicator.qml     # Persistent volume slider
    ├── TimeDisplay.qml         # Clock
    ├── SimpleKeyboard.qml      # On-screen QWERTY
    └── AlphabetSidebar.qml     # A-Z quick scroll
```

### Pattern 1: QML Singleton Theme
**What:** A pragma Singleton QML file exporting all design tokens
**When to use:** Every component references Theme.spacing, Theme.colors.primary, etc.
**Example:**
```qml
// Theme.qml
pragma Singleton
import QtQuick

QtObject {
    readonly property color primaryBg: "#0a1628"
    readonly property color secondaryBg: "#162844"
    readonly property color accent: "#2e5c8a"
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int touchTargetSmall: 44
    readonly property int touchTargetLarge: 64
    readonly property int animFast: 150
    readonly property int animMedium: 300
    readonly property int animSlow: 500
}
```
Register in `qmldir` file or via `qt_add_qml_module` with `SINGLETON`.

### Pattern 2: State Binding (no signals wiring needed)
**What:** QML properties bind directly to C++ Q_PROPERTY singletons
**When to use:** All UI state reads
**Example:**
```qml
Text {
    text: PlaybackState.title
    // Auto-updates when C++ emits titleChanged()
}

Slider {
    value: ReceiverState.volume
    // Two-way binding via onMoved handler
}
```

### Pattern 3: Q_INVOKABLE for Actions
**What:** QML calls C++ methods marked Q_INVOKABLE
**When to use:** Playback controls, input selection
**Example:**
```qml
Button {
    onClicked: PlaybackRouter.play()
}
```
PlaybackRouter already has Q_INVOKABLE on play/pause/stop/next/previous/seek.

### Pattern 4: Adaptive Controls via Source
**What:** Show/hide controls based on active media source
**When to use:** NowPlaying controls differ by source
**Example:**
```qml
Row {
    // Always show
    IconButton { icon: "play"; onClicked: PlaybackRouter.play() }

    // Only for CD/FLAC/Spotify
    IconButton {
        icon: "previous"
        visible: PlaybackState.activeSource !== MediaSource.Bluetooth
        onClicked: PlaybackRouter.previous()
    }
}
```

### Pattern 5: Dynamic Color from Album Art
**What:** Extract dominant color from album art image for accent theming
**When to use:** NowPlaying accent colors shift with the music
**Example:**
```qml
Image {
    id: albumArt
    source: AlbumArtResolver.albumArtUrl

    // Use Canvas to sample dominant color from loaded image
    onStatusChanged: {
        if (status === Image.Ready) {
            colorExtractor.extractDominantColor(albumArt)
        }
    }
}
```
Approach: Use a QML Canvas element to draw the image at 1x1 pixel to get the average color, or sample a small grid. This avoids needing C++ image processing.

### Anti-Patterns to Avoid
- **Direct QSettings reads in QML:** All config is loaded once in AppConfig and passed through. QML reads state objects only.
- **Imperative state management:** Use declarative property bindings, not manual signal handlers for state sync.
- **Blocking the UI thread:** All heavy operations (scanning, network) already run on background threads. QML must not introduce blocking.
- **Hard-coded dimensions:** All sizes go through Theme.qml tokens for consistency.
- **Nested MouseArea conflicts:** Use `propagateComposedEvents: true` on the global touch-activity MouseArea to avoid eating touch events.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Blur effects | Custom shader | Qt5Compat.GraphicalEffects FastBlur/GaussianBlur | Optimized, hardware-accelerated |
| Progress ring | Manual arc drawing | QtQuick.Shapes ShapePath with PathArc | Clean API, anti-aliased rendering |
| Hierarchical navigation | Manual stack management | StackView (Qt Quick Controls) | Built-in push/pop with transitions |
| On-screen keyboard | Integrate Qt Virtual Keyboard module | Simple custom Grid of Button items | VK module is heavy, hard to style; a grid of 30 buttons is simpler |
| Service management | init.d scripts | systemd user service | Standard on Raspberry Pi OS, restart policies, logging |

## Common Pitfalls

### Pitfall 1: QML Module Registration
**What goes wrong:** New QML files are created but not added to qt_add_qml_module, causing "module not found" at runtime.
**Why it happens:** Qt6 requires explicit file listing in CMakeLists.txt.
**How to avoid:** Add EVERY new .qml file to the QML_FILES list in qt_add_qml_module. Also add a qmldir file for the singleton.
**Warning signs:** "QML module not found" or "is not a type" errors at startup.

### Pitfall 2: Singleton Registration for Non-State Objects
**What goes wrong:** PlaybackRouter, AlbumArtResolver, SpotifyController, FlacLibraryController are constructed but not accessible from QML.
**Why it happens:** Only ReceiverState, PlaybackState, UIState are currently registered as QML singletons in main.cpp.
**How to avoid:** Add qmlRegisterSingletonInstance calls for PlaybackRouter, AlbumArtResolver, SpotifyController, and FlacLibraryController in main.cpp.
**Warning signs:** "ReferenceError: PlaybackRouter is not defined" in QML.

### Pitfall 3: EGLFS MouseArea vs Touch
**What goes wrong:** MouseArea doesn't receive touch events on EGLFS platform.
**Why it happens:** EGLFS may deliver touch events differently than mouse events.
**How to avoid:** Use MultiPointTouchArea for touch-critical areas, or ensure global MouseArea has `acceptedButtons: Qt.AllButtons` and test on target hardware.
**Warning signs:** Touch works on desktop but not on Pi.

### Pitfall 4: GraphicalEffects Performance on Pi
**What goes wrong:** GaussianBlur on large areas causes frame drops on Raspberry Pi 5.
**Why it happens:** Blur is GPU-intensive; Pi's GPU is limited.
**How to avoid:** Use FastBlur instead of GaussianBlur, limit blur radius (16-32px), blur static elements and cache with `layer.enabled: true`. Consider pre-rendered blurred backgrounds.
**Warning signs:** FPS drops below 30 on Pi when blur is visible.

### Pitfall 5: Image Loading Blocking
**What goes wrong:** Loading album art causes UI stutter.
**Why it happens:** Image decoding happens on the render thread by default.
**How to avoid:** Use `asynchronous: true` on all Image elements. Show placeholder during load.
**Warning signs:** Brief freeze when switching tracks or scrolling library.

### Pitfall 6: StackView Transition Memory
**What goes wrong:** Pushed pages remain in memory after pop, causing OOM.
**Why it happens:** StackView keeps destroyed items by default.
**How to avoid:** Set `StackView.onRemoved: destroy()` on pushed components, or use `destroyOnPop: true`.
**Warning signs:** Memory grows steadily when navigating library drill-downs.

### Pitfall 7: systemd Service Environment
**What goes wrong:** QML app fails to start as a systemd service.
**Why it happens:** Missing DISPLAY, XDG_RUNTIME_DIR, or QT_QPA_PLATFORM environment variables.
**How to avoid:** Set `Environment=QT_QPA_PLATFORM=eglfs` and `Environment=XDG_RUNTIME_DIR=/run/user/1000` in the service unit.
**Warning signs:** "Could not find display" or "EGLFS: Failed to open DRM device" in journal.

## Code Examples

### Registering Additional QML Singletons (main.cpp)
```cpp
// After existing singleton registrations, before engine.load():
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "PlaybackRouter", ctx.playbackRouter);
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "AlbumArtResolver", ctx.albumArtResolver);
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "SpotifyController", ctx.spotifyController);
#ifdef HAS_SNDFILE
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "LibraryArtistModel", ctx.flacLibraryController->artistModel());
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "LibraryAlbumModel", ctx.flacLibraryController->albumModel());
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "LibraryTrackModel", ctx.flacLibraryController->trackModel());
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "FlacLibraryController", ctx.flacLibraryController);
#endif
```

### Global Touch Activity MouseArea (main.qml)
```qml
Window {
    width: 1920; height: 720; visible: true

    // Global touch activity detection (UI-18)
    MouseArea {
        anchors.fill: parent
        propagateComposedEvents: true
        onPressed: (mouse) => {
            ScreenTimeoutController.activityDetected()
            mouse.accepted = false // Let events pass through
        }
    }

    // Main layout below...
}
```
Note: ScreenTimeoutController needs to be registered as QML singleton too.

### systemd Service Unit
```ini
[Unit]
Description=Media Console Kiosk
After=graphical.target

[Service]
Type=simple
User=rory
Environment=QT_QPA_PLATFORM=eglfs
Environment=QT_QPA_EGLFS_KMS_CONFIG=/etc/media-console/kms.json
Environment=XDG_RUNTIME_DIR=/run/user/1000
ExecStart=/usr/local/bin/media-console
Restart=always
RestartSec=3

[Install]
WantedBy=graphical.target
```

### Frosted Glass Panel
```qml
Rectangle {
    color: Qt.rgba(0.04, 0.09, 0.16, 0.7) // semi-transparent primary
    radius: Theme.radiusMedium

    layer.enabled: true
    layer.effect: FastBlur {
        radius: 24
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Qt Virtual Keyboard module | Custom QML keyboard grids | Qt6 trend | Lighter, more styleable |
| GraphicalEffects (Qt5) | Qt5Compat.GraphicalEffects or MultiEffect | Qt6.0 | Import path changed |
| qmlRegisterType | qmlRegisterSingletonInstance | Qt6 preferred | Cleaner singleton pattern |
| init.d scripts | systemd user services | Raspberry Pi OS Buster+ | Restart policies, journal logging |

## Open Questions

1. **Pi 5 EGLFS Performance with Blur**
   - What we know: FastBlur is lighter than GaussianBlur, layer caching helps
   - What's unclear: Exact performance on Pi 5 GPU with multiple blurred panels at 1920x720
   - Recommendation: Implement with FastBlur, test on hardware, fall back to solid semi-transparent panels if needed

2. **Album Art Dominant Color Extraction in QML**
   - What we know: Canvas element can sample pixels from loaded Image
   - What's unclear: Performance of per-track color extraction in pure QML
   - Recommendation: Sample a 10x10 grid via Canvas.getImageData(), compute average. If too slow, add a C++ helper later.

3. **Touch vs Mouse Events on EGLFS**
   - What we know: EGLFS translates touch to mouse events by default
   - What's unclear: Whether MultiPointTouchArea is needed alongside MouseArea
   - Recommendation: Start with MouseArea, test on Pi, add MultiPointTouchArea if touch detection fails

## Sources

### Primary (HIGH confidence)
- Qt6 QML documentation (qt.io) - Singleton pattern, StackView, Q_PROPERTY binding
- Existing codebase analysis - All state objects, models, and controllers inspected
- CMakeLists.txt - qt_add_qml_module registration pattern already established

### Secondary (MEDIUM confidence)
- Qt5Compat.GraphicalEffects - Confirmed available in Qt 6.8.2
- Raspberry Pi OS systemd practices - Standard Linux service management
- EGLFS platform plugin documentation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Qt6 QML is mature, all patterns well-documented
- Architecture: HIGH - Backend fully built, QML layer is pure binding
- Pitfalls: HIGH - Common Qt/QML issues well-known, Pi deployment patterns established

**Research date:** 2026-02-28
**Valid until:** 2026-03-28 (stable technology, 30-day validity)
