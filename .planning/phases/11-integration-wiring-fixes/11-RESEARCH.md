# Phase 11: Integration Wiring Fixes - Research

**Researched:** 2026-02-28
**Domain:** Qt6/QML C++ integration — Q_INVOKABLE, QML singleton registration, signal/slot wiring
**Confidence:** HIGH

## Summary

Phase 11 closes 5 specific integration gaps where components were built but not connected. All
underlying controllers, signals, and QML screens exist — the gaps are wiring deficiencies. No
new technology is needed; the fix patterns are identical to those already in use in the codebase.

The research task was to read actual source files and confirm the exact state of each bug, the
correct fix, and the constraints on each change. Every finding below is based on direct code
inspection of the live codebase, not documentation assumptions.

**Primary recommendation:** Treat each bug as a targeted 1–3 line C++ or QML edit. Group into
two plans: (1) C++ backend wiring — Q_INVOKABLE annotations, signal additions, AppBuilder
connect() calls, PlaybackRouter constructor change; (2) QML frontend wiring — eject handler,
eject confirmation modal, CdController QML singleton, CD track list in NowPlaying.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Bug 1 — FlacLibraryController.playTrack() not Q_INVOKABLE**
- Add `Q_INVOKABLE` to `FlacLibraryController::playTrack(int trackModelIndex)`
- QML already calls `FlacLibraryController.playTrack(index)` in LibraryBrowser.qml:525 — this is the only change needed

**Bug 2 — Audio error dialog never fires**
- Connect `LocalPlaybackController::audioRecoveryFailed` → `UIState::setAudioError` in AppBuilder (lambda to inject the message string)
- Error message: "Audio Error encountered, please restart the application"
- Dialog already exists in main.qml; the Restart button should call a **pluggable action** — initially `QCoreApplication::quit()` so systemd restarts the process
- Architecture: make the restart action replaceable without changing QML (e.g., a signal on UIState or a dedicated slot in main.cpp wired at startup) so the action can later be changed to `systemctl reboot` without QML changes
- Before exiting, log **full diagnostic context**: error timestamp, active source, playback mode, recovery retry count, ALSA device name — enough to diagnose the root cause post-mortem
- The dialog clears when dismissed (existing `UIState.setAudioError("")` calls in main.qml handle this)

**Bug 3 — CdController not accessible from QML**
- Register `CdController` as a QML singleton in main.cpp: `qmlRegisterSingletonInstance("MediaConsole", 1, 0, "CdController", ctx.cdController)`
- Wire the eject button handler in main.qml: `onClicked: CdController.eject()`
- Add `Q_INVOKABLE` to `CdController::eject()` and `CdController::playTrack(int)` so QML can call them

**CD track list in NowPlaying (FLAC-08 / UI-09)**
- When `PlaybackState.activeSource === MediaSource.CD`, NowPlaying shows a scrollable track list below the album art
- Track list is only visible when CD is not currently playing (i.e., `PlaybackState.playbackMode !== PlaybackMode.Playing`)
- When the user taps a track → `CdController.playTrack(trackNumber)` → playback starts → track list hides
- Track list shows: track number, track duration from TOC
- Metadata (title) shows if available from CdController, falls back to "Track N" if not yet loaded
- Source of track data: `CdController.currentToc()` — needs to be exposed for QML binding (read-only Q_PROPERTY or tocReady signal)

**Eject button confirmation dialog**
- Eject button in main.qml is visible only when `PlaybackState.activeSource === MediaSource.CD`
- If `PlaybackState.playbackMode === PlaybackMode.Playing` when eject is tapped: show confirmation modal "Playback in progress. Eject the disc?" with Yes and No buttons
- No: dismiss modal, playback continues
- Yes: call `CdController.eject()` immediately
- If NOT playing: eject immediately without confirmation

**Bug 4 — PlaybackRouter.seek() is a no-op for CD and Library**
- Add `LocalPlaybackController*` parameter to `PlaybackRouter` constructor
- In `seek(int ms)`: for `MediaSource::CD` and `MediaSource::Library`, call `m_localPlaybackController->seek(ms)`
- Update AppBuilder to pass `m_localPlaybackController.get()` when constructing PlaybackRouter

