# Architecture Research

**Domain:** Qt6/QML embedded music console kiosk (Raspberry Pi 5)
**Researched:** 2026-02-28
**Confidence:** HIGH

## System Overview

```
+-----------------------------------------------------------------------+
|                          QML UI Layer                                   |
|  +----------+ +----------+ +----------+ +----------+ +----------+     |
|  | NowPlay  | | Library  | | Spotify  | | InputCar | | Overlays |     |
|  +----+-----+ +----+-----+ +----+-----+ +----+-----+ +----+-----+     |
|       |             |            |            |            |            |
+-------+-------------+------------+------------+------------+-----------+
|                      QML Binding Surface                               |
|  +----------------+ +----------------+ +-------------------+           |
|  | ReceiverState  | | PlaybackState  | | UIState           |           |
|  | (Q_PROPERTY)   | | (Q_PROPERTY)   | | (Q_PROPERTY)     |           |
|  +-------+--------+ +-------+--------+ +--------+---------+           |
+----------+------------------+-----------------------+------------------+
|                      Orchestration Layer                               |
|  +----------------+ +-------------------+ +-------------------+        |
|  | PlaybackRouter | | AlbumArtResolver  | | VolumeGesture     |        |
|  |                | |                   | | Controller        |        |
|  +-------+--------+ +--------+----------+ +--------+----------+        |
+----------+-------------------+------------------------+----------------+
|                      Domain Services                                   |
|  +----------+ +----------+ +----------+ +----------+ +----------+     |
|  | Receiver | | Local    | | CD Meta  | | Library  | | Spotify  |     |
|  | Ctrl     | | Playback | | Fetcher  | | Scanner  | | Auth     |     |
|  +----+-----+ +----+-----+ +----+-----+ +----+-----+ +----+-----+     |
|       |             |            |            |            |            |
+-------+-------------+------------+------------+------------+-----------+
|                      Platform Abstraction                              |
|  +----------+ +----------+ +----------+ +----------+ +----------+     |
|  | ALSA     | | GPIO     | | CD Drive | | Display  | | Network  |     |
|  | Output   | | Monitor  | | (libcdio)| | (DDC/CI) | | (Qt)     |     |
|  +----------+ +----------+ +----------+ +----------+ +----------+     |
+-----------------------------------------------------------------------+
|                      Infrastructure                                    |
|  +----------------+ +----------------+ +----------------+              |
|  | AppBuilder     | | AppConfig      | | Logging        |              |
|  | (comp. root)   | | (typed struct) | | (categories)   |              |
|  +----------------+ +----------------+ +----------------+              |
+-----------------------------------------------------------------------+
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| **AppBuilder** | Composition root: constructs object graph, wires signals/slots, returns context | Single class in `src/app/`, called from `main.cpp` (< 20 lines) |
| **AppConfig** | Typed configuration struct, loaded once from QSettings at startup | POD struct with nested sub-configs (ReceiverConfig, AudioConfig, etc.) |
| **ReceiverState** | Q_PROPERTY bag for receiver data (volume, input, power, mute, metadata) | QObject with Q_PROPERTY macros, no business logic, emits change signals |
| **PlaybackState** | Q_PROPERTY bag for local playback (track info, position, state) | QObject with Q_PROPERTY macros, updated by LocalPlaybackController |
| **UIState** | Q_PROPERTY bag for UI concerns (overlay visibility, initialization flag) | QObject with Q_PROPERTY macros, manipulated by QML and controllers |
| **PlaybackRouter** | Routes play/pause/stop/next/prev/seek to correct controller based on current source | Owns the MediaSource state, delegates to LocalPlaybackController or ReceiverController |
| **AlbumArtResolver** | Determines album art URL/path based on current source | Simple strategy: receiver CGI art for streaming, local cache for CD/Library |
| **VolumeGestureController** | Coalesces encoder events into gestures, sends single volume command at gesture end | Owns gesture timer, emits volumeGestureComplete(targetVolume) |
| **EiscpReceiverController** | eISCP TCP protocol implementation, polling, auto-reconnect | QTcpSocket + QTimer polling, parses NTI/NAT/NAL/NTM/NJA2/NST/NMS messages |
| **LocalPlaybackController** | Unified CD/FLAC playback: owns AudioStream + AlsaOutput, runs playback thread | std::thread with atomic flags, parameterized by AudioStream interface |
| **CDMetadataFetcher** | Three-tier async metadata lookup (MusicBrainz, GnuDB, Discogs) with cache | QNetworkAccessManager, SQLite cache via CDMetadataCache |
| **LibraryScanner** | Scans FLAC directory, extracts metadata via TagLib, indexes in SQLite | QtConcurrent::run for async scan, signals scanComplete |
| **SpotifyAuthManager** | OAuth 2.0 flow, token management, device discovery, playback transfer | QNetworkAccessManager for Spotify Web API |
| **PlatformFactory** | Creates real or stub platform implementations based on runtime detection | Factory method returning IGpioMonitor, IAlsaOutput, ICdDrive, IDisplayControl |
| **HttpApiServer** | REST API for external control (volume, input, status, Spotify OAuth) | Qt HttpServer on port 8080 |
| **ScreenTimeoutManager** | State machine: ACTIVE -> DIMMED -> OFF, with DOOR_CLOSED bypass | QTimer-based timeout checks, integrates with DisplayControl |

## Recommended Project Structure

```
media-console-v2/
+-- CMakeLists.txt              # Root: project config, find_package, add_subdirectory
+-- src/
|   +-- CMakeLists.txt          # Main app target, links all libraries
|   +-- main.cpp                # < 20 lines: QGuiApplication + AppBuilder::build()
|   +-- app/
|   |   +-- AppBuilder.h/cpp    # Composition root: constructs, wires, returns context
|   |   +-- AppContext.h        # Holds ownership of all created objects
|   |   +-- AppConfig.h         # Typed config struct (no .cpp needed, header-only)
|   +-- state/
|   |   +-- ReceiverState.h/cpp # Q_PROPERTY bag for receiver data
|   |   +-- PlaybackState.h/cpp # Q_PROPERTY bag for playback data
|   |   +-- UIState.h/cpp       # Q_PROPERTY bag for UI concerns
|   |   +-- MediaSource.h       # enum class MediaSource { Spotify, Phono, CD, ... }
|   +-- receiver/
|   |   +-- IReceiverController.h   # Abstract interface
|   |   +-- EiscpReceiverController.h/cpp
|   |   +-- EiscpProtocol.h/cpp     # Packet building/parsing extracted
|   +-- playback/
|   |   +-- PlaybackRouter.h/cpp    # Source -> controller routing
|   |   +-- LocalPlaybackController.h/cpp  # Unified CD/FLAC controller
|   |   +-- VolumeGestureController.h/cpp
|   +-- audio/
|   |   +-- IAudioOutput.h          # Abstract ALSA output interface
|   |   +-- AlsaOutput.h/cpp        # Real ALSA implementation
|   |   +-- IAudioStream.h          # Abstract stream interface
|   |   +-- CdAudioStream.h/cpp
|   |   +-- FlacAudioStream.h/cpp
|   +-- cd/
|   |   +-- ICdDrive.h              # Abstract CD drive interface
|   |   +-- CdDrive.h/cpp
|   |   +-- CdMonitor.h/cpp
|   |   +-- CdMetadataFetcher.h/cpp
|   |   +-- CdMetadataCache.h/cpp
|   +-- library/
|   |   +-- LibraryDatabase.h/cpp
|   |   +-- LibraryScanner.h/cpp
|   |   +-- LibraryArtistModel.h/cpp
|   |   +-- LibraryAlbumModel.h/cpp
|   |   +-- LibraryTrackBrowseModel.h/cpp
|   |   +-- AlbumArtCache.h/cpp
|   +-- metadata/
|   |   +-- AlbumArtResolver.h/cpp
|   |   +-- MetadataParsers.h/cpp
|   +-- spotify/
|   |   +-- SpotifyAuthManager.h/cpp
|   +-- display/
|   |   +-- IDisplayControl.h       # Abstract display interface
|   |   +-- DisplayControl.h/cpp    # DDC/CI via ddcutil
|   |   +-- ScreenTimeoutManager.h/cpp
|   +-- gpio/
|   |   +-- IGpioMonitor.h          # Abstract GPIO interface
|   |   +-- VolumeEncoderMonitor.h/cpp
|   |   +-- InputEncoderMonitor.h/cpp
|   |   +-- ReedSwitchMonitor.h/cpp
|   +-- api/
|   |   +-- HttpApiServer.h/cpp
|   +-- platform/
|   |   +-- PlatformFactory.h/cpp   # Runtime: real vs stub selection
|   |   +-- StubAlsaOutput.h/cpp
|   |   +-- StubGpioMonitor.h/cpp
|   |   +-- StubCdDrive.h/cpp
|   |   +-- StubDisplayControl.h/cpp
|   +-- config/
|   |   +-- AppConfig.h             # Typed config struct
|   +-- logging/
|       +-- Logging.h/cpp           # Category definitions and init
+-- qml/
|   +-- main.qml
|   +-- Theme.qml                   # Singleton design tokens
|   +-- components/
|   |   +-- NowPlaying.qml
|   |   +-- InputCarousel.qml
|   |   +-- LibraryBrowser.qml
|   |   +-- SpotifySearch.qml
|   |   +-- VolumeOverlay.qml
|   |   +-- VolumeIndicator.qml
|   |   +-- PlaybackControls.qml
|   |   +-- (... other components)
|   +-- pages/                      # Optional: if multi-page layout used
+-- tests/
|   +-- CMakeLists.txt              # Test targets
|   +-- audio/
|   |   +-- test_AlsaOutput.cpp
|   |   +-- test_CdAudioStream.cpp
|   +-- receiver/
|   |   +-- test_EiscpProtocol.cpp
|   |   +-- test_EiscpReceiverController.cpp
|   +-- playback/
|   |   +-- test_LocalPlaybackController.cpp
|   |   +-- test_PlaybackRouter.cpp
|   |   +-- test_VolumeGestureController.cpp
|   +-- cd/
|   |   +-- test_CdMetadataCache.cpp
|   |   +-- test_CdMonitor.cpp
|   +-- state/
|   |   +-- test_ReceiverState.cpp
|   +-- app/
|   |   +-- test_AppBuilder.cpp
|   +-- mocks/
|       +-- MockAudioOutput.h
|       +-- MockAudioStream.h
|       +-- MockCdDrive.h
|       +-- MockGpioMonitor.h
|       +-- MockDisplayControl.h
|       +-- MockReceiverController.h
+-- icons/                          # SVG/PNG assets
+-- scripts/
    +-- install-kiosk.sh
    +-- uninstall-kiosk.sh
