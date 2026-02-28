# Phase 2: State Layer and QML Binding Surface - Research

**Researched:** 2026-02-28
**Domain:** Qt6 Q_PROPERTY / QML singleton binding
**Confidence:** HIGH

## Summary

Phase 2 creates three thin QObject-derived state classes (ReceiverState, PlaybackState, UIState), a MediaSource enum with hex code conversion, and registers everything as QML singletons. These are pure property bags — no business logic, no networking, no timers. Later phases populate them via setters.

The Qt6 Q_PROPERTY system is mature and well-documented. The key patterns are: Q_PROPERTY macros with READ/WRITE/NOTIFY, Q_ENUM for enum registration, qmlRegisterSingletonInstance() for making C++ objects available as QML globals, and qmlRegisterUncreatableType() for enum-only types. The existing codebase already uses Q_OBJECT (AppBuilder), unique_ptr ownership in AppBuilder, and non-owning pointers in AppContext — state objects follow the same pattern exactly.

**Primary recommendation:** Implement each state class as a minimal QObject with Q_PROPERTY macros and change signals. Keep all three in a `src/state/` directory. Register as QML singletons in main.cpp before engine.load(). Test with Google Test verifying signal emission on property changes.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Individual Q_PROPERTYs for each metadata field (title, artist, album, albumArtUrl, duration) — no grouped struct
- ReceiverState and PlaybackState each have their own distinct metadata properties
- ReceiverState metadata: title, artist, album, albumArtUrl, fileInfo, serviceName (from eISCP streaming)
- PlaybackState metadata: title, artist, album, albumArtUrl (from local CD/FLAC playback)
- Clean property names only — eISCP command codes stay internal to receiver controller
- Each property emits its own change signal
- PlaybackState.positionMs and durationMs unified for ALL sources
- Timer-driven position updates for smooth progress bar (not in this phase — just the property)
- MediaSource enum: 7 values (None, Streaming, Phono, CD, Computer, Bluetooth, Library)
- MediaSource::None as default for startup/disconnected states
- StreamingService enum: Unknown, Spotify, Pandora, AirPlay, AmazonMusic, Chromecast
- Free functions toHexCode(MediaSource) and fromHexCode(uint8_t) co-located in MediaSource header
- Registered as QML enum type via Q_ENUM + qmlRegisterUncreatableType
- UIState ActiveView enum: NowPlaying, LibraryBrowser, SpotifySearch
- Boolean Q_PROPERTYs per overlay: volumeOverlayVisible, errorBannerVisible, toastVisible
- Toast state: toastMessage (QString), toastType (QString), toastVisible (bool)
- Persistent error properties: receiverConnected (bool), audioError (QString)
- Transient errors emitted as signals for toast display
- Input source sets default view (CD->NowPlaying, Library->LibraryBrowser, Streaming->NowPlaying)
- PlaybackMode enum: Stopped, Playing, Paused
- PlaybackState.activeSource (MediaSource) tracks current audio source
- trackNumber (int) and trackCount (int) for album/playlist navigation

### Claude's Discretion
- Exact QTimer interval for position updates (not relevant this phase — just the property)
- Default values for all state properties
- Whether to use Q_GADGET for StreamingService enum or host it on a helper class
- Test harness implementation approach (QML test vs C++ signal verification)
- Property naming style (camelCase per Qt convention assumed)