**Bug 5 — ReceiverController volume bypasses VolumeGestureController**
- In `ReceiverController::parseResponse()`, the MVL case currently calls `m_receiverState->setVolume(vol)` directly
- Fix: ReceiverController should emit a signal `volumeReceivedFromReceiver(int volume)` instead
- AppBuilder wires: `ReceiverController::volumeReceivedFromReceiver` → `VolumeGestureController::onExternalVolumeUpdate`
- No overlay for receiver-side volume changes — overlay is only for user-initiated input

### Claude's Discretion
- TOC exposure approach for QML (Q_PROPERTY vs Connections signal vs a simple QML-accessible model)
- Exact log format for audio diagnostic output
- Restart action abstraction mechanism (signal, virtual method, or std::function)
- Order of stop/eject operations inside CdController.eject() (controller owns this detail)
- Whether the CD track list uses a Repeater or ListView given it's a short list (max ~20 tracks)

### Deferred Ideas (OUT OF SCOPE)
- Reboot system instead of clean exit on audio recovery failure
- Spotify seek implementation
- CD auto-play on insert

</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| AUDIO-06 | audioRecoveryFailed signal connected to UIState.setAudioError in AppBuilder — AudioErrorDialog triggers on ALSA recovery failure | LocalPlaybackController already emits audioRecoveryFailed (line 57 of .h); UIState.setAudioError(QString) slot exists; AppBuilder missing one connect() call |
| AUDIO-07 | Restart button in AudioErrorDialog calls pluggable action (initially QCoreApplication::quit()) | Dialog exists at main.qml:545; "Retry" button currently calls PlaybackRouter.play() — needs replacement with pluggable restart action via UIState signal or main.cpp wiring |
| FLAC-08 | FlacLibraryController.playTrack() is Q_INVOKABLE and QML library playback works end-to-end | FlacLibraryController::playTrack(int trackModelIndex) exists, lacks Q_INVOKABLE; QML at LibraryBrowser.qml:525 already calls it; one keyword addition fixes it |
| UI-09 | CD track list shown in NowPlaying when CD active and not playing | NowPlaying.qml has no CD-conditional track list section; CdController.currentToc() returns QVector<TocEntry> but is not Q_PROPERTY-exposed; TocEntry has trackNumber, durationSeconds |
| UI-13 | CdController registered as QML singleton, eject button wired, CD track selection works | CdController not registered in main.cpp (only PlaybackRouter, AlbumArtResolver, SpotifyController, ReceiverController are); eject handler in main.qml:100-102 is empty comment |
| ORCH-03 | PlaybackRouter.seek() routes to LocalPlaybackController for CD and Library | seek() in PlaybackRouter.cpp:212 is a no-op for both sources with comments "handled internally" — no actual call; PlaybackRouter.h constructor has no LocalPlaybackController parameter |

</phase_requirements>

## Standard Stack

### Core — Qt6 Patterns Used in This Codebase

| Pattern | Where Used | Notes |
|---------|-----------|-------|
| `Q_INVOKABLE` | PlaybackRouter (all methods), CdController (none yet) | Marks C++ methods callable from QML directly |
| `qmlRegisterSingletonInstance()` | main.cpp:103-118 | Registers C++ objects as named QML singletons |
| `QObject::connect()` in AppBuilder::build() | AppBuilder.cpp:66-85 | All cross-component wiring lives here |
| `Q_PROPERTY` with NOTIFY signal | UIState, ReceiverState, PlaybackState | QML property binding pattern |
| `emit signal` in C++ → QML via Connections | Established pattern | For event-driven UI updates |
| `#ifdef HAS_SNDFILE` guard | AppBuilder.cpp, PlaybackRouter.cpp | All FlacLibraryController usage gated |

### Key Verified Facts from Code Inspection

**Bug 1 (FLAC-08):**
- `FlacLibraryController::playTrack(int trackModelIndex)` exists in FlacLibraryController.h:34
- It is NOT marked Q_INVOKABLE — missing keyword only
- `FlacLibraryController` IS registered as QML singleton in main.cpp:115 (`#ifdef HAS_SNDFILE` block)
- LibraryBrowser.qml:525 already calls `FlacLibraryController.playTrack(index)` — QML call exists but currently silently fails at runtime (no Q_INVOKABLE = method not visible to QML meta-object system)
- Also: `next()` and `previous()` should get Q_INVOKABLE if they'll be called from QML