```

### Structure Rationale

- **`src/app/`:** Composition root (AppBuilder) is the single place that knows how everything connects. AppContext owns all objects. AppConfig is the typed configuration loaded once. This replaces the 325-line `main.cpp` wiring mess.
- **`src/state/`:** Thin Q_PROPERTY bags that QML binds to. Zero business logic. The decomposed AppState. Each state object has a focused concern: receiver data, playback data, UI state.
- **`src/receiver/`:** eISCP protocol implementation behind an interface. The interface enables testing without a real receiver.
- **`src/playback/`:** PlaybackRouter (source->controller routing), LocalPlaybackController (unified CD/FLAC), VolumeGestureController (encoder coalescing). The router eliminates the 7x duplicated if/else chain from the original AppState.
- **`src/audio/`:** IAudioStream + IAudioOutput interfaces with real and mock implementations. The interface boundary enables testing playback logic without ALSA hardware.
- **`src/platform/`:** Factory that returns real implementations on Linux/RPi, stubs on macOS/test. Replaces `#ifdef __linux__` guards with runtime polymorphism.
- **`tests/mocks/`:** Centralized mock definitions used across all test files. Google Mock MOCK_METHOD on the abstract interfaces.

## Architectural Patterns

### Pattern 1: Composition Root (AppBuilder)