### Deferred Ideas (OUT OF SCOPE)
- None — discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| STATE-01 | ReceiverState as thin Q_PROPERTY bag exposing volume, input, power, mute, metadata | Q_PROPERTY macros with NOTIFY, Q_OBJECT class, individual properties per CONTEXT.md |
| STATE-02 | PlaybackState as thin Q_PROPERTY bag exposing playback mode, position, duration, track info | Q_PROPERTY macros, PlaybackMode enum, positionMs/durationMs, track metadata |
| STATE-03 | UIState as thin Q_PROPERTY bag exposing overlay visibility, active view, error states | ActiveView enum, boolean overlay properties, toast state, error properties |
| STATE-04 | MediaSource enum separates user-facing sources from receiver input hex codes | Q_ENUM, qmlRegisterUncreatableType, free function toHexCode/fromHexCode |
| STATE-05 | All state objects registered as QML singletons via qmlRegisterSingletonInstance() | qmlRegisterSingletonInstance in main.cpp before engine.load() |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6 Core | 6.8.2 | Q_PROPERTY, Q_OBJECT, Q_ENUM, signals/slots | Already in project, required for QML binding |
| Qt6 Qml | 6.8.2 | qmlRegisterSingletonInstance, qmlRegisterUncreatableType | Standard Qt6 QML registration API |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Qt6 Test | 6.8.2 | QSignalSpy for verifying signal emission | Already in test dependencies |
| Google Test | 1.17.0 | Test framework | Already in project |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| qmlRegisterSingletonInstance | QML_SINGLETON macro (declarative) | Declarative approach requires QML module integration changes; instance-based is simpler for objects created in C++ composition root |
| Q_PROPERTY macros | QML_PROPERTY (Qt 6.8 bindable) | Bindable properties are newer but Q_PROPERTY is the established pattern and matches Phase 1 style |

## Architecture Patterns

### Recommended Project Structure
```
src/
├── state/
│   ├── ReceiverState.h      # Q_PROPERTY bag for receiver
│   ├── ReceiverState.cpp     # Setter implementations with signal emission
│   ├── PlaybackState.h       # Q_PROPERTY bag for playback
│   ├── PlaybackState.cpp     # Setter implementations
│   ├── UIState.h             # Q_PROPERTY bag for UI
│   ├── UIState.cpp           # Setter implementations
│   ├── MediaSource.h         # MediaSource enum + conversion functions
│   ├── MediaSource.cpp       # toHexCode / fromHexCode implementations
│   ├── StreamingService.h    # StreamingService enum
│   ├── PlaybackMode.h        # PlaybackMode enum
│   └── ActiveView.h          # ActiveView enum
├── app/
│   ├── AppBuilder.h          # (modified) creates state objects
│   ├── AppBuilder.cpp         # (modified) wires state into AppContext
│   └── AppContext.h           # (modified) adds state object pointers
└── main.cpp                   # (modified) QML singleton registration
```

### Pattern 1: Q_PROPERTY Bag (Thin State Object)

**What:** A QObject subclass that is purely a collection of Q_PROPERTY declarations with getters, setters, and change signals. No business logic.

**When to use:** When C++ state needs to be reactively bound to QML UI.

**Example:**
```cpp
// ReceiverState.h
class ReceiverState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

public:
    explicit ReceiverState(QObject* parent = nullptr);

    int volume() const { return m_volume; }

public slots:
    void setVolume(int volume)
    {
        if (m_volume == volume)
            return;
        m_volume = volume;
        emit volumeChanged(m_volume);
    }

signals:
    void volumeChanged(int volume);

private:
    int m_volume = 0;
};
```

**Key rules:**
- Setter guard: always check if value changed before emitting signal (prevents infinite loops in QML bindings)
- Inline getters in header (trivial accessors)
- Setters can be inline for simple types, or in .cpp for complex logic
- NOTIFY signal name must match convention: `{propertyName}Changed`

### Pattern 2: Q_ENUM for QML-Accessible Enums

**What:** Registering C++ enums so QML can reference them by name (e.g., `MediaSource.CD`).

**When to use:** When QML needs to compare or switch on enum values.

**Example:**
```cpp
// MediaSource.h
#pragma once
#include <QObject>
#include <cstdint>

class MediaSourceEnum : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        None = 0,
        Streaming,
        Phono,
        CD,
        Computer,
        Bluetooth,
        Library
    };
    Q_ENUM(Value)
};

using MediaSource = MediaSourceEnum::Value;

// Free functions for hex code conversion
uint8_t toHexCode(MediaSource source);
MediaSource fromHexCode(uint8_t code);
```

