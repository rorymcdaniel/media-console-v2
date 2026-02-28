# Phase 2: State Layer and QML Binding Surface - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Thin reactive state objects (ReceiverState, PlaybackState, UIState) that QML binds to as singletons, plus a MediaSource enum with hex code conversion. No business logic — just Q_PROPERTY bags that later phases will populate. State objects are registered via qmlRegisterSingletonInstance() and wired into AppBuilder/AppContext.

</domain>

<decisions>
## Implementation Decisions

### Track Metadata Shape
- Individual Q_PROPERTYs for each metadata field (title, artist, album, albumArtUrl, duration) — no grouped struct
- ReceiverState and PlaybackState each have their own distinct metadata properties
- ReceiverState metadata comes from eISCP streaming (title, artist, album, albumArtUrl, fileInfo, serviceName)
- PlaybackState metadata comes from local CD/FLAC playback (title, artist, album, albumArtUrl)
- Clean property names only — eISCP command codes (NTI, NAT, NAL, NJA2) stay internal to receiver controller
- Each property emits its own change signal

### Position/Duration Tracking
- PlaybackState.positionMs and durationMs are unified for ALL sources (local and streaming)
- Timer-driven position updates (e.g., 250ms QTimer incrementing positionMs while playing) for smooth progress bar
- Backend corrects drift on seek or track change
- Streaming sources update from eISCP NTM commands through the same properties

### MediaSource Enum
- 7 values: None, Streaming, Phono, CD, Computer, Bluetooth, Library
- MediaSource::None as default for startup/disconnected states
- Streaming sub-services tracked via separate ReceiverState.streamingService property (StreamingService enum: Unknown, Spotify, Pandora, AirPlay, AmazonMusic, Chromecast)
- Free functions toHexCode(MediaSource) and fromHexCode(uint8_t) co-located in the MediaSource header
- Registered as QML enum type via Q_ENUM + qmlRegisterUncreatableType for direct QML access (MediaSource.CD)

### UIState Scope
- ActiveView enum defined upfront with all known views: NowPlaying, LibraryBrowser, SpotifySearch
- Boolean Q_PROPERTYs per overlay: volumeOverlayVisible, errorBannerVisible, toastVisible
- Toast state: toastMessage (QString) and toastType (QString) properties alongside toastVisible
- Persistent error properties: receiverConnected (bool), audioError (QString) — QML binds error banners directly
- Transient errors emitted as signals for toast display
- Changing input source sets a default view (CD→NowPlaying, Library→LibraryBrowser, Streaming→NowPlaying) — user can still navigate away

### Playback Mode
- Simple tri-state PlaybackMode enum: Stopped, Playing, Paused
- PlaybackState.activeSource (MediaSource) tracks which source is currently producing audio — distinct from ReceiverState.currentInput
- trackNumber (int) and trackCount (int) included for album/playlist navigation context (0/0 when not applicable)

### Claude's Discretion
- Exact QTimer interval for position updates
- Default values for all state properties
- Whether to use Q_GADGET for StreamingService enum or host it on a helper class
- Test harness implementation approach (QML test vs C++ signal verification)
- Property naming style (camelCase per Qt convention assumed)

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard Qt6/QML patterns. The state objects should feel like standard Qt property bags that any Qt developer would recognize.

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- AppBuilder (src/app/AppBuilder.h): Composition root — state objects will be constructed here and wired into AppContext
- AppContext (src/app/AppContext.h): Already has placeholder comments for ReceiverState*, PlaybackState*, UIState* pointers
- AppConfig (src/app/AppConfig.h): Establishes the pattern of grouped config structs — state objects follow similar grouping philosophy
- main.qml (src/qml/main.qml): Bare 1920x720 window with #0a1628 background — QML test harness will extend this

### Established Patterns
- QObject parent chain for ownership, unique_ptr in AppBuilder, non-owning pointers in AppContext
- Forward declarations in headers, includes in .cpp files
- Platform abstraction via interfaces (I-prefix) — state objects are concrete, not abstract
- Q_OBJECT macro usage in AppBuilder

### Integration Points
- AppBuilder::build() needs to create state objects and add pointers to AppContext
- main.cpp needs qmlRegisterSingletonInstance() calls before engine.load()
- State objects become QML globals accessible from any .qml file
- Future controller phases will take state object pointers and call setters

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-state-layer-and-qml-binding-surface*
*Context gathered: 2026-02-28*