**What:** A single class that constructs the entire object graph, wires all signal/slot connections, and returns an opaque context object. main.cpp becomes trivial.

**When to use:** Always. This is the core architectural pattern for the rewrite.

**Trade-offs:** Requires all dependencies to flow through constructor injection or setter injection. Forces explicit dependency declaration (good). Single point of truth for wiring (good). The builder class can get large for complex apps (mitigate by organizing wiring by subsystem).

**Example:**
```cpp
// main.cpp - under 20 lines
#include <QGuiApplication>
#include "app/AppBuilder.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("MediaConsole");
    QCoreApplication::setApplicationName("media-console");

    auto context = AppBuilder::build(app);
    if (!context) return 1;

    return app.exec();
}

// app/AppBuilder.cpp - organized by subsystem
std::unique_ptr<AppContext> AppBuilder::build(QGuiApplication &app)
{
    auto ctx = std::make_unique<AppContext>();
    auto config = AppConfig::load();
    auto platform = PlatformFactory::create();

    // --- Audio subsystem ---
    ctx->alsaOutput = platform->createAudioOutput(config.audio);
    // ... build LocalPlaybackController ...

    // --- Receiver subsystem ---
    ctx->receiverController = std::make_unique<EiscpReceiverController>();
    // ... wire receiver signals to ReceiverState ...

    // --- Wire subsystems ---
    wireReceiverSignals(*ctx);
    wirePlaybackSignals(*ctx);
    wireGpioSignals(*ctx);
    wireQmlBindings(*ctx, engine);

    return ctx;
}
```

### Pattern 2: Thin Reactive State Objects (Decomposed AppState)

**What:** Split the god-object AppState into focused Q_PROPERTY bags with zero business logic. Each state object is a passive data holder that emits change notifications. Controllers write to state objects; QML reads from them.

**When to use:** For every piece of state that QML needs to observe. This is the primary QML-C++ interface pattern.

**Trade-offs:** More classes to manage (3-4 state objects vs 1 god object). Eliminates 90% of the complexity in the original AppState. QML import list grows slightly but each import is scoped and understandable.

**Example:**
```cpp
// state/ReceiverState.h
class ReceiverState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(double displayVolume READ displayVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool power READ power NOTIFY powerChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
    Q_PROPERTY(MediaSource currentSource READ currentSource
               NOTIFY currentSourceChanged)
    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY metadataChanged)
    Q_PROPERTY(QString trackArtist READ trackArtist NOTIFY metadataChanged)
    // ... more properties ...

public:
    // Only controllers call setters -- not QML
    void setVolume(int volume);
    void setConnected(bool connected);
    // ...

signals:
    void connectedChanged();
    void volumeChanged();
    // ...
};
```

### Pattern 3: Singleton Registration for QML (Not setContextProperty)

**What:** Use `qmlRegisterSingletonInstance()` for exposing C++ objects to QML instead of `setContextProperty()`. This is the modern Qt6 approach.

