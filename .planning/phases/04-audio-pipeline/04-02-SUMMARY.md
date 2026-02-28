---
phase: 04-audio-pipeline
plan: 02
status: completed
started: 2026-02-28
completed: 2026-02-28
duration_minutes: 8
commits: [525c01e]
---

# Plan 04-02 Summary: AudioRingBuffer, AudioBufferStats, LocalPlaybackController

## What Was Built

### AudioRingBuffer (`src/audio/AudioRingBuffer.h/.cpp`)
Lock-free circular buffer for interleaved int16_t audio frames. Uses modular arithmetic on sample positions with two-part memcpy for wraparound. write() and read() return actual frames transferred (partial on full/empty). Reports availableFrames, freeFrames, capacityFrames. clear() resets all positions.

### AudioBufferStats (`src/audio/AudioBufferStats.h`)
POD struct tracking xrunCount, readLatencyAvgUs, readLatencyMaxUs, errorCount, totalReads. Inline methods: recordReadLatency() computes running average, recordXrun(), recordError(), reset().

### LocalPlaybackController (`src/audio/LocalPlaybackController.h/.cpp`)
Unified playback engine for any AudioStream source. Reads frames via AudioStream, buffers through AudioRingBuffer (8s capacity), writes to IAudioOutput on a background QThread. Controls:
- **play(stream):** Auto-stops current playback (architectural exclusivity), opens stream + output, prefills 1s of buffer, starts thread, sets Playing state.
- **pause():** Immediate silence via m_audioOutput->reset() (no drain per user decision), sets Paused.
- **resume():** Clears pause flag, sets Playing.
- **stop():** Stops thread, closes stream + output, sets Stopped, resets position.
- **seek(positionMs):** Optimistic UI update (per user decision), sets atomic flag, thread clears buffer + rebuffers from new position.

Thread safety: atomic flags (m_stopRequested, m_pauseRequested, m_seekPending) for lock-free control. Position updates every 250ms via QMetaObject::invokeMethod with Qt::QueuedConnection. End-of-track and recovery failure signals delivered via QueuedConnection.

EIO recovery: closes and reopens ALSA device up to 3 times with 50ms backoff. audioRecoveryFailed signal on exhaustion.

## Key Decisions
- Ring buffer operates in frames externally, samples internally (frames * channels)
- Playback thread started via QThread::started signal with Qt::DirectConnection lambda
- TestPlaybackStream uses throttle mode (1ms sleep per readFrames call) to prevent instant stream exhaustion with StubAudioOutput in tests
- Tests check synchronous state (play() sets Playing immediately on main thread) rather than relying on processEvents timing

## Self-Check: PASSED

| Must-Have | Status |
|-----------|--------|
| AudioRingBuffer write/read/clear with wraparound | PASS |
| AudioRingBuffer reports availableFrames/freeFrames | PASS |
| play(stream) stops current, opens new, starts thread | PASS |
| pause() immediate silence (ALSA reset, no drain) | PASS |
| stop() stops thread, closes stream | PASS |
| seek() optimistic UI update, thread rebuffers | PASS |
| QThread with atomic flags for control | PASS |
| Position updates every 250ms via QueuedConnection | PASS |
| EIO recovery: 3 retries, 50ms backoff | PASS |
| audioRecoveryFailed signal on exhaustion | PASS |
| trackFinished signal at end of stream | PASS |
| Buffer stats tracked and logged to media.audio | PASS |
| Architectural exclusivity: play(new) auto-stops | PASS |

## Key Files

### Created
- `src/audio/AudioRingBuffer.h` -- Ring buffer interface
- `src/audio/AudioRingBuffer.cpp` -- Ring buffer implementation
- `src/audio/AudioBufferStats.h` -- Stats POD struct with inline methods
- `src/audio/LocalPlaybackController.h` -- Playback controller interface
- `src/audio/LocalPlaybackController.cpp` -- Playback controller implementation
- `tests/test_AudioRingBuffer.cpp` -- 10 ring buffer + 5 stats tests
- `tests/test_LocalPlaybackController.cpp` -- 10 controller tests

### Modified
- `CMakeLists.txt` -- Added all new source files to LIB_SOURCES
- `tests/CMakeLists.txt` -- Added both test files to TEST_SOURCES

## Test Results
- 10 AudioRingBuffer tests: all pass
- 5 AudioBufferStats tests: all pass
- 10 LocalPlaybackController tests: all pass
- 206 total tests: all pass