**Bug 2 (AUDIO-06 / AUDIO-07):**
- `LocalPlaybackController::audioRecoveryFailed()` signal is declared in LocalPlaybackController.h:57
- `UIState::setAudioError(const QString&)` slot exists in UIState.h:47
- AppBuilder.cpp has NO connect() call for audioRecoveryFailed → missing entirely
- AudioErrorDialog in main.qml:545-641 is complete: shows UIState.audioError text, Dismiss clears it, "Retry" button calls PlaybackRouter.play() — but "Retry" is wrong label/behavior; CONTEXT.md says "Restart Now" button should call QCoreApplication::quit()
- Current dialog has "Dismiss" + "Retry" — needs to become "Dismiss" + "Restart Now"
- QML cannot call QCoreApplication::quit() directly; must be exposed via C++ signal/slot or Connections to a UIState signal

**Bug 3 (UI-13):**
- `CdController::eject()` and `CdController::playTrack(int)` exist in CdController.h:37-38
- Neither is Q_INVOKABLE — both need the annotation
- main.cpp does NOT have `qmlRegisterSingletonInstance("MediaConsole", 1, 0, "CdController", ctx.cdController)` — missing from the registration block
- AppContext.h:43 has `CdController* cdController = nullptr;` — pointer available
- main.qml:97-103 eject button: handler at onClicked is empty comment "CD eject — CdController.eject() if available"
- Eject confirmation modal not present in main.qml — needs to be added
- TOC exposure: CdController has `tocReady(const QVector<TocEntry>& toc)` signal and `currentToc()` method; TocEntry is not registered with Qt meta-object system, so QML cannot iterate a QVector<TocEntry> directly

**TocEntry QML exposure issue:** TocEntry is a plain struct in ICdDrive.h. For QML to iterate it, the cleanest option matching CONTEXT.md's "Claude's Discretion" is either:
  - Option A: Add a `Q_PROPERTY(QVariantList toc READ tocAsVariantList NOTIFY tocReady)` to CdController — converts each TocEntry to a QVariantMap at read time. Simple, no new model class.
  - Option B: Expose via Connections in QML listening to `tocReady` signal, store as JS array in QML property — no C++ changes needed for TOC but requires QML to handle the QVector<TocEntry> as opaque until converted. This is risky because QVector<TocEntry> is not auto-converted to JS array.
  - **Recommendation: Option A** — add `Q_PROPERTY(QVariantList toc READ tocAsVariantList NOTIFY tocReady)` with a helper that maps each TocEntry to `{trackNumber, durationSeconds, title}`. This is minimal C++ addition, QML-friendly, and follows existing Q_PROPERTY patterns in the codebase.
  - Track metadata (title): CdController has `currentMetadata()` returning `CdMetadata` which contains `QVector<CdTrackInfo>`. Need to check CdMetadata structure for per-track title data.

**Bug 4 (ORCH-03):**
- PlaybackRouter constructor (PlaybackRouter.h:22-24): no `LocalPlaybackController*` parameter
- PlaybackRouter::seek() (PlaybackRouter.cpp:212-237): CD and Library cases have no-op comments, no actual seek call
- AppBuilder.cpp:152-159 constructs PlaybackRouter without LocalPlaybackController
- Fix is: add parameter to constructor, store as member, update seek() cases, update AppBuilder construction call
- Must add `LocalPlaybackController*` member and forward-declare/include header

**Bug 5 (ORCH-03 / receiver volume):**
- ReceiverController.cpp:199-204 MVL case: directly calls `m_receiverState->setVolume(vol)` with no intermediary
- VolumeGestureController::onExternalVolumeUpdate(int) exists in VolumeGestureController.h:23 — correct target
- ReceiverController.h signals section (line 44-47): only `staleDataDetected(bool)` signal — `volumeReceivedFromReceiver` does not exist yet
- AppBuilder.cpp already wires volumeGestureController to receiverController for gestureEnded → setVolume (line 66-67); the reverse path (receiver → gesture controller) is missing

## Architecture Patterns

### Pattern 1: Q_INVOKABLE Annotation
**What:** Add `Q_INVOKABLE` keyword before return type in class declaration
**When to use:** Any C++ method that QML calls directly by name on a registered object
**Example (from PlaybackRouter.h which is correct):**
```cpp
Q_INVOKABLE void play();
Q_INVOKABLE void pause();
Q_INVOKABLE void seek(int ms);
```
**Apply to:**
- `FlacLibraryController::playTrack(int trackModelIndex)`
- `FlacLibraryController::next()` and `previous()` (if called from QML)
- `CdController::eject()`
- `CdController::playTrack(int trackNumber)`