**Registration in main.cpp:**
```cpp
qmlRegisterUncreatableType<MediaSourceEnum>("MediaConsole", 1, 0, "MediaSource",
    "MediaSource is an enum type");
```

**QML usage:**
```qml
if (ReceiverState.currentInput === MediaSource.CD) { ... }
```

### Pattern 3: QML Singleton Registration

**What:** Making a C++ object instance globally accessible in QML without import.

**When to use:** For application-wide state objects.

**Example:**
```cpp
// main.cpp - BEFORE engine.load()
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "ReceiverState", receiverState);
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "PlaybackState", playbackState);
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "UIState", uiState);
```

**QML usage:**
```qml
import MediaConsole 1.0

Text {
    text: ReceiverState.volume
}
```

**Critical:** Registration MUST happen before `engine.load()`. Objects must remain alive for the lifetime of the QML engine. AppBuilder owns via unique_ptr, AppContext holds non-owning pointers, main.cpp registers the pointers.

### Anti-Patterns to Avoid
- **Business logic in state objects:** State objects are property bags ONLY. No timers, no network calls, no command sending. Controllers in later phases will call setters.
- **Grouped property structs:** CONTEXT.md explicitly says individual Q_PROPERTYs per metadata field, not grouped structs.
- **Missing setter guards:** Every setter MUST check `if (m_value == value) return;` before emitting. Without this, QML binding loops can freeze the UI.
- **Late registration:** Calling qmlRegisterSingletonInstance after engine.load() causes undefined behavior. Always register before loading QML.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| QML binding | Custom signal/slot wiring | Q_PROPERTY + NOTIFY | Qt's property binding system handles all the reactive update plumbing |
| Enum in QML | String-based enum comparison | Q_ENUM + qmlRegisterUncreatableType | Type-safe, auto-complete in IDE, no string typos |
| Singleton access | Context property injection | qmlRegisterSingletonInstance | Type-safe, works across all QML files without import chains |
| Signal verification in tests | Manual signal counting | QSignalSpy | Qt's built-in test utility, handles async signal delivery |

**Key insight:** Qt6's property system IS the reactive state management framework. Don't layer anything on top of it for this phase.

## Common Pitfalls

### Pitfall 1: Forgetting Setter Guard
**What goes wrong:** Setting a property emits a signal, which triggers a QML binding, which sets the same property again, causing an infinite loop.
**Why it happens:** Copy-paste error or oversight in setter implementation.
**How to avoid:** Every setter MUST have `if (m_value == newValue) return;` as its first line.
**Warning signs:** UI freezes when a property is first set from C++.

### Pitfall 2: Q_ENUM Scoping
**What goes wrong:** Q_ENUM must be declared inside a Q_OBJECT or Q_GADGET class. A standalone enum cannot be registered.
**Why it happens:** Developers try to use Q_ENUM on a namespace-scoped enum.
**How to avoid:** Host each enum in a minimal QObject class (e.g., `MediaSourceEnum`) or use Q_GADGET for lightweight hosting. Use a type alias (`using MediaSource = MediaSourceEnum::Value;`) for clean usage in C++ code.
**Warning signs:** moc errors about Q_ENUM outside Q_OBJECT scope.

### Pitfall 3: Singleton Lifetime
**What goes wrong:** QML engine accesses a singleton after its C++ object has been destroyed, causing a crash.
**Why it happens:** Object ownership doesn't match engine lifetime.
**How to avoid:** AppBuilder owns state objects via unique_ptr, AppBuilder lives as long as the application (parented to QGuiApplication or lives in main scope). The QML engine must be destroyed before AppBuilder.
**Warning signs:** Crash on application exit, particularly on macOS.