**When to use:** For all state objects and manager singletons exposed to QML.

**Trade-offs:** Requires `qt_add_qml_module()` in CMake (already used in original). Types must have a specific import path. But: provides type safety, IDE support, proper scoping, and is ~23% faster than setContextProperty in benchmarks.

**Important caveat for singletons with injected dependencies:** QML_SINGLETON with default construction works for self-contained types. For state objects that need to be created by AppBuilder and shared between C++ and QML, use `qmlRegisterSingletonInstance()` in the composition root rather than QML_SINGLETON macro. This gives you the performance and scoping benefits of singletons while keeping AppBuilder in control of construction.

**Example:**
```cpp
// In AppBuilder, after creating state objects:
qmlRegisterSingletonInstance("MediaConsole", 1, 0,
    "ReceiverState", ctx->receiverState.get());
qmlRegisterSingletonInstance("MediaConsole", 1, 0,
    "PlaybackState", ctx->playbackState.get());
qmlRegisterSingletonInstance("MediaConsole", 1, 0,
    "UIState", ctx->uiState.get());

// In QML:
// import MediaConsole 1.0
// Text {
//     text: ReceiverState.trackTitle
// }
```

**Confidence:** HIGH -- Qt official documentation explicitly deprecates setContextProperty and recommends QML_ELEMENT/singleton registration. Verified via Qt 6.10 docs.

### Pattern 4: Interface + Factory for Platform Abstraction

**What:** Define abstract interfaces for all hardware-dependent components (ALSA, GPIO, CD drive, display). A PlatformFactory creates real or stub implementations at runtime based on platform detection.

**When to use:** For every hardware dependency. This is a settled decision from PROJECT.md.

**Trade-offs:** Adds one interface + one stub per hardware component (~12 extra files). Eliminates all `#ifdef __linux__` guards. Enables full test path on macOS. Development machine becomes a first-class citizen for testing.

**Example:**
```cpp
// audio/IAudioOutput.h
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;
    virtual bool open(const AudioConfig &config) = 0;
    virtual void close() = 0;
    virtual WriteResult writeFrames(const int16_t *data, size_t frames) = 0;
    virtual void pause(bool paused) = 0;
    virtual void reset() = 0;
};

// platform/PlatformFactory.h
class PlatformFactory {
public:
    static std::unique_ptr<PlatformFactory> create();
    virtual std::unique_ptr<IAudioOutput> createAudioOutput(
        const AudioConfig &config) = 0;
    virtual std::unique_ptr<IGpioMonitor> createVolumeEncoder() = 0;
    virtual std::unique_ptr<ICdDrive> createCdDrive() = 0;
    virtual std::unique_ptr<IDisplayControl> createDisplayControl() = 0;
};

// platform/LinuxPlatformFactory.cpp
std::unique_ptr<IAudioOutput> LinuxPlatformFactory::createAudioOutput(
    const AudioConfig &config)
{
    return std::make_unique<AlsaOutput>(config);
}

// platform/StubPlatformFactory.cpp
std::unique_ptr<IAudioOutput> StubPlatformFactory::createAudioOutput(
    const AudioConfig &config)
{
    return std::make_unique<StubAudioOutput>();
}
```

### Pattern 5: Unified LocalPlaybackController with AudioStream Interface

**What:** A single playback controller that accepts any `IAudioStream` implementation (CdAudioStream or FlacAudioStream). The controller owns the playback thread, audio output, and state management. Switching sources means swapping the stream, not the controller.

**When to use:** This directly replaces the duplicated CdPlaybackController and FlacPlaybackController from the original.

**Trade-offs:** Requires a common AudioStream interface that both CD and FLAC can implement. CD uses sector-based seeking; FLAC uses sample-based. The interface must abstract this difference (use frame-based positions, let each implementation convert internally). The payoff: ALSA device exclusivity is solved structurally (one controller = one device), and the identical playback loop code is written once.

**Example:**
```cpp
// audio/IAudioStream.h
class IAudioStream {
public:
    virtual ~IAudioStream() = default;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual size_t readFrames(int16_t *buffer, size_t frames) = 0;
    virtual int64_t totalFrames() const = 0;
    virtual bool seekToPosition(int64_t frame) = 0;
    virtual int sampleRate() const = 0;
    virtual int channels() const = 0;
    // Track management (for multi-track sources like CD)
    virtual int trackCount() const = 0;
    virtual bool seekToTrack(int track) = 0;
    virtual int currentTrack() const = 0;
};

// playback/LocalPlaybackController.h
class LocalPlaybackController : public QObject {
    Q_OBJECT
public:
    void setAudioStream(std::unique_ptr<IAudioStream> stream);
    void setAudioOutput(std::unique_ptr<IAudioOutput> output);

    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void seek(int positionSeconds);

signals:
    void playbackStateChanged(PlaybackState state);
    void trackChanged(const TrackInfo &info);
    void positionChanged(int seconds);
    void audioRecoveryFailed();

private:
    void playbackLoop();  // Runs in std::thread
    std::unique_ptr<IAudioStream> m_stream;
    std::unique_ptr<IAudioOutput> m_output;
    std::thread m_playbackThread;
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_paused{false};
    std::atomic<int> m_pendingSeek{-1};
    std::atomic<int> m_pendingTrack{-1};
};
```