### Pattern 2: QML Singleton Registration
**What:** `qmlRegisterSingletonInstance()` in main.cpp before `engine.load()`
**Example (from main.cpp:108-112, existing registrations):**
```cpp
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "PlaybackRouter", ctx.playbackRouter);
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "CdController", ctx.cdController);
```

### Pattern 3: AppBuilder Signal Wiring
**What:** `connect()` calls in AppBuilder::build()
**Example (existing pattern, line 66-67):**
```cpp
connect(m_volumeGestureController.get(), &VolumeGestureController::gestureEnded,
        m_receiverController.get(), &ReceiverController::setVolume);
```
**New connections needed:**
```cpp
// Bug 2: audioRecoveryFailed → UIState.setAudioError
connect(m_localPlaybackController.get(), &LocalPlaybackController::audioRecoveryFailed,
        m_uiState.get(), [this]() {
            m_uiState->setAudioError("Audio Error encountered, please restart the application");
        });

// Bug 5: volumeReceivedFromReceiver → VolumeGestureController.onExternalVolumeUpdate
connect(m_receiverController.get(), &ReceiverController::volumeReceivedFromReceiver,
        m_volumeGestureController.get(), &VolumeGestureController::onExternalVolumeUpdate);
```

### Pattern 4: PlaybackRouter Constructor Extension
**What:** Add LocalPlaybackController* parameter to PlaybackRouter
**Follow existing pattern:** All other controller pointers in PlaybackRouter are stored as private members, initialized in constructor initializer list, and used in switch cases.

### Pattern 5: QVariantList Q_PROPERTY for QML-accessible data
**What:** Convert C++ container to QVariantList (list of QVariantMaps) for QML iteration
**Why:** QML can natively iterate QVariantList; QVector<TocEntry> cannot be iterated in QML without meta-type registration
**Example approach:**
```cpp
Q_PROPERTY(QVariantList toc READ tocAsVariantList NOTIFY tocReady)

QVariantList CdController::tocAsVariantList() const {
    QVariantList result;
    for (const TocEntry& e : m_currentToc) {
        QVariantMap entry;
        entry["trackNumber"] = e.trackNumber;
        entry["durationSeconds"] = e.durationSeconds;
        entry["title"] = ""; // filled from currentMetadata() if available
        result.append(entry);
    }
    return result;
}
```

### Pattern 6: Pluggable Restart Action via UIState Signal
**What:** Add signal to UIState that QML triggers; wire to quit action in main.cpp
**Why:** Avoids hardcoding QCoreApplication::quit() in QML; makes action swappable later
**Implementation:**
1. Add `void restartRequested()` signal to UIState
2. In main.cpp, after build: `QObject::connect(ctx.uiState, &UIState::restartRequested, &app, [](){ QCoreApplication::quit(); })`
3. In main.qml Restart Now button: `onClicked: { UIState.setAudioError(""); UIState.restartRequested() }`

### Anti-Patterns to Avoid
- **Calling QCoreApplication::quit() from QML** — not accessible; breaks pluggability
- **Iterating QVector<TocEntry> in QML directly** — struct not registered with meta-object system
- **Direct ReceiverState.setVolume() from ReceiverController** — bypasses gesture suppression
- **Adding HAS_SNDFILE guard to PlaybackRouter constructor** — already partially guarded; LocalPlaybackController is always present regardless of HAS_SNDFILE, so the new parameter needs no guard

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| QML-callable C++ methods | Custom IPC/property hack | `Q_INVOKABLE` keyword | Built into Qt meta-object system |
| QML access to C++ objects | QML context properties (deprecated) | `qmlRegisterSingletonInstance()` | Type-safe, same pattern already used |
| QML iteration over C++ struct vector | Custom QAbstractListModel | QVariantList Q_PROPERTY | Sufficient for max ~20 tracks, follows existing ReceiverState pattern |
| Pluggable restart action | Hardcoded QCoreApplication::quit() in QML | UIState signal wired in main.cpp | Keeps action swappable without QML changes |

## Common Pitfalls