### Pitfall 4: Missing AUTOMOC
**What goes wrong:** Q_OBJECT macro present but moc doesn't process the file, leading to linker errors ("undefined reference to vtable").
**Why it happens:** New .h files with Q_OBJECT aren't in CMake source lists.
**How to avoid:** Add all new .h AND .cpp files to LIB_SOURCES in CMakeLists.txt. CMAKE_AUTOMOC is already enabled.
**Warning signs:** Linker errors mentioning vtable for the new state class.

### Pitfall 5: QString Comparison in Setters
**What goes wrong:** QString comparison can be expensive for long strings; but more importantly, empty QString vs null QString can differ.
**Why it happens:** Default-constructed QString is null, but `""` is empty (not null). `QString() != QString("")` is true prior to Qt 6.7.
**How to avoid:** Use `QString::isEmpty()` checks where needed, but for property setters the `==` operator works correctly for value semantics. Just be aware that initial empty vs null distinction exists.
**Warning signs:** Properties appearing to change when they shouldn't.

## Code Examples

### Complete Q_PROPERTY Pattern (ReceiverState excerpt)
```cpp
// ReceiverState.h
#pragma once

#include <QObject>
#include <QString>

#include "state/MediaSource.h"
#include "state/StreamingService.h"

class ReceiverState : public QObject
{
    Q_OBJECT

    // Core receiver state
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY poweredChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(MediaSourceEnum::Value currentInput READ currentInput
                   WRITE setCurrentInput NOTIFY currentInputChanged)

    // Streaming metadata
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    // ... etc

public:
    explicit ReceiverState(QObject* parent = nullptr);

    int volume() const { return m_volume; }
    bool powered() const { return m_powered; }
    bool muted() const { return m_muted; }
    MediaSource currentInput() const { return m_currentInput; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }

public slots:
    void setVolume(int volume);
    void setPowered(bool powered);
    void setMuted(bool muted);
    void setCurrentInput(MediaSource input);
    void setTitle(const QString& title);
    void setArtist(const QString& artist);

signals:
    void volumeChanged(int volume);
    void poweredChanged(bool powered);
    void mutedChanged(bool muted);
    void currentInputChanged(MediaSource input);
    void titleChanged(const QString& title);
    void artistChanged(const QString& artist);

private:
    int m_volume = 0;
    bool m_powered = false;
    bool m_muted = false;
    MediaSource m_currentInput = MediaSource::None;
    QString m_title;
    QString m_artist;
};
```

### QSignalSpy Test Pattern
```cpp
#include <QSignalSpy>
#include <gtest/gtest.h>

#include "state/ReceiverState.h"

TEST(ReceiverStateTest, VolumeEmitsChanged)
{
    ReceiverState state;
    QSignalSpy spy(&state, &ReceiverState::volumeChanged);

    state.setVolume(42);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 42);
}

TEST(ReceiverStateTest, VolumeDoesNotEmitWhenUnchanged)
{
    ReceiverState state;
    state.setVolume(42);

    QSignalSpy spy(&state, &ReceiverState::volumeChanged);
    state.setVolume(42);  // Same value

    EXPECT_EQ(spy.count(), 0);  // No signal
}
```

### Enum Registration Pattern
```cpp
// main.cpp (additions)
#include "state/MediaSource.h"
#include "state/ReceiverState.h"
#include "state/PlaybackState.h"
#include "state/UIState.h"

// After builder.build(config):
auto* receiverState = ctx.receiverState;
auto* playbackState = ctx.playbackState;
auto* uiState = ctx.uiState;

// Register enums (before engine.load)
qmlRegisterUncreatableType<MediaSourceEnum>("MediaConsole", 1, 0, "MediaSource",
    "MediaSource is an enum type");
qmlRegisterUncreatableType<PlaybackModeEnum>("MediaConsole", 1, 0, "PlaybackMode",
    "PlaybackMode is an enum type");
qmlRegisterUncreatableType<ActiveViewEnum>("MediaConsole", 1, 0, "ActiveView",
    "ActiveView is an enum type");

// Register singletons (before engine.load)
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "ReceiverState", receiverState);
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "PlaybackState", playbackState);
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "UIState", uiState);
```

