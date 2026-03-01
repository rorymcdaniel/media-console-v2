# Phase 11: Integration Wiring Fixes - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Close 5 cross-phase integration gaps that leave built features unreachable from the UI or disconnected from their error paths. All underlying controllers and signals exist — the work is wiring them correctly (Q_INVOKABLE, AppBuilder connections, QML singleton registration, PlaybackRouter injection, UI track lists). No new controllers or business logic.

</domain>

<decisions>
## Implementation Decisions

### Bug 1 — FlacLibraryController.playTrack() not Q_INVOKABLE
- Add `Q_INVOKABLE` to `FlacLibraryController::playTrack(int trackModelIndex)`
- QML already calls `FlacLibraryController.playTrack(index)` in LibraryBrowser.qml:525 — this is the only change needed

### Bug 2 — Audio error dialog never fires
- Connect `LocalPlaybackController::audioRecoveryFailed` → `UIState::setAudioError` in AppBuilder (lambda to inject the message string)
- Error message: **"Audio Error encountered, please restart the application"**
- Dialog already exists in main.qml; the Restart button should call a **pluggable action** — initially `QCoreApplication::quit()` so systemd restarts the process
- Architecture: make the restart action replaceable without changing QML (e.g., a signal on UIState or a dedicated slot in main.cpp wired at startup) so the action can later be changed to `systemctl reboot` without QML changes
- Before exiting, log **full diagnostic context**: error timestamp, active source, playback mode, recovery retry count, ALSA device name — enough to diagnose the root cause post-mortem
- The dialog clears when dismissed (existing `UIState.setAudioError("")` calls in main.qml handle this)

### Bug 3 — CdController not accessible from QML
- Register `CdController` as a QML singleton in main.cpp: `qmlRegisterSingletonInstance("MediaConsole", 1, 0, "CdController", ctx.cdController)`
- Wire the eject button handler in main.qml: `onClicked: CdController.eject()`
- Add `Q_INVOKABLE` to `CdController::eject()` and `CdController::playTrack(int)` so QML can call them

#### CD track list in NowPlaying (FLAC-08 / UI-09)
- When `PlaybackState.activeSource === MediaSource.CD`, NowPlaying shows a **scrollable track list** below the album art
- Track list is **only visible when CD is not currently playing** (i.e., `PlaybackState.playbackMode !== PlaybackMode.Playing`)
  - When the user taps a track → `CdController.playTrack(trackNumber)` → playback starts → track list hides, standard now-playing view (progress bar, controls) appears
- Track list shows: track number, track duration from TOC
- Metadata (title) shows if available from CdController, falls back to "Track N" if not yet loaded
- Source of track data: `CdController.currentToc()` — this is a C++ QVector, needs to be exposed for QML binding (read-only Q_PROPERTY returning the TOC list, or a signal that QML Connections listens to)

#### Eject button confirmation dialog
- Eject button in main.qml is already visible only when `PlaybackState.activeSource === MediaSource.CD`
- If `PlaybackState.playbackMode === PlaybackMode.Playing` when eject is tapped: show a **confirmation modal** — "Playback in progress. Eject the disc?" with **Yes** and **No** buttons
  - **No**: dismiss modal, playback continues
  - **Yes**: call `CdController.eject()` immediately (CdController internally stops playback before ejecting — the most efficient sequence since the controller manages its own state)
- If NOT playing: eject immediately without confirmation

### Bug 4 — PlaybackRouter.seek() is a no-op for CD and Library
- Add `LocalPlaybackController*` parameter to `PlaybackRouter` constructor
- In `seek(int ms)`: for `MediaSource::CD` and `MediaSource::Library`, call `m_localPlaybackController->seek(ms)`
- Update AppBuilder to pass `m_localPlaybackController.get()` when constructing PlaybackRouter