### Pitfall 1: Q_INVOKABLE Missing from .cpp Too
**What goes wrong:** Compiler sees declaration without Q_INVOKABLE but finds implementation; moc ignores non-Q_INVOKABLE methods
**How to avoid:** Only need Q_INVOKABLE in the .h class declaration, not in the .cpp definition. Correct.

### Pitfall 2: HAS_SNDFILE Guard Scope
**What goes wrong:** Adding LocalPlaybackController* to PlaybackRouter but forgetting the include guard in PlaybackRouter.cpp
**How to avoid:** LocalPlaybackController does not require HAS_SNDFILE. The `#ifdef HAS_SNDFILE` guard in PlaybackRouter.cpp guards only FlacLibraryController includes/calls — not LocalPlaybackController. Keep it that way.

### Pitfall 3: QML Singleton Not Registered Before Engine Load
**What goes wrong:** Adding `qmlRegisterSingletonInstance("MediaConsole", 1, 0, "CdController", ...)` after `engine.load()` causes QML type resolution failure
**How to avoid:** All registrations in main.cpp already precede `engine.load()` — add new registration in same block (lines 102-119).

### Pitfall 4: TocEntry Not Q_GADGET
**What goes wrong:** Trying to pass TocEntry directly to QML or using it as a Q_PROPERTY type without registering it
**How to avoid:** Use QVariantList/QVariantMap conversion approach instead of registering TocEntry as Q_GADGET (simpler, no moc work).

### Pitfall 5: ReceiverController Signal Emission in parseResponse
**What goes wrong:** Changing `m_receiverState->setVolume(vol)` to `emit volumeReceivedFromReceiver(vol)` means VolumeGestureController.onExternalVolumeUpdate() now updates ReceiverState — ensure VolumeGestureController always calls m_receiverState->setVolume() after gesture check (it does — confirmed in VolumeGestureController.cpp design).

### Pitfall 6: Eject Confirmation Modal QML State
**What goes wrong:** Using a separate QML state for the modal that conflicts with the main error dialog state
**How to avoid:** Use a simple boolean property `property bool ejectConfirmVisible: false` local to the main.qml item or to the eject button component. No need for UIState C++ property for this transient UI state.

### Pitfall 7: seek() ms vs qint64
**What goes wrong:** PlaybackRouter.seek(int ms) passes int; LocalPlaybackController::seek(qint64 positionMs) takes qint64
**How to avoid:** Cast explicitly: `m_localPlaybackController->seek(static_cast<qint64>(ms))`. No precision loss for typical track lengths.

## Code Examples

### Adding Q_INVOKABLE (FlacLibraryController.h)
```cpp
// Before:
void playTrack(int trackModelIndex);

// After:
Q_INVOKABLE void playTrack(int trackModelIndex);
```

### PlaybackRouter Constructor — Add LocalPlaybackController
```cpp
// PlaybackRouter.h — add member and update constructor signature
class LocalPlaybackController; // forward declare

PlaybackRouter(PlaybackState* playbackState, ReceiverController* receiverController,
               CdController* cdController, FlacLibraryController* flacLibraryController,
               SpotifyController* spotifyController,
               LocalPlaybackController* localPlaybackController,  // NEW
               QObject* parent = nullptr);

private:
    LocalPlaybackController* m_localPlaybackController; // NEW
```

```cpp
// PlaybackRouter.cpp — seek() implementation
void PlaybackRouter::seek(int ms)
{
    const auto source = m_playbackState->activeSource();
    switch (source) {
    case MediaSource::CD:
    case MediaSource::Library:
        if (m_localPlaybackController)
            m_localPlaybackController->seek(static_cast<qint64>(ms));
        break;
    // ... other cases unchanged
    }
}
```

### ReceiverController — Emit Signal Instead of Direct Call
```cpp
// ReceiverController.h — add to signals section:
signals:
    void staleDataDetected(bool stale);
    void volumeReceivedFromReceiver(int volume);  // NEW

// ReceiverController.cpp — in parseResponse(), MVL case:
if (cmd == "MVL") {
    int vol = hexToVolume(payload);
    if (vol >= 0) {
        emit volumeReceivedFromReceiver(vol);  // was: m_receiverState->setVolume(vol)
    }
}
```

### AppBuilder — New Connections
```cpp
// After existing connect() for gestureEnded:
connect(m_localPlaybackController.get(), &LocalPlaybackController::audioRecoveryFailed,
        m_uiState.get(), [this]() {
            m_uiState->setAudioError(
                QStringLiteral("Audio Error encountered, please restart the application"));
        });

connect(m_receiverController.get(), &ReceiverController::volumeReceivedFromReceiver,
        m_volumeGestureController.get(), &VolumeGestureController::onExternalVolumeUpdate);
```

