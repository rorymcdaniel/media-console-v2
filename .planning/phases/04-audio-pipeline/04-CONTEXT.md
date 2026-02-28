# Phase 4: Audio Pipeline - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

A unified LocalPlaybackController that accepts any AudioStream (CD or FLAC) and outputs bit-perfect audio through the S/PDIF HAT via ALSA. Includes playback controls, background thread, buffering, and error recovery. CD and FLAC stream implementations are separate phases (5 and 6).

</domain>

<decisions>
## Implementation Decisions

### Seek & pause feel
- Pause cuts audio immediately (ALSA reset/flush), no buffer drain
- Seek updates the progress bar optimistically (same pattern as volume gesture coalescing in Phase 3)
- During seek, briefly mute output while rebuffering at the new position (~1 second prefill), then resume — prevents audible glitches
- Position updates emitted every 250ms for smooth progress bar movement

### Source transitions
- play(newStream) auto-stops the current stream — architectural exclusivity, not caller responsibility
- Brief silence between sources is acceptable (~1 second while new stream opens and prefills)
- No gapless playback between sources — this is a kiosk, not a DJ setup
- Controller is source-agnostic: caller provides AudioStream, controller plays it. Next/previous logic lives in the calling subsystem (CD or FLAC)
- End-of-track: controller emits trackFinished(), stops playback, waits for caller to provide next stream via play()

### Error recovery UX
- ALSA recovery (close/reopen) is invisible to the user if it succeeds — just a brief audio dropout
- When all 3 retries exhaust: emit audioRecoveryFailed signal, playback stops, UI shows modal AudioErrorDialog with retry option (matches UI-09 spec)
- After successful recovery (or user retry): resume playback from the last known position, not from track start
- Buffer statistics (xrun count, read latency, error count) logged to media.audio category only — not exposed in UI

### Device configuration
- ALSA device name configurable via new AudioConfig section in AppConfig, defaulting to "hw:2,0"
- Buffer parameters (8s buffer, 1s prefill, 3 retries, 50ms backoff) are named constants in code, not configurable — values from requirements are tuned for Pi 5
- Sample rate (44100Hz), channels (2), bit depth (16) are locked constants — hardware dictates format, upstream sources resample to match
- StubAudioOutput silently accepts all writes (no timing simulation) — real playback testing on Pi hardware

### Claude's Discretion
- AudioStream interface method signatures and error reporting details
- Playback thread implementation (QThread vs std::thread, synchronization approach)
- Ring buffer vs linear buffer for audio data
- ALSA period/buffer size tuning
- Exact signal/slot wiring between playback thread and main thread for position updates

</decisions>

<specifics>
## Specific Ideas

- Optimistic seek should follow the same pattern as VolumeGestureController's optimistic volume updates — consistent "update UI first, confirm later" pattern across the app
- Position update interval (250ms) matches common media player standards

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `IAudioOutput` interface (`src/platform/IAudioOutput.h`): Already defines open/close/reset/pause/writeFrames/isOpen/deviceName — ready for ALSA implementation
- `StubAudioOutput` (`src/platform/stubs/StubAudioOutput.h`): Stub for non-Linux platforms, already wired in PlatformFactory
- `PlaybackState` (`src/state/PlaybackState.h`): Q_PROPERTY bag with positionMs, durationMs, playbackMode, activeSource, track info — controller writes to this
- `PlaybackMode` enum: Stopped, Playing, Paused — maps directly to controller states
- `MediaSource` enum: CD, Library — identifies which source is active

### Established Patterns
- Platform abstraction: interfaces (IAudioOutput) with stubs for non-Linux, real implementations selected by PlatformFactory
- State objects: thin Q_PROPERTY bags with no business logic — controller owns the logic, state is just the binding surface
- Composition root: AppBuilder owns all objects via unique_ptr, AppContext provides non-owning pointers
- Signal/slot communication between layers (ReceiverController -> ReceiverState pattern)

### Integration Points
- `AppBuilder` already owns `IAudioOutput` and `PlaybackState` — needs new `LocalPlaybackController` added
- `AppContext` needs `LocalPlaybackController*` pointer for caller access
- `AppConfig` needs new `AudioConfig` struct with device name
- New `AlsaAudioOutput` implementing `IAudioOutput` for real ALSA PCM output
- PlatformFactory needs to create AlsaAudioOutput on Linux

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 04-audio-pipeline*
*Context gathered: 2026-02-28*
