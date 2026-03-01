---
phase: 11-integration-wiring-fixes
plan: "02"
subsystem: [cd, state, qml-registration]
tags: [CdController, QVariantList, Q_PROPERTY, qmlRegisterSingletonInstance, UIState]
provides:
  - CdController accessible as "CdController" QML singleton
  - CdController.toc Q_PROPERTY for track list binding
  - CdController.eject() and playTrack() callable from QML
  - UIState.restartRequested() signal for pluggable quit action
  - main.cpp wires UIState::restartRequested to QCoreApplication::quit()
affects: [qml-layer, cd-playback, audio-error-dialog]
tech-stack:
  added: []
  patterns: [QVariantList for QML data, qmlRegisterSingletonInstance, progressive disclosure pattern]
key-files:
  created: []
  modified:
    - src/cd/CdController.h
    - src/cd/CdController.cpp
    - src/state/UIState.h
    - src/main.cpp
key-decisions:
  - "QVariantList chosen over QAbstractListModel for toc — max ~20 tracks, simpler, no new model class"
  - "tocReady re-emitted in onMetadataReady() so QML toc updates when track titles load from GnuDB"
  - "restartRequested is a plain signal — implementation (quit vs systemctl) lives in main.cpp, not QML"
  - "CdController registered BEFORE engine.load() per Qt6 singleton registration rules"
duration: 20min
completed: 2026-02-28
---

# Phase 11 Plan 02: CdController QML Exposure Summary

**Registered CdController as QML singleton with toc property, added UIState restart signal wired to quit.**

## Performance
- **Duration:** ~20 min
- **Tasks:** 2 completed
- **Files modified:** 4

## Accomplishments
- `CdController::eject()` and `playTrack(int)` are now `Q_INVOKABLE` — callable from QML
- `CdController.toc` Q_PROPERTY returns `QVariantList` of `{trackNumber, durationSeconds, title}` maps
- `tocAsVariantList()` merges TocEntry (sector data) with CdTrackInfo (metadata titles) into QML-friendly format
- `tocReady` is re-emitted in `onMetadataReady()` so QML track list updates with titles when GnuDB lookup completes
- `CdController` registered as `"CdController"` QML singleton in `main.cpp` before `engine.load()`
- `UIState::restartRequested()` signal added — QML Restart Now button emits this; main.cpp wires it to `QCoreApplication::quit()` with diagnostic logging

## Task Commits
1. **Task 1-2: CdController QML exposure + UIState restart signal** - `c84e2f4`

## Files Created/Modified
- `src/cd/CdController.h` — Q_INVOKABLE on eject/playTrack, Q_PROPERTY toc, tocAsVariantList() declaration
- `src/cd/CdController.cpp` — tocAsVariantList() implementation, emit tocReady in onMetadataReady
- `src/state/UIState.h` — restartRequested() signal added
- `src/main.cpp` — CdController singleton registration, UIState::restartRequested → quit() connection

## Decisions & Deviations
- **No UIState.cpp changes needed**: Qt moc generates signal implementations automatically; only header declaration required.
- **Progressive title loading**: Emitting `tocReady` a second time in `onMetadataReady()` means QML re-evaluates the `toc` binding when track titles arrive — track list shows "Track N" initially, then real title when metadata loads.

## Next Phase Readiness
- Plan 03 (Wave 2) can now use `CdController.toc`, `CdController.eject()`, `CdController.playTrack()`, and `UIState.restartRequested()` from QML — all foundations are in place.