### Pattern 6: Signal Subsystem Wiring in AppBuilder

**What:** Group signal/slot connections by subsystem in the composition root. Each subsystem gets its own wiring method, keeping the builder organized as the application grows.

**When to use:** In AppBuilder, for all inter-component signal/slot connections.

**Trade-offs:** Wiring is centralized (easy to audit, single source of truth) but can grow large. Subsystem grouping keeps it manageable. Alternative (components wire themselves) distributes knowledge and makes the graph harder to reason about.

**Example:**
```cpp
void AppBuilder::wireReceiverSignals(AppContext &ctx)
{
    auto *rc = ctx.receiverController.get();
    auto *rs = ctx.receiverState.get();

    connect(rc, &EiscpReceiverController::volumeChanged,
            rs, &ReceiverState::setVolume);
    connect(rc, &EiscpReceiverController::inputChanged,
            rs, [rs](auto input) { rs->setCurrentInput(input); });
    connect(rc, &EiscpReceiverController::powerChanged,
            rs, &ReceiverState::setPower);
    connect(rc, &EiscpReceiverController::muteChanged,
            rs, &ReceiverState::setMuted);
    connect(rc, &EiscpReceiverController::connected,
            rs, [rs]() { rs->setConnected(true); });
    connect(rc, &EiscpReceiverController::disconnected,
            rs, [rs]() { rs->setConnected(false); });
    // ... metadata signals ...
}

void AppBuilder::wirePlaybackSignals(AppContext &ctx)
{
    auto *lpc = ctx.localPlaybackController.get();
    auto *ps = ctx.playbackState.get();

    connect(lpc, &LocalPlaybackController::playbackStateChanged,
            ps, &PlaybackState::setPlaybackState);
    connect(lpc, &LocalPlaybackController::trackChanged,
            ps, &PlaybackState::setTrackInfo);
    connect(lpc, &LocalPlaybackController::positionChanged,
            ps, &PlaybackState::setPosition);
}

void AppBuilder::wireGpioSignals(AppContext &ctx)
{
    auto *ve = ctx.volumeEncoder.get();
    auto *vg = ctx.volumeGestureController.get();

    connect(ve, &IGpioMonitor::rotated,
            vg, &VolumeGestureController::onEncoderStep);
    connect(vg, &VolumeGestureController::gestureComplete,
            ctx.receiverController.get(),
            &IReceiverController::setVolume);
}
```

## Data Flow

### Primary Data Flows

```
1. User Touch/Encoder Input
   [Touch/GPIO] -> [VolumeGestureController or PlaybackRouter]
       -> [ReceiverController or LocalPlaybackController]
       -> [ReceiverState or PlaybackState] (property update)
       -> [QML UI] (binding notification)

2. Receiver State Updates (polling)
   [Receiver TCP] -> [EiscpReceiverController] (parse eISCP)
       -> [ReceiverState] (setVolume, setInput, etc.)
       -> [QML UI] (binding notification)

3. CD Insertion
   [CDMonitor] --cdInserted--> [AppBuilder wiring]
       -> [CdMetadataFetcher.fetchMetadata()] (async)
       -> [PlaybackState.setCdPresent(true)]
       -> [QML: show eject button]
   Meanwhile: [CdMetadataFetcher]
       -> cache check -> network lookup chain
       -> [PlaybackState.setTrackInfo()] (progressive)
       -> [QML: track list updates progressively]

4. Local Playback (CD or FLAC)
   [PlaybackRouter.play()] -> [LocalPlaybackController.play()]
       -> [std::thread: playbackLoop()]
           -> [IAudioStream.readFrames()] (CD paranoia or FLAC decode)
           -> [IAudioOutput.writeFrames()] (ALSA PCM write)
       -> [positionChanged signal] -> [PlaybackState]
       -> [QML: progress bar updates]

5. Volume Encoder Gesture
   [VolumeEncoderMonitor] --volumeUp/Down-->
       [VolumeGestureController]
           -> update optimistic volume -> [ReceiverState.setVolume()]
           -> [QML: volume overlay shows immediately]
           -> (gesture timer expires)
           -> [ReceiverController.setVolume(finalTarget)]
           -> (receiver confirms) -> reconcile
```

### Thread Architecture

