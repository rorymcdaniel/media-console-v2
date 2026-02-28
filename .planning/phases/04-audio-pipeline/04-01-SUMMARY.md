---
phase: 04-audio-pipeline
plan: 01
status: completed
started: 2026-02-28
completed: 2026-02-28
duration_minutes: 5
commits: [336bd99]
---

# Plan 04-01 Summary: AudioStream Interface, AlsaAudioOutput, AudioConfig

## What Was Built

### AudioStream Interface (`src/audio/AudioStream.h`)
Pure virtual interface for audio sources with open/close/readFrames/totalFrames/positionFrames/seek and format accessor methods. CD and FLAC streams implement this in phases 5/6.

### AlsaAudioOutput (`src/audio/AlsaAudioOutput.h/.cpp`)
IAudioOutput implementation using libasound2 (ALSA) for direct PCM output to S/PDIF HAT. Configures hardware for SND_PCM_ACCESS_RW_INTERLEAVED, SND_PCM_FORMAT_S16_LE, period 1024, buffer 8192. Error handling: EPIPE underrun recovery via snd_pcm_prepare, EIO returns -1 for caller recovery, generic snd_pcm_recover for other errors. reset() uses snd_pcm_drop for immediate silence.

### AudioConfig (`src/app/AppConfig.h`)
New config struct with deviceName defaulting to "hw:2,0", loaded from QSettings audio/deviceName.

### PlatformFactory Update
Conditional compilation: `#ifdef HAS_ALSA` creates AlsaAudioOutput on Linux, StubAudioOutput elsewhere.

### CMake ALSA Integration
pkg_check_modules for ALSA on Linux, HAS_ALSA compile definition, conditional source inclusion.

### TestAudioStream + 18 Tests
Test stub producing silence with configurable frame count and format. 18 Google Test cases covering open/close, format accessors, readFrames, seek, edge cases.

## Key Decisions
- AudioStream uses `size_t` for frame positions (natural for audio, no negative positions)
- readFrames returns `long` (-1 for error, 0 for end, positive for frames read)
- AlsaAudioOutput forward-declares `snd_pcm_t` in header, includes `<alsa/asoundlib.h>` only in .cpp
- PlatformFactory uses compile-time HAS_ALSA check, not runtime isLinux()

## Self-Check: PASSED

| Must-Have | Status |
|-----------|--------|
| AudioStream pure virtual interface | PASS |
| AlsaAudioOutput implements IAudioOutput | PASS |
| EPIPE/EIO error handling | PASS |
| reset() uses snd_pcm_drop | PASS |
| AudioConfig with hw:2,0 default | PASS |
| PlatformFactory conditional | PASS |
| TestAudioStream + tests pass | PASS |

## Key Files

### Created
- `src/audio/AudioStream.h` — Pure virtual interface
- `src/audio/AlsaAudioOutput.h` — ALSA implementation header
- `src/audio/AlsaAudioOutput.cpp` — ALSA implementation
- `tests/test_AudioStream.cpp` — 18 tests with TestAudioStream

### Modified
- `src/app/AppConfig.h` — Added AudioConfig struct
- `src/app/AppConfig.cpp` — Added audio settings loading
- `src/platform/PlatformFactory.cpp` — Conditional ALSA/Stub
- `CMakeLists.txt` — ALSA detection, sources, linking
- `tests/CMakeLists.txt` — Added test_AudioStream.cpp

## Test Results
- 18 new AudioStream tests: all pass
- 181 total tests: all pass
