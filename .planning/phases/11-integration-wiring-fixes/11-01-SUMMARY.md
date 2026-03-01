---
phase: 11-integration-wiring-fixes
plan: "01"
subsystem: [backend, orchestration, audio]
tags: [Q_INVOKABLE, signals, PlaybackRouter, AppBuilder, VolumeGestureController]
provides:
  - FlacLibraryController methods callable from QML
  - ReceiverController volume via signal (gesture-aware path)
  - PlaybackRouter seek() functional for CD and Library sources
  - AppBuilder wires audioRecoveryFailed and volumeReceivedFromReceiver
affects: [qml-layer, cd-playback, library-playback, volume-control, audio-error-dialog]
tech-stack:
  added: []
  patterns: [signal-based decoupling, Q_INVOKABLE annotation, AppBuilder composition root]
key-files:
  created: []
  modified:
    - src/library/FlacLibraryController.h
    - src/receiver/ReceiverController.h
    - src/receiver/ReceiverController.cpp
    - src/orchestration/PlaybackRouter.h
    - src/orchestration/PlaybackRouter.cpp
    - src/app/AppBuilder.cpp
    - tests/test_ReceiverController.cpp
key-decisions:
  - "Q_INVOKABLE added to declarations only (not .cpp implementations)"
  - "MVL volume now emits signal instead of direct setVolume — VolumeGestureController reconciles state"
  - "CD and Library seek() cases collapsed into one case with fallthrough"
  - "LocalPlaybackController passed to PlaybackRouter as 6th constructor parameter"
  - "Tests updated to verify signal emission (QSignalSpy) instead of state change"
duration: 25min
completed: 2026-02-28
---

# Phase 11 Plan 01: Backend Integration Wiring Summary

**Fixed 3 backend integration gaps: Q_INVOKABLE annotations, volume signal refactor, and PlaybackRouter seek implementation.**

## Performance
- **Duration:** ~25 min
- **Tasks:** 3 completed
- **Files modified:** 6 (+ 1 test file fix)

## Accomplishments
- `FlacLibraryController::playTrack()`, `next()`, `previous()` now have `Q_INVOKABLE` — QML calls no longer silently fail
- `ReceiverController` MVL parsing now emits `volumeReceivedFromReceiver(int)` instead of calling `setVolume()` directly, routing volume through `VolumeGestureController::onExternalVolumeUpdate()`
- `PlaybackRouter::seek()` now calls `m_localPlaybackController->seek(qint64)` for CD and Library sources (was a no-op)
- `AppBuilder` wires `LocalPlaybackController::audioRecoveryFailed` → `UIState::setAudioError()` lambda
- `AppBuilder` wires `ReceiverController::volumeReceivedFromReceiver` → `VolumeGestureController::onExternalVolumeUpdate()`

## Task Commits
1. **Task 1-3: All backend wiring** - `0428bbf`
   - All 3 tasks committed together: Q_INVOKABLE, signal refactor, AppBuilder connections, PlaybackRouter seek

## Files Created/Modified
- `src/library/FlacLibraryController.h` — added Q_INVOKABLE to playTrack, next, previous
- `src/receiver/ReceiverController.h` — added volumeReceivedFromReceiver(int) signal
- `src/receiver/ReceiverController.cpp` — MVL case emits signal instead of direct setVolume
- `src/orchestration/PlaybackRouter.h` — added LocalPlaybackController* parameter + member
- `src/orchestration/PlaybackRouter.cpp` — updated constructor, fixed seek() for CD/Library
- `src/app/AppBuilder.cpp` — two new connect() calls, updated PlaybackRouter construction
- `tests/test_ReceiverController.cpp` — 3 ParseMvl tests updated to use QSignalSpy

## Decisions & Deviations
- **CdController Q_INVOKABLE** was originally in this plan but moved to Plan 02 to avoid parallel file ownership conflict (Plan 01 and Plan 02 both run in Wave 1)
- **Test update required**: 3 ReceiverController tests were broken because they expected `receiverState->volume()` to be set directly. Updated to verify `volumeReceivedFromReceiver` signal emission via QSignalSpy instead.

## Next Phase Readiness
- Plan 02 (Wave 1, parallel): CdController Q_INVOKABLE + toc Q_PROPERTY + UIState restart signal ready to proceed
- Plan 03 (Wave 2): QML wiring depends on both Plan 01 and Plan 02 being complete