```
Main Thread (Qt Event Loop):
  - QML rendering
  - Signal/slot dispatch
  - QNetworkAccessManager callbacks
  - Timer callbacks (polling, timeout, gesture)
  - All Q_PROPERTY updates

Playback Thread (std::thread):
  - AudioStream.readFrames() [blocking I/O]
  - AudioOutput.writeFrames() [blocking I/O]
  - Communicates with main thread via:
    - atomic flags (stop, pause, pending seek/track)
    - QMetaObject::invokeMethod(Qt::QueuedConnection) for signals

GPIO Monitor Threads (QThread):
  - One thread per monitor (volume encoder, input encoder, reed switch)
  - libgpiod event wait [blocking]
  - Emit signals to main thread via Qt::QueuedConnection

CD Metadata Thread (QtConcurrent):
  - Disc ID calculation via libdiscid [blocking]
  - QFutureWatcher bridges back to main thread

Library Scanner Thread (QtConcurrent):
  - Directory traversal + TagLib extraction [blocking]
  - Signals scanComplete to main thread
```

### Key Data Flow Rules

1. **QML never calls hardware directly.** All QML interactions go through Q_INVOKABLE methods on state objects or the PlaybackRouter, which delegate to the appropriate controller.
2. **Controllers never modify state objects directly from background threads.** Use `QMetaObject::invokeMethod(Qt::QueuedConnection)` or emit signals connected with `Qt::QueuedConnection` to marshal back to the main thread.
3. **Configuration flows one direction: AppConfig -> constructors.** No component reads QSettings after construction.
4. **Volume changes have two paths:** (a) user-initiated -> optimistic UI update + gesture coalescing + receiver command, and (b) receiver-reported -> silent state update, no overlay.

## Anti-Patterns

### Anti-Pattern 1: God Object State Manager

**What people do:** Put all application state, business logic, command routing, and UI state into a single AppState class.
**Why it's wrong:** The original AppState is 310 lines of header, handles 7 distinct responsibilities, and requires 20+ signal connections in main.cpp. Every change risks breaking unrelated features. Testing requires constructing the entire object graph.
**Do this instead:** Decompose into ReceiverState, PlaybackState, UIState (passive data), and PlaybackRouter, AlbumArtResolver, VolumeGestureController (active logic). Each class has a single reason to change.

### Anti-Pattern 2: setContextProperty for QML Exposure

**What people do:** `engine.rootContext()->setContextProperty("appState", appState)` for every object QML needs.
**Why it's wrong:** Context properties are globally scoped (namespace collision risk), not type-checked (IDE cannot validate), slower than registered types (~23% in benchmarks), and deprecated in Qt6. QML files have invisible dependencies on names injected elsewhere.
**Do this instead:** Use `qmlRegisterSingletonInstance()` for objects created by AppBuilder, or `QML_ELEMENT`/`QML_SINGLETON` for self-contained types. QML uses explicit `import MediaConsole 1.0`.

### Anti-Pattern 3: ifdef Platform Guards

**What people do:** `#ifdef __linux__` blocks around GPIO, ALSA, and CD code, with empty else branches or stub behavior inline.
**Why it's wrong:** Platform-specific code is scattered throughout the codebase. Tests on macOS skip all hardware-dependent paths. Conditional compilation makes the code harder to read and maintain.
**Do this instead:** Define interfaces (IAudioOutput, IGpioMonitor, ICdDrive, IDisplayControl). PlatformFactory returns real implementations on Linux, stubs elsewhere. All code paths execute on all platforms. Tests use mock implementations via Google Mock.

### Anti-Pattern 4: Blocking the Qt Event Loop

**What people do:** Synchronous network calls, synchronous disc ID calculation, or heavy computation on the main thread.
**Why it's wrong:** The original codebase has a known production bug where CD metadata fetching freezes the entire UI for 15-20 seconds. For a kiosk shown to visitors, this is unacceptable.
**Do this instead:** All I/O operations use one of: (a) QNetworkAccessManager (inherently async), (b) QtConcurrent::run for blocking operations like disc ID calculation, (c) std::thread for the playback loop, (d) QThread for GPIO monitoring. Bridge back to main thread via signals or QMetaObject::invokeMethod.

### Anti-Pattern 5: Duplicated Command Routing

**What people do:** Copy the same if/activeSource/else logic into play(), pause(), stop(), next(), previous(), seek(), and playPause() -- 7 methods with identical dispatch logic.
**Why it's wrong:** The original AppState duplicates this pattern 7 times. Adding a new source or changing routing requires editing 7 methods identically.
**Do this instead:** PlaybackRouter owns the MediaSource state and provides a single dispatch point. play() calls `router->activeController()->play()`, etc. The routing decision is made once, not 7 times.

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| Onkyo Receiver | TCP socket (eISCP binary protocol), port 60128 | 2.5s polling interval, auto-reconnect on disconnect. Consider telnet server as supplement for event-driven updates. |
| MusicBrainz | REST API via QNetworkAccessManager | Rate-limited (1 req/sec). Use libdiscid for disc ID. |
| GnuDB | CDDB protocol via HTTP GET | Validate response before caching. Provides artist/album hints for Discogs fallback. |
| Discogs | REST API via QNetworkAccessManager | Requires API key. Used as third-tier fallback for CD metadata and album art. |
| Spotify | OAuth 2.0 + REST API via QNetworkAccessManager | Token persistence in QSettings. Device discovery by name. |
| CoverArtArchive | REST API (redirects to actual image URL) | Provides album art for CDs found via MusicBrainz. |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| QML to C++ State | Q_PROPERTY bindings + Q_INVOKABLE calls | State objects are registered singleton instances |
| Main Thread to Playback Thread | Atomic flags (write from main, read from thread) + signals with QueuedConnection (emit from thread, receive on main) | Never share mutable state without atomics or mutexes |
| Main Thread to GPIO Threads | Qt signals with QueuedConnection (GPIO thread emits, main thread receives) | Each GPIO monitor runs its own QThread |
| AppBuilder to All Components | Constructor/setter injection at build time | No component discovers its dependencies; all are provided |
| Controllers to State Objects | Controllers call setters on state objects; state objects emit Q_PROPERTY change signals | Unidirectional: controllers write, QML reads |

