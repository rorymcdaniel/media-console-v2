# Phase 4: Audio Pipeline - Research

**Researched:** 2026-02-28
**Domain:** ALSA PCM audio output, threaded playback controller, audio stream abstraction
**Confidence:** HIGH

## Summary

Phase 4 builds the audio playback infrastructure: an AudioStream interface that CD and FLAC sources will implement (in phases 5 and 6), an AlsaAudioOutput that implements the existing IAudioOutput interface using the ALSA library, and a LocalPlaybackController that runs a background thread to read frames from an AudioStream and write them to IAudioOutput with buffering, error recovery, and playback controls.

The ALSA C library (libasound2) is the standard Linux audio output API for direct hardware access. The project already has the IAudioOutput interface with open/close/reset/pause/writeFrames/isOpen/deviceName methods, and a StubAudioOutput for non-Linux platforms. AlsaAudioOutput implements this interface using snd_pcm_* functions. The playback controller uses a dedicated QThread with atomic flags for stop/pause/seek control, communicating position updates back to the main thread via queued signal/slot connections.

**Primary recommendation:** Implement in three plans: (1) AudioStream interface + AudioConfig + AlsaAudioOutput, (2) LocalPlaybackController with background thread and playback controls, (3) AppBuilder wiring + integration tests with a test AudioStream that generates a sine wave.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Pause cuts audio immediately (ALSA reset/flush), no buffer drain
- Seek updates the progress bar optimistically (same pattern as volume gesture coalescing in Phase 3)
- During seek, briefly mute output while rebuffering at the new position (~1 second prefill), then resume — prevents audible glitches
- Position updates emitted every 250ms for smooth progress bar movement
- play(newStream) auto-stops the current stream — architectural exclusivity, not caller responsibility
- Brief silence between sources is acceptable (~1 second while new stream opens and prefills)
- No gapless playback between sources — this is a kiosk, not a DJ setup
- Controller is source-agnostic: caller provides AudioStream, controller plays it. Next/previous logic lives in the calling subsystem (CD or FLAC)
- End-of-track: controller emits trackFinished(), stops playback, waits for caller to provide next stream via play()
- ALSA recovery (close/reopen) is invisible to the user if it succeeds — just a brief audio dropout
- When all 3 retries exhaust: emit audioRecoveryFailed signal, playback stops, UI shows modal AudioErrorDialog with retry option (matches UI-09 spec)
- After successful recovery (or user retry): resume playback from the last known position, not from track start
- Buffer statistics (xrun count, read latency, error count) logged to media.audio category only — not exposed in UI
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

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| AUDIO-01 | AudioStream interface (open, readFrames, totalFrames, close, seek) implemented by CdAudioStream and FlacAudioStream | Define interface in this phase; CD/FLAC implementations in phases 5/6 |
| AUDIO-02 | Single LocalPlaybackController parameterized by AudioStream — replaces separate CD and FLAC controllers | Controller architecture pattern, thread design |
| AUDIO-03 | ALSA PCM output to S/PDIF device (hw:2,0) at 44100Hz, 16-bit, stereo via IAudioOutput interface | ALSA library usage, AlsaAudioOutput implementation |
| AUDIO-04 | Background thread playback loop with atomic flag control (stop, pause, pending track, pending seek) | QThread + atomic flags pattern |
| AUDIO-05 | Intelligent audio buffering: 8-second buffer, 1-second prefill target, max 3 retries with 50ms backoff | Ring buffer design, prefill logic |
| AUDIO-06 | Buffer statistics: xrun tracking, per-read latency (avg/max microseconds), read error counting | Stats struct, chrono timing |
| AUDIO-07 | EIO recovery: close and reopen ALSA device on I/O errors, emit audioRecoveryFailed when exhausted | ALSA error handling, recovery pattern |
| AUDIO-08 | Full playback control: play, pause, stop, next, previous, seek (sector-based for CD, sample-based for FLAC) | Controller public API, seek abstraction |
| AUDIO-09 | ALSA device exclusivity enforced architecturally — one controller, one device, mutually exclusive sources | Single controller owns single IAudioOutput |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libasound2 (ALSA lib) | 1.2.x | PCM audio output to hardware devices | The Linux kernel audio API; only way to do direct hardware S/PDIF output on Pi |
| Qt6::Core (QThread) | 6.8.2 | Background playback thread | Already in project; provides signal/slot across threads with queued connections |
| std::atomic | C++17 | Lock-free control flags (stop, pause, seek) | Standard library; no overhead for single-writer/single-reader flags |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| std::chrono | C++17 | Microsecond timing for buffer statistics | Per-read latency measurement |
| std::vector | C++17 | Ring buffer backing store | Simple, cache-friendly, known size at init |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| QThread | std::thread | QThread integrates with Qt event loop for signal/slot; std::thread requires manual event dispatch. QThread is better here. |
| Ring buffer | QByteArray linear buffer | Ring buffer avoids memmove on consume; linear buffer simpler but O(n) shifts. Ring buffer wins for audio. |
| ALSA | PulseAudio/PipeWire | PulseAudio adds latency and doesn't guarantee bit-perfect. Direct ALSA is required for S/PDIF HAT. |