### main.cpp — CdController Registration + Restart Action
```cpp
qmlRegisterSingletonInstance("MediaConsole", 1, 0, "CdController", ctx.cdController);

// Wire restart action (after build, before engine.load):
QObject::connect(ctx.uiState, &UIState::restartRequested, &app,
    []() {
        // Log diagnostic context before exit
        qCCritical(mediaApp) << "Restart requested from AudioErrorDialog — exiting for systemd restart";
        QCoreApplication::quit();
    });
```

### NowPlaying.qml — CD Track List
```qml
// Conditional CD track list — shown when CD source and not playing
Column {
    visible: PlaybackState.activeSource === MediaSource.CD
             && PlaybackState.playbackMode !== PlaybackMode.Playing
    width: parent.width
    spacing: Theme.spacingSmall

    Text {
        text: "Select a Track"
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeBody
    }

    Repeater {
        model: CdController.toc  // QVariantList Q_PROPERTY
        delegate: Rectangle {
            width: parent.width
            height: Theme.touchTargetSmall
            color: trackMa.pressed ? Theme.glassBg : "transparent"
            MouseArea {
                id: trackMa
                anchors.fill: parent
                onClicked: CdController.playTrack(modelData.trackNumber)
            }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingSmall
                Text { text: "Track " + modelData.trackNumber; color: Theme.textPrimary }
                Text {
                    text: modelData.title || ("Track " + modelData.trackNumber)
                    color: Theme.textSecondary
                }
                Text {
                    text: formatDuration(modelData.durationSeconds)
                    color: Theme.textDimmed
                }
            }
        }
    }
}
```

## Open Questions

1. **CdMetadata track title access**
   - What we know: `CdController::currentMetadata()` returns `CdMetadata` which has `QVector<CdTrackInfo>` per CONTEXT.md's reference to "metadata from CdController"
   - What's unclear: Exact field name on `CdTrackInfo` for per-track title — need to confirm it's `.title` in CdMetadataCache.h
   - Recommendation: Read CdMetadataCache.h during execution to confirm field names; if title is available, include in `tocAsVariantList()` via `currentMetadata().tracks[i].title` with fallback

2. **UIState restartRequested signal vs std::function approach**
   - What we know: CONTEXT.md says "pluggable action" with three possible mechanisms
   - What's unclear: Whether to use UIState signal, a dedicated slot, or std::function stored in UIState
   - Recommendation: UIState signal is simplest — one signal declaration, one connect() in main.cpp — no new object needed

## Sources

### Primary (HIGH confidence)
- Direct code inspection: `src/library/FlacLibraryController.h` — confirmed missing Q_INVOKABLE
- Direct code inspection: `src/orchestration/PlaybackRouter.h` and `.cpp` — confirmed seek() no-op
- Direct code inspection: `src/receiver/ReceiverController.cpp:199-204` — confirmed direct setVolume() call
- Direct code inspection: `src/main.cpp:102-119` — confirmed CdController not registered
- Direct code inspection: `src/qml/main.qml:97-103` — confirmed eject handler is empty
- Direct code inspection: `src/qml/main.qml:545-641` — confirmed AudioErrorDialog exists, Restart button is "Retry"
- Direct code inspection: `src/app/AppBuilder.cpp` — confirmed missing connect() calls
- Direct code inspection: `src/platform/ICdDrive.h` — confirmed TocEntry struct fields: trackNumber, startSector, endSector, durationSeconds
- Direct code inspection: `src/receiver/VolumeGestureController.h:23` — confirmed onExternalVolumeUpdate(int) exists

## Metadata

**Confidence breakdown:**
- All 5 bugs and their fixes: HIGH — confirmed by direct source inspection
- TocEntry QML exposure via QVariantList: HIGH — Qt6 pattern, no external docs needed
- Pluggable restart action via UIState signal: HIGH — standard Qt signal pattern
- CD track list QML implementation: HIGH — follows existing LibraryBrowser pattern
- Eject confirmation modal: HIGH — simple QML boolean property pattern

**Research date:** 2026-02-28
**Valid until:** Indefinite (no external dependencies — all codebase-internal)