## CMake Architecture

### Target Structure

Use a library-per-domain model for modular builds and faster incremental compilation:

```cmake
# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(media-console VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Quick Network HttpServer Concurrent Sql)

add_subdirectory(src)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()
```

```cmake
# src/CMakeLists.txt
# Core library (platform-independent logic)
add_library(media-console-core STATIC
    state/ReceiverState.cpp state/ReceiverState.h
    state/PlaybackState.cpp state/PlaybackState.h
    state/UIState.cpp state/UIState.h
    state/MediaSource.h
    receiver/EiscpReceiverController.cpp receiver/EiscpReceiverController.h
    receiver/EiscpProtocol.cpp receiver/EiscpProtocol.h
    playback/PlaybackRouter.cpp playback/PlaybackRouter.h
    playback/LocalPlaybackController.cpp playback/LocalPlaybackController.h
    playback/VolumeGestureController.cpp playback/VolumeGestureController.h
    # ... audio, cd, library, metadata, spotify, display, api, logging ...
)
target_include_directories(media-console-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(media-console-core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Network Qt6::HttpServer
    Qt6::Concurrent Qt6::Sql)

# Platform library (real hardware implementations)
add_library(media-console-platform STATIC
    audio/AlsaOutput.cpp audio/AlsaOutput.h
    gpio/VolumeEncoderMonitor.cpp gpio/VolumeEncoderMonitor.h
    gpio/InputEncoderMonitor.cpp gpio/InputEncoderMonitor.h
    gpio/ReedSwitchMonitor.cpp gpio/ReedSwitchMonitor.h
    # ...
)
target_link_libraries(media-console-platform PUBLIC media-console-core)
# Platform-specific linking handled per-target

# Main executable
qt_add_executable(media-console main.cpp app/AppBuilder.cpp)
target_link_libraries(media-console PRIVATE
    media-console-core media-console-platform)
qt_add_qml_module(media-console URI MediaConsole VERSION 1.0
    QML_FILES ../qml/main.qml ../qml/Theme.qml
    # ... all QML files ...
)
```

This structure means tests link against `media-console-core` without pulling in platform-specific hardware libraries, enabling test compilation and execution on any platform.

## Test Architecture

### Google Test + Qt Integration

**Framework:** Google Test (gtest) with Google Mock (gmock) as the primary test framework. Qt::Test utilities (QSignalSpy, QTest::qWait) used alongside for signal verification.

**Key patterns:**

1. **Mock interfaces with gmock:** All abstract interfaces (IAudioOutput, IAudioStream, ICdDrive, IGpioMonitor, IReceiverController, IDisplayControl) get corresponding gmock mocks.

2. **QSignalSpy for signal verification:** Use Qt's QSignalSpy to verify that state objects emit the correct change notifications without needing actual QML bindings.

3. **QCoreApplication in test main:** Tests that use signals/slots need a QCoreApplication instance and `processEvents()` calls.

4. **No hardware in tests:** Every test uses mock/stub implementations. Tests run on macOS and in CI without any hardware.

**Example:**
```cpp
// tests/mocks/MockAudioOutput.h
class MockAudioOutput : public IAudioOutput {
public:
    MOCK_METHOD(bool, open, (const AudioConfig &config), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(WriteResult, writeFrames,
                (const int16_t *data, size_t frames), (override));
    MOCK_METHOD(void, pause, (bool paused), (override));
    MOCK_METHOD(void, reset, (), (override));
};

// tests/playback/test_LocalPlaybackController.cpp
class LocalPlaybackControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_mockStream = std::make_unique<MockAudioStream>();
        m_mockOutput = std::make_unique<MockAudioOutput>();
        m_controller = std::make_unique<LocalPlaybackController>();
        // Keep raw pointers for expectations before moving ownership
        m_rawStream = m_mockStream.get();
        m_rawOutput = m_mockOutput.get();
        m_controller->setAudioStream(std::move(m_mockStream));
        m_controller->setAudioOutput(std::move(m_mockOutput));
    }
    // ...
};

TEST_F(LocalPlaybackControllerTest, PlayOpensStreamAndOutput) {
    EXPECT_CALL(*m_rawStream, open()).WillOnce(Return(true));
    EXPECT_CALL(*m_rawOutput, open(_)).WillOnce(Return(true));

    QSignalSpy stateSpy(m_controller.get(),
        &LocalPlaybackController::playbackStateChanged);
    m_controller->play();
    QTest::qWait(100);  // Allow thread to start

    ASSERT_EQ(stateSpy.count(), 1);
    // Verify Playing state was emitted
}
```