### Bug 5 — ReceiverController volume bypasses VolumeGestureController
- In `ReceiverController::parseResponse()`, the MVL case currently calls `m_receiverState->setVolume(vol)` directly
- Fix: ReceiverController should **emit a signal** `volumeReceivedFromReceiver(int volume)` instead of calling receiverState directly
- AppBuilder wires: `ReceiverController::volumeReceivedFromReceiver` → `VolumeGestureController::onExternalVolumeUpdate`
- VolumeGestureController already handles the logic: suppresses update during active gesture (gesture has priority), applies update silently after gesture ends (reconciliation snap)
- **No overlay** for receiver-side volume changes — overlay is only for user-initiated input (encoder tick or touchscreen slider)
- VolumeGestureController then calls `m_receiverState->setVolume()` itself after the gesture check

### Claude's Discretion
- TOC exposure approach for QML (Q_PROPERTY vs Connections signal vs a simple QML-accessible model)
- Exact log format for audio diagnostic output
- Restart action abstraction mechanism (signal, virtual method, or std::function)
- Order of stop/eject operations inside CdController.eject() (controller owns this detail)
- Whether the CD track list uses a Repeater or ListView given it's a short list (max ~20 tracks)

</decisions>

<specifics>
## Specific Ideas

- Restart button architecture should be "pluggable" — avoid hardcoding `QCoreApplication::quit()` in a QML signal. Instead, connect a C++ slot at startup (in main.cpp) so the action can be swapped to `systemctl reboot` later without touching QML.
- Diagnostic logging before exit should be rich enough to diagnose ALSA issues post-mortem: timestamp, source, playback mode, ALSA device name, recovery retry count.
- Eject confirmation dialog: "Playback in progress. Eject the disc?" — yes/no. Call CdController.eject() on yes (CdController handles stop internally for efficiency).
- CD track list hides when playback starts — user should see the standard NowPlaying view (progress bar, seek, controls) during active CD playback, not the track selection list.

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `AudioErrorDialog` in main.qml:547 — already wired to `UIState.audioError !== ""`, already has dismiss button calling `UIState.setAudioError("")`. Just needs the Restart Now button wired to an action.
- `CdController::eject()` and `CdController::playTrack(int)` — methods exist, just need `Q_INVOKABLE` and QML singleton registration
- `VolumeGestureController::onExternalVolumeUpdate(int)` — logic already correct, just not being called
- `LocalPlaybackController::seek(qint64 positionMs)` — exists, PlaybackRouter just needs a reference to call it
- Eject button in main.qml:97-103 — already positioned and visible, handler is empty comment

### Established Patterns
- QML singleton registration: `qmlRegisterSingletonInstance("MediaConsole", 1, 0, "Name", ptr)` in main.cpp
- Q_INVOKABLE on controller methods called from QML: PlaybackRouter already has all methods Q_INVOKABLE
- AppBuilder connect() pattern: all cross-component wiring lives in AppBuilder::build()
- `#ifdef HAS_SNDFILE` guards around FlacLibraryController usage in PlaybackRouter.cpp and AppBuilder.cpp

### Integration Points
- `AppBuilder::build()` — add 2 new connect() calls: audioRecoveryFailed and volumeReceivedFromReceiver
- `PlaybackRouter` constructor — add `LocalPlaybackController*` parameter, update AppBuilder construction call
- `main.cpp` — add CdController singleton registration, wire restart action signal
- `ReceiverController` — emit signal instead of direct receiverState call for volume
- `NowPlaying.qml` — add conditional CD track list section

</code_context>

<deferred>
## Deferred Ideas

- Reboot system instead of clean exit on audio recovery failure — explicitly deferred pending diagnostic data
- Spotify seek implementation — noted as "not implemented in Phase 8" in PlaybackRouter.cpp, remains deferred
- CD auto-play on insert — out of scope per PROJECT.md constraints

</deferred>

---

*Phase: 11-integration-wiring-fixes*
*Context gathered: 2026-02-28*