**Installation (on Pi 5 Trixie):**
```bash
sudo apt install libasound2-dev
```

**CMake:**
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
target_link_libraries(media-console-lib PUBLIC PkgConfig::ALSA)
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── audio/
│   ├── AudioStream.h           # Abstract interface for audio sources
│   ├── AudioBufferStats.h      # POD struct for buffer statistics
│   ├── LocalPlaybackController.h/.cpp  # Main thread API + background thread
│   └── AlsaAudioOutput.h/.cpp  # IAudioOutput implementation for ALSA
├── platform/
│   ├── IAudioOutput.h          # Existing interface (unchanged)
│   └── stubs/
│       └── StubAudioOutput.h/.cpp  # Existing stub (unchanged)
└── app/
    ├── AppConfig.h             # Add AudioConfig struct
    ├── AppBuilder.h/.cpp       # Add LocalPlaybackController ownership
    └── AppContext.h            # Add LocalPlaybackController* pointer
```

### Pattern 1: QThread Worker with Atomic Control Flags
**What:** LocalPlaybackController creates a QThread internally. The playback loop runs on that thread, reading from AudioStream and writing to IAudioOutput. Main-thread methods (play, pause, stop, seek) set atomic flags that the loop checks each iteration.
**When to use:** When you need a long-running loop with responsive control from another thread.
**Example:**
```cpp
class LocalPlaybackController : public QObject
{
    Q_OBJECT

public:
    // Main thread API
    void play(std::shared_ptr<AudioStream> stream);
    void pause();
    void resume();
    void stop();
    void seek(qint64 positionMs);

signals:
    void positionChanged(qint64 positionMs);
    void trackFinished();
    void audioRecoveryFailed();

private:
    void playbackLoop();  // Runs on m_thread

    QThread m_thread;
    IAudioOutput* m_audioOutput;        // Non-owning
    PlaybackState* m_playbackState;     // Non-owning