### gmock + QObject Compatibility Note

Google Mock's `MOCK_METHOD` works on abstract interfaces that do NOT inherit from QObject. Since the abstract interfaces (IAudioOutput, IAudioStream, ICdDrive) are pure C++ classes (not QObjects), gmock works without issues. For mocking QObject-based classes with signals/slots, prefer hand-written mocks that inherit from the abstract interface and emit signals manually, since gmock's `MOCK_METHOD` cannot generate Qt signals.

## Build Order (Dependency Graph)

The following build order reflects component dependencies. Each layer depends only on layers below it.

```
Phase 1: Foundation
  AppConfig, MediaSource enum, Logging, PlatformFactory (stubs)
  IAudioOutput, IAudioStream, ICdDrive, IGpioMonitor, IDisplayControl
  -> No external deps. Pure C++17 + Qt Core. Testable immediately.

Phase 2: State Layer
  ReceiverState, PlaybackState, UIState
  -> Depends on: MediaSource enum
  -> Testable with QSignalSpy

Phase 3: Audio Pipeline
  AlsaOutput (real + stub), CdAudioStream, FlacAudioStream
  LocalPlaybackController
  -> Depends on: IAudioOutput, IAudioStream, PlaybackState
  -> Testable with mocks

Phase 4: Receiver Communication
  EiscpProtocol (packet build/parse), EiscpReceiverController
  -> Depends on: ReceiverState, IReceiverController
  -> Testable with mock TCP socket

Phase 5: Orchestration
  PlaybackRouter, VolumeGestureController, AlbumArtResolver
  -> Depends on: State layer, LocalPlaybackController, ReceiverController
  -> Testable with mocks of both controllers

Phase 6: CD Subsystem
  CdDrive, CdMonitor, CdMetadataFetcher, CdMetadataCache
  -> Depends on: ICdDrive, IAudioStream (CdAudioStream)
  -> Network-dependent parts testable with recorded responses

Phase 7: Library Subsystem
  LibraryDatabase, LibraryScanner, Browse models, AlbumArtCache
  -> Depends on: IAudioStream (FlacAudioStream)
  -> Testable with temporary SQLite databases

Phase 8: Spotify + Display + GPIO
  SpotifyAuthManager, DisplayControl, ScreenTimeoutManager,
  VolumeEncoderMonitor, InputEncoderMonitor, ReedSwitchMonitor
  -> Depends on: State layer, platform interfaces
  -> GPIO testable with mock monitors

Phase 9: API + Composition
  HttpApiServer, AppBuilder (composition root)
  -> Depends on: Everything above
  -> Integration tests verify wiring

Phase 10: QML UI
  All QML components with updated bindings
  -> Depends on: State layer (registered singleton instances)
  -> Manual/visual testing on target hardware
```

## Sources

- [Qt 6.10 Documentation: Embedding C++ Objects into QML with Context Properties](https://doc.qt.io/qt-6/qtqml-cppintegration-contextproperties.html) -- confirms setContextProperty deprecation status
- [Qt 6.10 Documentation: Defining QML Types from C++](https://doc.qt.io/qt-6/qtqml-cppintegration-definetypes.html) -- QML_ELEMENT and QML_SINGLETON macro usage
- [Qt 6.10 Documentation: Singletons in QML](https://doc.qt.io/qt-6/qml-singleton.html) -- singleton registration patterns
- [Qt 6.10 Documentation: qt_add_qml_module](https://doc.qt.io/qt-6/qt-add-qml-module.html) -- CMake QML module configuration
- [Qt 6.10 Documentation: Signals and Slots](https://doc.qt.io/qt-6/signalsandslots.html) -- connection types including Qt::QueuedConnection for cross-thread
- [Qt 6.10 Documentation: Getting started with CMake](https://doc.qt.io/qt-6/cmake-get-started.html) -- CMake setup and target-based architecture
- [Qt 6.10 Documentation: Qt Test Best Practices](https://doc.qt.io/qt-6/qttest-best-practices-qdoc.html) -- testing patterns
- [Raymii.org: Qt/QML Integrate C++ with QML and why ContextProperties are bad](https://raymii.org/s/articles/Qt_QML_Integrate_Cpp_with_QML_and_why_ContextProperties_are_bad.html) -- performance benchmarks showing setContextProperty is 23% slower
- [LearnQt Guide: From Zero to Production Qt6 QML Architecture](https://www.learnqt.guide/qt6-qml-project-architecture) -- Manager pattern and project structure
- Original codebase at `~/Code/media-console` -- AppState.h (310 lines), main.cpp (325 lines), all controller interfaces

---
*Architecture research for: Qt6/QML embedded music console kiosk*
*Researched: 2026-02-28*