### Hex Code Conversion
```cpp
// MediaSource.cpp
#include "MediaSource.h"

uint8_t toHexCode(MediaSource source)
{
    switch (source)
    {
        case MediaSource::Streaming:  return 0x2B;
        case MediaSource::Phono:      return 0x22;
        case MediaSource::CD:         return 0x23;
        case MediaSource::Computer:   return 0x05;
        case MediaSource::Bluetooth:  return 0x2E;
        case MediaSource::Library:    return 0x23;  // Same as CD (uses CD input)
        case MediaSource::None:
        default:                      return 0x00;
    }
}

MediaSource fromHexCode(uint8_t code)
{
    switch (code)
    {
        case 0x2B: return MediaSource::Streaming;
        case 0x22: return MediaSource::Phono;
        case 0x23: return MediaSource::CD;        // Note: Library also uses 0x23
        case 0x05: return MediaSource::Computer;
        case 0x2E: return MediaSource::Bluetooth;
        default:   return MediaSource::None;
    }
}
```

**Note on hex codes:** The REQUIREMENTS.md lists CD as 0x02 and Library as 0x02, but the Onkyo eISCP protocol uses different codes. The exact hex values should be verified against the Onkyo TX-8260 eISCP documentation in Phase 3. For this phase, the mapping structure is what matters — values can be corrected when the receiver controller is implemented.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| qmlRegisterType (Qt5) | qmlRegisterSingletonInstance (Qt6) | Qt 6.0 | Simpler API, no factory function needed |
| Q_PROPERTY with manual moc | CMAKE_AUTOMOC | CMake 3.16+ | Automatic moc invocation, no manual rules |
| Context properties (setContextProperty) | qmlRegisterSingletonInstance | Qt 6.0+ | Type-safe, better tooling support, works with Qt Creator |

**Deprecated/outdated:**
- `qmlRegisterType` for singletons: Use `qmlRegisterSingletonInstance` instead
- `setContextProperty`: Deprecated in Qt6, no type checking, no auto-complete in IDEs

## Open Questions

1. **CD hex code (0x02 vs 0x23)**
   - What we know: REQUIREMENTS.md says CD=02, but this may be decimal, not hex
   - What's unclear: Exact eISCP hex codes for the TX-8260
   - Recommendation: Use placeholder values in this phase, correct in Phase 3 when receiver controller is built. The conversion function structure is what matters.

2. **StreamingService enum hosting**
   - What we know: CONTEXT.md leaves this to Claude's discretion (Q_GADGET vs helper class)
   - What's unclear: Whether StreamingService needs QML access in this phase
   - Recommendation: Use a minimal QObject class with Q_ENUM (same pattern as MediaSource) for consistency. If QML doesn't need it yet, the registration can be deferred, but having the enum class ready avoids rework.

## Sources

### Primary (HIGH confidence)
- Qt6 6.8 Q_PROPERTY documentation — property system, NOTIFY signals
- Qt6 6.8 QML C++ integration — qmlRegisterSingletonInstance, qmlRegisterUncreatableType
- Existing codebase (AppBuilder, AppContext, CMakeLists.txt) — established patterns

### Secondary (MEDIUM confidence)
- Qt6 Q_ENUM documentation — enum registration and QML access patterns

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Qt6 property system is core Qt, well-documented, already in project
- Architecture: HIGH - Follows established AppBuilder/AppContext pattern from Phase 1
- Pitfalls: HIGH - Common Qt property pitfalls are well-known and documented

**Research date:** 2026-02-28
**Valid until:** 2027-02-28 (stable Qt6 API, unlikely to change)