    std::shared_ptr<AudioStream> m_currentStream;
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_pauseRequested{false};
    std::atomic<bool> m_seekPending{false};
    std::atomic<qint64> m_seekTargetMs{0};
};
```

### Pattern 2: Ring Buffer for Audio Data
**What:** A fixed-size ring buffer between the reader (AudioStream) and writer (IAudioOutput). The playback loop fills the buffer from AudioStream, then drains it to IAudioOutput. Buffer size = 8 seconds of audio at 44100Hz/16-bit/stereo = 8 * 44100 * 2 * 2 = 1,411,200 bytes (~1.35 MB).
**When to use:** Decouples read and write rates, absorbs jitter from disk I/O.
**Example:**
```cpp
class AudioRingBuffer
{
public:
    explicit AudioRingBuffer(size_t capacityFrames);
    size_t write(const int16_t* data, size_t frames);
    size_t read(int16_t* data, size_t frames);
    size_t availableFrames() const;
    size_t freeFrames() const;
    void clear();

private:
    std::vector<int16_t> m_buffer;  // capacityFrames * channels
    size_t m_readPos = 0;
    size_t m_writePos = 0;
    size_t m_count = 0;  // frames currently in buffer
    size_t m_capacityFrames;
    static constexpr int kChannels = 2;
};
```

### Pattern 3: ALSA PCM Setup for S/PDIF
**What:** Open ALSA PCM device in SND_PCM_ACCESS_RW_INTERLEAVED mode with exact hardware parameters.
**Example:**
```cpp
bool AlsaAudioOutput::open(const QString& deviceName, unsigned int sampleRate,
                           unsigned int channels, unsigned int bitDepth)
{
    int err = snd_pcm_open(&m_handle, deviceName.toLocal8Bit().constData(),
                           SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) return false;

    snd_pcm_hw_params_t* hwParams;
    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_hw_params_any(m_handle, hwParams);
    snd_pcm_hw_params_set_access(m_handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(m_handle, hwParams, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(m_handle, hwParams, channels);
    snd_pcm_hw_params_set_rate(m_handle, hwParams, sampleRate, 0);

    // Period = 1024 frames (~23ms at 44100Hz), Buffer = 8192 frames (~186ms)
    snd_pcm_uframes_t periodSize = 1024;
    snd_pcm_uframes_t bufferSize = 8192;
    snd_pcm_hw_params_set_period_size_near(m_handle, hwParams, &periodSize, nullptr);
    snd_pcm_hw_params_set_buffer_size_near(m_handle, hwParams, &bufferSize, nullptr);

    err = snd_pcm_hw_params(m_handle, hwParams);
    if (err < 0) { snd_pcm_close(m_handle); m_handle = nullptr; return false; }

    return true;
}
```

### Anti-Patterns to Avoid
- **Blocking the main thread with audio I/O:** Never call writeFrames or readFrames from the main thread. All audio I/O happens on the playback thread.
- **Using mutex for pause/stop control:** Mutexes introduce priority inversion risk in real-time audio. Use atomic flags instead — single-writer (main thread), single-reader (playback thread).
- **Calling snd_pcm_drain on pause:** drain blocks until all buffered data is played. Use snd_pcm_drop (or reset) for immediate pause as per user decision.
- **Not handling ALSA underruns (EPIPE):** If writeFrames returns -EPIPE, call snd_pcm_prepare before retrying. This is distinct from EIO recovery.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| ALSA PCM output | Custom audio driver | libasound2 snd_pcm_* API | Kernel interface, no alternative for direct hardware access |
| Thread-safe position updates | Manual mutex + condition variable | QThread + queued signal/slot | Qt handles thread affinity and event loop dispatch automatically |
| Audio format conversion | Manual byte swapping / resampling | Defer to AudioStream implementations (phase 5/6) | Controller only handles 44100/16/stereo; sources must resample |

**Key insight:** The controller should be format-agnostic. It receives 44100Hz/16-bit/stereo frames from AudioStream and writes them to IAudioOutput. Any format conversion (resampling, bit depth) is the AudioStream's responsibility.

## Common Pitfalls

### Pitfall 1: ALSA Buffer Underrun (EPIPE)
**What goes wrong:** writeFrames returns -EPIPE when the hardware runs out of data to play.
**Why it happens:** Playback thread was too slow filling the buffer (disk I/O stall, CPU spike, GC pause).
**How to avoid:** Use the 8-second application-level ring buffer as a cushion. Keep ALSA hardware buffer modest (8192 frames / ~186ms). Prefill 1 second before starting playback.
**Warning signs:** Audible clicks/pops. snd_pcm_state returns SND_PCM_STATE_XRUN.

### Pitfall 2: Thread Lifetime vs QObject Lifetime
**What goes wrong:** LocalPlaybackController destroyed while playback thread is still running, causing use-after-free.
**Why it happens:** QThread doesn't automatically stop when the parent is destroyed.
**How to avoid:** In destructor: set m_stopRequested = true, call m_thread.quit() + m_thread.wait(). Ensure clean shutdown ordering.
**Warning signs:** Crash on application exit, sporadic segfaults.

### Pitfall 3: Signal/Slot Thread Affinity
**What goes wrong:** Emitting a signal connected to a slot on a different thread causes direct call instead of queued invocation.
**Why it happens:** Default connection type is Qt::AutoConnection, which chooses based on caller/receiver thread affinity.
**How to avoid:** Move the worker object to the thread using moveToThread(). Or explicitly use Qt::QueuedConnection for cross-thread connections. Since LocalPlaybackController lives on the main thread but runs its loop on m_thread, use QMetaObject::invokeMethod with Qt::QueuedConnection to emit signals from the playback loop.
**Warning signs:** UI freezes during audio operations, race conditions on state.

### Pitfall 4: ALSA Device Remains Open After Error
**What goes wrong:** EIO error occurs, recovery fails, but ALSA handle is still open. Next open() call fails because device is busy.
**Why it happens:** Error path doesn't call snd_pcm_close().
**How to avoid:** Always close the handle in the error recovery path before attempting reopen. Track handle state carefully.
**Warning signs:** "Device or resource busy" errors after recovery attempt.

### Pitfall 5: Seek Race Condition
**What goes wrong:** Main thread sets seek position, but playback thread has already read past that point from the ring buffer.
**Why it happens:** Ring buffer contains pre-read data from the old position.
**How to avoid:** On seek: set atomic seek flag + target, playback loop detects flag, clears ring buffer, calls AudioStream::seek(), refills from new position.
**Warning signs:** Audio from wrong position after seek, brief burst of old audio.

## Code Examples

### ALSA writeFrames with Error Handling
```cpp
long AlsaAudioOutput::writeFrames(const int16_t* interleaved, size_t frames)
{
    snd_pcm_sframes_t written = snd_pcm_writei(m_handle, interleaved, frames);
    if (written == -EPIPE)
    {
        // Underrun recovery
        snd_pcm_prepare(m_handle);
        written = snd_pcm_writei(m_handle, interleaved, frames);
    }
    else if (written == -EIO)
    {
        // Hardware error — caller should initiate close/reopen recovery
        return -1;
    }
    else if (written < 0)
    {
        // Other error
        int recovered = snd_pcm_recover(m_handle, static_cast<int>(written), 1 /* silent */);
        if (recovered < 0) return -1;
        written = snd_pcm_writei(m_handle, interleaved, frames);
    }
    return written;
}
```

### Playback Loop Core
```cpp
void LocalPlaybackController::playbackLoop()
{
    std::vector<int16_t> chunk(kPeriodFrames * kChannels);

    while (!m_stopRequested.load(std::memory_order_relaxed))
    {
        if (m_pauseRequested.load(std::memory_order_relaxed))
        {
            QThread::msleep(10);
            continue;
        }

        if (m_seekPending.load(std::memory_order_acquire))
        {
            handleSeek();
            continue;
        }

        // Fill ring buffer from AudioStream
        fillBuffer();

        // Drain ring buffer to IAudioOutput
        size_t available = m_ringBuffer.availableFrames();
        if (available == 0)
        {
            // End of stream
            emitTrackFinished();
            break;
        }

        size_t toWrite = std::min(available, static_cast<size_t>(kPeriodFrames));
        m_ringBuffer.read(chunk.data(), toWrite);
        long written = m_audioOutput->writeFrames(chunk.data(), toWrite);
        if (written < 0)
        {
            if (!attemptRecovery()) break;
        }

        updatePosition();
    }
}
```

### Position Update Throttling
```cpp
void LocalPlaybackController::updatePosition()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPositionUpdate);
    if (elapsed.count() >= kPositionUpdateIntervalMs)
    {
        qint64 posMs = framesToMs(m_framesPlayed);
        QMetaObject::invokeMethod(this, [this, posMs]() {
            m_playbackState->setPositionMs(posMs);
        }, Qt::QueuedConnection);
        m_lastPositionUpdate = now;
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| OSS /dev/dsp | ALSA snd_pcm_* | 2002+ | OSS deprecated on Linux, ALSA is the kernel API |
| PulseAudio for all audio | PipeWire replacing PulseAudio | 2021+ | For direct hardware S/PDIF, still use ALSA directly |
| QAudioOutput (Qt Multimedia) | Direct ALSA for bit-perfect | Always | Qt Multimedia adds resampling/mixing; not suitable for bit-perfect S/PDIF |

**Deprecated/outdated:**
- Qt Multimedia QAudioOutput: Adds a mixing layer, does not guarantee bit-perfect output, not suitable for direct S/PDIF HAT access
- OSS (/dev/dsp): Completely replaced by ALSA on modern Linux

## Open Questions

1. **ALSA Period/Buffer Size Tuning**
   - What we know: 1024-frame periods (~23ms) and 8192-frame ALSA buffer (~186ms) are common starting points
   - What's unclear: Optimal values depend on Pi 5 S/PDIF HAT DMA behavior — may need adjustment
   - Recommendation: Use these as defaults, tune on hardware if needed. Named constants make adjustment easy.

2. **AudioStream Seek Units**
   - What we know: CD seeks by sector (1/75 second), FLAC seeks by sample
   - What's unclear: Whether to use a unified unit (milliseconds) or source-specific units
   - Recommendation: Use milliseconds as the unified seek unit in the controller API. AudioStream implementations convert internally.

## Sources

### Primary (HIGH confidence)
- ALSA library API — direct experience with snd_pcm_* functions, well-documented in alsa-project.org
- Qt6 QThread documentation — signal/slot thread affinity, moveToThread, queued connections
- Existing codebase — IAudioOutput interface, StubAudioOutput, AppBuilder pattern, PlaybackState

### Secondary (MEDIUM confidence)
- ALSA period/buffer sizing recommendations from various audio programming references
- Ring buffer pattern for audio applications

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - ALSA is the only option for direct hardware S/PDIF, well-understood
- Architecture: HIGH - QThread + atomic flags is a proven pattern for audio playback, matches existing project patterns
- Pitfalls: HIGH - ALSA error handling, thread lifetime, seek races are well-documented problems

**Research date:** 2026-02-28
**Valid until:** 2026-03-30 (stable domain, ALSA API hasn't changed significantly in years)
