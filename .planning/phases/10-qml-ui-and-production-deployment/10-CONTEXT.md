# Phase 10: QML UI and Production Deployment - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

The complete touch interface running as a kiosk on the 1920x720 display with all QML components visually integrated, a theme system with design tokens, and production deployment via systemd. This phase replaces the test-harness main.qml with the real UI. Production deployment includes kiosk systemd service and install/uninstall scripts.

</domain>

<decisions>
## Implementation Decisions

### Visual style and polish
- Soft glass/blur aesthetic — frosted glass panels, backdrop blur behind overlays, subtle translucency (iOS control center / KDE Plasma feel)
- Smooth and fluid transitions — 300-500ms with eased slide/scale and spring physics. Polished and deliberate, not snappy.
- Dynamic accent colors extracted from album art — dominant color drives progress bar, accent highlights, and subtle background tint. Shifts mood with the music.
- Generous touch targets — 64-80px minimum. Larger hit areas with more padding between controls. Forgiving for casual/unfamiliar kiosk users.

### Input carousel
- Vertical wheel style — items stack vertically like a slot machine wheel. Selected item centered and full-size, neighbors shrink above/below. Natural fit for the narrow left panel.
- Icon-heavy items — large recognizable icons (vinyl, CD, Bluetooth symbol, etc.) with small label below. Visual recognition first.
- Progress ring countdown for 4-second auto-select — circular progress indicator around the selected icon fills over 4 seconds. Tap or encoder click selects immediately.
- Always visible — carousel stays permanently in the left panel. Current input highlighted. User can always see and switch inputs without extra interaction.

### Now Playing layout
- Dominant album art — ~60% of the right panel. Large, immersive, front-and-center. Track info and controls compact beside/below it.
- Tap to flip album art between front and back cover — 3D card-flip animation. Only shown when back art is available.
- Adaptive playback controls — show only controls that work for the active source:
  - CD/FLAC: prev/play/next/seek
  - Spotify: shuffle/prev/play/next/repeat/seek
  - Bluetooth: play/pause only
  - No dead/grayed-out buttons
- Idle state shows last played — when nothing is playing, display the last played track's art and info dimmed. Screen always has something on it, feels lived-in.

### Library browser
- Album art grid — 2-3 columns of album covers when browsing an artist's albums. Visual, leverages existing FLAC picture block art.
- Drag scrub A-Z sidebar — vertical letter strip supports both tap-to-jump and drag-to-scrub. Shows floating preview of current letter during drag.
- Split track page — album art + album info (artist, year, duration) on the left third, scrollable track list on the right two-thirds. Takes advantage of the wide display.

### Spotify search
- Fullscreen overlay with on-screen QWERTY keyboard
- Live results with debounce — results update as you type, keyboard stays visible at the bottom, results populate above it. Responsive and immediate.

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

</decisions>

<specifics>
## Specific Ideas

- Glass aesthetic like iOS control center or KDE Plasma — layered depth with frosted panels
- Input carousel should feel like a slot machine wheel — satisfying rotation between inputs
- Album art should be the hero of the Now Playing view — the dominant visual element
- Dynamic color should shift the mood with the music — accent colors track the current album art
- A-Z sidebar should work like iOS contacts — drag scrub with floating letter preview
- Track page split layout inspired by desktop music players — art on left, tracks on right
- Spotify search should feel instant — live results appearing as you type

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- ReceiverState (src/state/ReceiverState.h): QML singleton with volume, input, power, mute, metadata — drives NowPlaying and VolumeOverlay
- PlaybackState (src/state/PlaybackState.h): QML singleton with playback mode, position, duration, track info — drives NowPlaying controls and progress bar
- UIState (src/state/UIState.h): QML singleton with activeView, volumeOverlayVisible, errorBannerVisible, toastVisible/message/type, receiverConnected, audioError, doorOpen, screenDimmed — drives view switching and overlay visibility
- ActiveView enum (NowPlaying, LibraryBrowser, SpotifySearch): Already defines the three main views
- PlaybackRouter (src/orchestration/PlaybackRouter.cpp): Dispatches playback commands to correct controller by source — QML controls should call through this
- AlbumArtResolver (src/orchestration/AlbumArtResolver.cpp): Resolves album art URL by source — QML binds to this for art display
- Library browse models (src/library/): Artist, Album, Track QAbstractListModel subclasses — ready for QML ListView/GridView binding
- SpotifyController (src/spotify/SpotifyController.cpp): Search, playback, auth — ready for QML search overlay binding
- ScreenTimeoutController (src/display/ScreenTimeoutController.cpp): Drives screen dim/off — needs touch activity signal from QML global MouseArea

### Established Patterns
- State singletons registered via qmlRegisterSingletonInstance() — all state accessed directly in QML as ReceiverState.property
- Signal/slot for all state changes — QML property bindings auto-update
- MediaSource enum with hex code conversion — maps user-facing sources to receiver commands
- CommandSource enum distinguishes local (encoder, touch) vs external input — drives volume overlay visibility logic

### Integration Points
- main.qml (src/qml/main.qml): Currently a test harness — will be completely replaced with the real UI
- AppBuilder (src/app/AppBuilder.cpp): Wires the object graph — may need to register new QML types or expose additional objects
- CMakeLists.txt: Currently references only main.qml — will need all new QML files added to qt_add_qml_module
- UIState.showToast signal: Transient signal for toast display — QML needs to connect and show toast notifications
- ScreenTimeoutController needs touch activity events from the global MouseArea (UI-18)

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 10-qml-ui-and-production-deployment*
*Context gathered: 2026-02-28*
