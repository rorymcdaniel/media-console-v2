---
phase: 11-integration-wiring-fixes
plan: "03"
subsystem: [qml, ui]
tags: [main.qml, NowPlaying, CdController, eject-modal, track-list, AudioErrorDialog]
provides:
  - Eject button calls CdController.eject() (was empty comment)
  - Eject confirmation modal when playback is active
  - CD track list in NowPlaying.qml bound to CdController.toc
  - AudioErrorDialog "Restart Now" button calls UIState.restartRequested()
affects: [cd-playback, audio-error-dialog, now-playing-view]
tech-stack:
  added: []
  patterns: [modal visibility via bool property, ListView for scrollable track list, anchors.bottom for layout split]
key-files:
  created: []
  modified:
    - src/qml/main.qml
    - src/qml/components/NowPlaying.qml
    - tests/test_ReceiverController.cpp
key-decisions:
  - "Eject confirmation uses ejectConfirmVisible bool property on root Window item (simple, no extra components)"
  - "Eject button skips modal when not playing — immediate eject for idle/stopped state"
  - "CD track list uses ListView (not Repeater) for clip/scroll — handles full albums correctly"
  - "Track list positioned at bottom of infoContainer, info column shrinks when CD source active"
  - "infoColumnWrapper Item wraps the Column to allow anchors.bottom binding without Column height conflict"
  - "Track list visible when activeSource===CD (not isDiscPresent — avoids non-notifiable accessor)"
  - "Confirm modal buttons: Cancel/Eject (not No/Yes) for clearer touch affordance"
duration: 30min
completed: 2026-02-28
---

# Phase 11 Plan 03: QML Integration Wiring Summary

**Completed all 5 integration bug fixes at the UI layer: eject flow, CD track selection, and audio error restart.**

## Performance
- **Duration:** ~30 min
- **Tasks:** 2 completed (+ test fix)
- **Files modified:** 3

## Accomplishments
- `main.qml` eject button now calls `CdController.eject()` directly when idle/stopped, or shows confirmation modal when playing
- Eject confirmation modal: semi-transparent backdrop, Cancel/Eject buttons, `z: 500` with fade animation
- `AudioErrorDialog` "Retry" button replaced with "Restart Now" → `UIState.restartRequested()` (systemd will restart process)
- `NowPlaying.qml` shows scrollable CD track list at bottom of right panel when `PlaybackState.activeSource === MediaSource.CD`
- Track list bound to `CdController.toc` — each row shows track number, title (or "Track N" fallback), duration
- Tapping a track calls `CdController.playTrack(modelData.trackNumber)` — wired to `LocalPlaybackController`
- Currently-playing track highlighted with accent background
- Track list auto-scrollable via ListView with clip — handles full albums
- 3 ReceiverController tests updated to verify `volumeReceivedFromReceiver` signal via `QSignalSpy`

## Task Commits
1. **Task 1-2: QML wiring + test fix** - `5e45273`

## Files Created/Modified
- `src/qml/main.qml` — ejectConfirmVisible property, eject button handler, eject confirmation modal, Restart Now button
- `src/qml/components/NowPlaying.qml` — cdTrackListContainer ListView at bottom, infoColumnWrapper layout split
- `tests/test_ReceiverController.cpp` — ParseMvl tests updated to use QSignalSpy for signal verification

## Decisions & Deviations
- **Layout approach**: Used `infoColumnWrapper` Item with `anchors.bottom` pointing to `cdTrackListContainer.top` rather than splitting into two separate Items. This allows the info column to naturally shrink when CD is active without breaking the existing centered layout for non-CD sources.
- **ListView not Repeater**: Chose ListView over Repeater to get clipping and scrolling — a full album can have 20+ tracks which would overflow without scrolling.
- **No `isDiscPresent` Q_PROPERTY needed**: Track list visible condition uses `PlaybackState.activeSource === MediaSource.CD` which changes when CD playback starts — avoids needing to add a Q_PROPERTY to CdController for `isDiscPresent`.
- **Test fix bundled in commit**: The 3 broken ReceiverController tests (MVL volume) were fixed in the same commit since they were blocking clean test runs.

## Next Phase Readiness
- All 5 integration bugs (AUDIO-06, AUDIO-07, FLAC-08, UI-09, UI-13, ORCH-03) are now closed
- 365/365 tests pass
- Phase 12 (Verification and Traceability) can proceed
