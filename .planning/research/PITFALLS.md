# Pitfalls Research

**Domain:** Qt6/QML embedded music console kiosk (Raspberry Pi 5)
**Researched:** 2026-02-28
**Confidence:** HIGH (verified against reference implementation + official docs + community sources)

## Critical Pitfalls

These cause application freezes, audio corruption, or require significant rework.

### Pitfall 1: Blocking the Qt Event Loop with Network I/O

**What goes wrong:**
CD metadata fetching (MusicBrainz, GnuDB, Discogs), Spotify API calls, or DNS resolution runs synchronously on the main thread. The UI freezes for 2-30 seconds. In a kiosk with no keyboard/mouse, a frozen UI is irrecoverable from the user's perspective.

**Why it happens:**
Qt's `QNetworkAccessManager` is async by design, but developers commonly: (a) use `QProcess::waitForFinished()` for external tools (the reference implementation does this for `eject` and `ddcutil`), (b) call `QNetworkReply::waitForReadyRead()`, or (c) do synchronous DNS lookups. The reference implementation's CDMetadataFetcher correctly uses `QtConcurrent` for disc ID calculation and async signals for network replies, but the eject command in `EiscpReceiverController::ejectCD()` uses `process.waitForFinished(5000)` which blocks the GUI thread for up to 5 seconds.

**How to avoid:**
- Never call any `waitFor*()` method on the main thread. Period.
- Use `QProcess::finished` signal instead of `waitForFinished()`.
- Use `QNetworkAccessManager` with signals, never blocking reads.
- Wrap disc ID calculation (which opens the CD drive, a blocking operation) in `QtConcurrent::run()` as the reference implementation does.
- Set `QNetworkRequest::setTransferTimeout()` on all HTTP requests (the reference implementation correctly does this at 10s for Spotify and 3s for session checks).
- For DNS resolution, Qt resolves asynchronously through `QNetworkAccessManager`, but `QHostInfo::fromName()` blocks -- never use it.

**Warning signs:**
- Any `waitFor` call in code review
- Any `QThread::sleep` or `std::this_thread::sleep_for` on the main thread
- `QProcess` used without signal/slot for completion
- UI stops responding during network requests

**Phase to address:**
Foundation/Core phase -- establish the async pattern in the composition root. Every new subsystem must follow it from the start.

---

### Pitfall 2: ALSA S/PDIF EIO Errors and Stale Device Handles

**What goes wrong:**
The S/PDIF output device (`hw:2,0` on the Nvdigi HAT) returns `-EIO` when the ALSA device becomes "stale" -- typically after the system has been idle or after a USB reconfiguration. Unlike XRUN (`-EPIPE`), EIO cannot be recovered with `snd_pcm_recover()` -- the handle is permanently broken. The application continues trying to write to a dead handle, producing silence indefinitely.

**Why it happens:**
S/PDIF digital output has no analog fallback. If the ALSA driver loses sync with the HAT, subsequent writes return EIO. Standard ALSA recovery (`snd_pcm_prepare`) does not fix this. The reference implementation discovered this the hard way and added a close-reopen recovery loop with exponential backoff (500ms, 1000ms, 2000ms) using non-blocking open to prevent thread hang.

**How to avoid:**
- Handle `-EIO` separately from `-EPIPE`. EIO requires closing the handle entirely and reopening with fresh configuration.
- Use non-blocking open (`SND_PCM_NONBLOCK`) during recovery to prevent the thread from hanging on a wedged driver, then switch to blocking mode for normal writes.
- Cache the ALSA config (`AlsaOutputConfig`) so the recovery path can reconfigure without re-reading settings.
- Implement maximum retry attempts (3 is proven in the reference implementation) with exponential backoff.
- Emit an `audioRecoveryFailed` signal to the UI so the kiosk can show an error state rather than silent failure.

**Warning signs:**
- ALSA write calls returning -EIO instead of positive frame count
- Audio stops but playback state still shows "Playing"
- Recovery attempts logging "open failed" repeatedly

**Phase to address:**
Audio/Playback phase -- this must be built into `AlsaOutput` from the first implementation. The reference implementation's pattern at `AlsaOutput.cpp:104-148` is battle-tested and should be lifted directly.

---

### Pitfall 3: ALSA Sample Rate Mismatch on S/PDIF Output

**What goes wrong:**
`snd_pcm_hw_params_set_rate_near()` silently negotiates a different sample rate than requested. On analog output this causes pitch shift. On S/PDIF output, it produces corrupted digital audio that the receiver silently rejects -- no sound at all, with no error indication.

**Why it happens:**
The `_near` family of ALSA functions are designed to negotiate the closest supported value. The S/PDIF HAT may support limited rates. If you request 44100Hz but the driver negotiates 48000Hz, the ALSA API reports success even though the audio will be corrupt for CD audio (which is strictly 44100Hz).

**How to avoid:**
- After `snd_pcm_hw_params()`, read back the actual negotiated rate with `snd_pcm_hw_params_get_rate()`.
- If `actualRate != requestedRate`, fail the open immediately with a critical log message. Do not proceed.
- The reference implementation has this exact check at `AlsaOutput.cpp:219-227` (commented as "ALSA-03: Strict sample rate verification for S/PDIF output").
- Also disable ALSA's built-in resampler: `snd_pcm_hw_params_set_rate_resample(handle, hwParams, 0)`.

**Warning signs:**
- CD plays but no sound comes from the receiver
- ALSA device opens successfully but receiver shows no S/PDIF lock
- Log shows adjusted rate different from 44100

**Phase to address:**
Audio/Playback phase -- this validation must be part of the ALSA device configuration, not an afterthought.

---

### Pitfall 4: GPIO Mute Button Double-Toggle (Both-Edge Firing)

**What goes wrong:**
The volume encoder's push switch generates events on both rising and falling edges. If the mute toggle handler fires on both edges, mute is toggled ON then immediately OFF (or vice versa), making the button appear non-functional.

**Why it happens:**
`libgpiod` with `GPIOD_LINE_EDGE_BOTH` for the encoder rotation lines also captures both edges on the switch line. The reference implementation handles this by: (a) only acting on falling edge (press, not release) for the switch, and (b) implementing a 250ms debounce timer in the GPIO thread plus a secondary debounce timer (`SWITCH_DEBOUNCE_MS`) on the main thread.

**How to avoid:**
- For push switches: filter to one edge only (falling edge for active-low switches with pull-up).
- Implement debouncing at the GPIO event level (in the monitoring thread) with a minimum interval (250ms works in the reference implementation).
- Optionally add a second debounce layer on the Qt main thread using `QTimer::singleShot`.
- Document which edge triggers the action in code comments.

**Warning signs:**
- Mute toggles twice per button press (effectively no-op)
- Intermittent mute behavior that "sometimes works"
- Rapid duplicate switch events in GPIO log

**Phase to address:**
GPIO/Hardware phase -- this is a day-one requirement for the encoder monitor.

---

### Pitfall 5: Volume Encoder Event Storm from Detentless Encoder

**What goes wrong:**
The PEC11R-4020F-S0024 volume encoder is smooth (no detents) and generates a continuous stream of rotation events during a gesture. If each event immediately sends an eISCP volume command to the receiver, the receiver is flooded with commands. The receiver either drops commands, introduces lag, or responds with volume oscillation as confirmed and pending values fight.

**Why it happens:**
A single finger rotation can generate 10-50 encoder events in under a second. The receiver's eISCP protocol processes commands sequentially over TCP with noticeable round-trip time. Sending every event as a separate `MVL` command creates a backlog.

**How to avoid:**
- Treat encoder input as a gesture, not individual steps. Accumulate rotation events during the gesture.
- Use a commit timer (the reference implementation uses 100ms `VOLUME_COMMIT_DELAY_MS`) -- each encoder event resets the timer. Only when the timer expires (user stopped turning) does the application send a single `MVL` command to the receiver.
- Track a `targetVolume` locally for immediate UI feedback while the actual command is pending.
- When the receiver confirms the volume (via `MVL` response), reconcile with the target.
- Never show the volume overlay for unsolicited receiver volume changes (e.g., Spotify sets volume to 40).

**Warning signs:**
- Volume jumps erratically when turning the knob
- Volume overlay flickers
- Multiple `MVL` commands logged per second during rotation
- Volume "bounces" between values after encoder stops

**Phase to address:**
Receiver Control phase -- the volume commit timer pattern must be implemented alongside eISCP volume commands.

---

### Pitfall 6: God Object AppState Accumulating All Responsibilities

**What goes wrong:**
A single `AppState` class becomes the mediator for every subsystem: receiver, CD, FLAC, Spotify, GPIO, display, library. It grows to 1,400+ lines (as in the reference implementation), with tightly coupled signal routing, mixed business logic, and untestable monolithic state.

**Why it happens:**
QML naturally binds to a single root context object. It is tempting to put every property and Q_INVOKABLE on one class. Over time, each new feature adds more properties, more slots, more private state. The class becomes the only coordination point.

**How to avoid:**
- Decompose into focused state objects exposed as separate context properties: `receiverState`, `playbackState`, `spotifyState`, `displayState`, `libraryState`.
- AppState becomes a thin coordinator that holds references but delegates all logic.
- Use a composition root (`AppBuilder`) that wires dependencies with clear ownership.
- Each subsystem should be independently testable by injecting mock dependencies.
- QML accesses subsystem properties through their own context objects, not a single god object.

**Warning signs:**
- AppState exceeds 300 lines
- Adding a feature requires modifying AppState
- Test setup requires constructing the entire AppState with all dependencies
- Private member count exceeds 15

**Phase to address:**
Foundation/Architecture phase -- this is the PRIMARY architectural decision of the rewrite. Get it right before any feature work.

---

### Pitfall 7: CD Metadata Fetch Freezing the Application

**What goes wrong:**
Computing the disc ID requires opening the CD drive and reading the TOC -- a blocking I/O operation that can take 1-5 seconds depending on drive spin-up. If done on the main thread, the UI freezes. The subsequent MusicBrainz/GnuDB/Discogs network lookups add another 2-15 seconds.

**Why it happens:**
`libcdio` functions like `cdio_open()`, `cdio_get_first_track_num()`, etc. are synchronous and involve physical hardware I/O. The drive may need to spin up from idle state.

**How to avoid:**
- Compute disc ID in a background thread using `QtConcurrent::run()` with `QFutureWatcher` (proven pattern from the reference implementation).
- Show TOC data (track count, durations from sector math) immediately while metadata fetches run async.
- Implement progressive metadata display: show "Reading Disc..." immediately, then "Audio CD" with track count from TOC, then fill in artist/album/title as metadata arrives.
- Cache metadata in SQLite by disc ID so repeat insertions are instant.
- Validate GnuDB responses before caching -- GnuDB returns malformed data frequently.

**Warning signs:**
- UI shows "Reading Disc..." for more than 1 second
- `cdio_` calls appearing outside a worker thread
- Missing cache hit logs on repeat disc insertions
- GnuDB cache returning garbage data

**Phase to address:**
CD Playback phase -- the async metadata pipeline is as important as the audio streaming itself.

---

## Moderate Pitfalls

### Pitfall 8: QSqlDatabase Thread Affinity Violations

**What goes wrong:**
A `QSqlDatabase` connection created on one thread is used from another thread. Qt silently corrupts data or crashes with "requested database does not belong to the calling thread."

**Why it happens:**
The CD metadata cache, library database, and library scanner all use SQLite. The scanner runs on a background thread (via `QtConcurrent`), while the main thread reads the database for UI models. If they share a connection, or if a connection is created on one thread and queries run on another, undefined behavior results.

**How to avoid:**
- Create one `QSqlDatabase` connection per thread, each with a unique connection name.
- Use named connections (`QSqlDatabase::addDatabase("QSQLITE", "connection-name")`) -- never use the default connection.
- The reference implementation correctly uses separate named connections: `"cd_metadata_cache"` for CDMetadataCache and `"library"` for LibraryDatabase.
- Enable WAL mode (`PRAGMA journal_mode=WAL`) for each database -- this allows concurrent reads from multiple connections while one connection writes.
- Never pass `QSqlQuery` or `QSqlDatabase` objects across thread boundaries.

**Warning signs:**
- "requested database does not belong to the calling thread" warning in logs
- "database is locked" errors during concurrent access
- Intermittent crashes during library scan with CD metadata lookup

**Phase to address:**
Data layer phase -- establish database connection patterns before any feature uses them.

---

### Pitfall 9: Spotify OAuth Token Expiry and Silent Authentication Loss

**What goes wrong:**
The Spotify access token expires (1-hour lifetime). If the refresh fails silently or the refresh token is invalidated, the application enters a state where it appears authenticated but all API calls fail with 401. The kiosk has no way to re-authenticate without a web browser.

**Why it happens:**
Token refresh can fail due to: network errors, Spotify revoking the refresh token, or the application not scheduling refresh before expiry. The Spotify API also changed its OAuth requirements in November 2025 -- implicit grant flow and HTTP redirect URIs are no longer supported.

**How to avoid:**
- Schedule token refresh 5 minutes before expiry (the reference implementation does this correctly in `scheduleTokenRefresh()`).
- If refresh fails, immediately set `m_authenticated = false` and emit a signal so the UI can show "Re-authentication needed."
- Persist both access token and refresh token via `QSettings` so they survive application restarts.
- Use Authorization Code Flow (not implicit grant, which was deprecated November 2025).
- Set `QNetworkRequest::setTransferTimeout()` on token requests to prevent hanging.
- The kiosk's HTTP API server must provide an OAuth callback endpoint for re-authentication from a phone/laptop browser.

**Warning signs:**
- Spotify controls stop working after ~1 hour of operation
- 401 errors in Spotify API logs
- Token refresh timer not rescheduling after a successful refresh
- Missing refresh token in persisted settings

**Phase to address:**
Spotify Integration phase -- token lifecycle must be robust from first implementation.

---

### Pitfall 10: ddcutil Command Serialization and I2C Bus Contention

**What goes wrong:**
Multiple concurrent `ddcutil` processes fight over the I2C bus, causing DDC/CI commands to fail or corrupt. The display control becomes unreliable -- brightness commands are dropped, or the display doesn't power on/off correctly.

**Why it happens:**
The reference implementation spawns `QProcess` instances for each `ddcutil` command (set brightness, set power). During a fade animation (50ms timer updating brightness), multiple processes can overlap. The reference implementation partially addresses this with `m_brightnessChangeInProgress` flag, but a rapid dim-then-off sequence can still overlap.

**How to avoid:**
- Serialize all `ddcutil` commands through a single command queue.
- Check if a command is in progress before spawning a new process (the flag pattern in the reference implementation is a starting point but needs to be a proper queue).
- Use `--sleep-multiplier` option with `ddcutil` if the display has timing issues.
- Auto-detect the display I2C bus on startup (`ddcutil detect`) rather than hardcoding.
- On Raspberry Pi 5, there's a known issue where `setvcp` may not correctly update values -- verify with `getvcp` after setting critical values like power.

**Warning signs:**
- `ddcutil` returning non-zero exit codes intermittently
- Brightness changes being "skipped" in fade animations
- Display not responding to power on/off commands
- I2C bus errors in system logs

**Phase to address:**
Display Control phase -- the command queue is essential infrastructure for reliable DDC/CI.

---

### Pitfall 11: eISCP Metadata Fields Arriving Non-Atomically

**What goes wrong:**
When the receiver changes tracks (especially on Spotify via the receiver's network input), metadata fields (NTI/title, NAT/artist, NAL/album, NJA/art) arrive as separate TCP messages over 100-500ms. If the UI updates on each individual field, the user sees "New Artist" with the old song title briefly, creating a jarring flash.

**Why it happens:**
The eISCP protocol sends one field per response packet. There is no atomic "track changed" message. The receiver responds to polling queries individually, and each response triggers a signal that updates the UI.

**How to avoid:**
- Buffer incoming metadata changes with a short accumulation timer (500-1000ms).
- Only emit a combined `trackInfoChanged` signal after either: all expected fields have arrived, or the buffer timer expires (whichever comes first).
- The reference implementation's `LidarrController` uses a similar buffer pattern (`LIDARR_BUFFER_TIMEOUT_MS = 2000`) for accumulating partial track data.
- For the Spotify/Bluetooth source, the receiver's metadata is the canonical source -- don't mix it with Spotify API metadata.

**Warning signs:**
- Track title flashes between old and new values during transitions
- Artist shows "Unknown" briefly when track changes
- Album art from previous track displays with new track's metadata

**Phase to address:**
Receiver Control phase -- metadata buffering is part of the eISCP response handling.

---

### Pitfall 12: CD Audio Ring Buffer Thread Safety

**What goes wrong:**
The CD audio streaming uses a ring buffer shared between a reader thread (reading from disc) and the playback thread (writing to ALSA). Without proper synchronization, the reader overwrites data the playback thread hasn't consumed yet, causing audio corruption, or the playback thread reads partially written data.

**Why it happens:**
Ring buffers with separate read/write indices seem "lock-free" but require careful synchronization for the available-frames count and for seek operations that reset the buffer state.

**How to avoid:**
- Use a mutex to protect buffer state (read index, write index, frames available). The reference implementation uses `std::mutex` with `std::lock_guard` and `std::unique_lock` for condition variable waits.
- Implement condition variables for flow control: the reader waits when the buffer is full (`m_canWrite`), and signals the consumer when data is available (`m_prefillCv`).
- On seek, acquire the lock, reset both indices and the available count, set a pending-seek flag, and notify the reader.
- The prefill mechanism (wait until N seconds of audio are buffered before playback starts) prevents initial underruns.

**Warning signs:**
- Audio glitches at track boundaries or during seeks
- `underrunCount` increasing steadily in buffer stats
- Seek operations causing brief noise or silence gaps
- Buffer "LOW" warnings after prefill is complete

**Phase to address:**
Audio/Playback phase -- the ring buffer is core infrastructure that must be correct from day one.

---

## Minor Pitfalls

### Pitfall 13: QML Binding Loops on Layout Properties

**What goes wrong:**
QML emits "Binding loop detected for property 'width'" or similar warnings. In mild cases, this causes extra layout passes and wastes CPU. In severe cases, the layout oscillates and never stabilizes, causing visual jitter and high CPU on the Pi.

**How to avoid:**
- Avoid circular dependencies in property bindings (e.g., an item's width depending on its parent's width which depends on the item's width).
- Use `implicitWidth`/`implicitHeight` for intrinsic sizes, `width`/`height` for explicit sizes -- don't mix them circularly.
- Use `onWidthChanged` handlers instead of bindings when the relationship is one-directional.
- Run the QML Profiler in Qt Creator to detect binding loop hotspots.

**Phase to address:**
UI phase -- catch during QML development, enforce zero binding loop warnings.

---

### Pitfall 14: QML Image Memory Leak on Long-Running Kiosk

**What goes wrong:**
Album art images loaded via QML `Image` components accumulate in the Quick pixmap cache. Over hours or days of continuous operation, memory usage grows unbounded until the Pi's 4GB RAM is exhausted and the OOM killer terminates the application.

**How to avoid:**
- Set `Image { cache: false }` for album art that changes frequently.
- Use `Image { asynchronous: true }` for network-loaded images.
- Set `QML_DISK_CACHE_PATH` and configure a reasonable `QQmlEngine::trimComponentCache()` interval.
- Call `gc()` from QML periodically if garbage collection is not running often enough.
- Monitor RSS with a periodic health check timer and log warnings if memory exceeds thresholds.
- Qt 6.7+ has fixes for known Quick pixmap cache leaks -- ensure the deployed Qt version includes these fixes.

**Phase to address:**
UI phase + Production deployment phase -- test with extended runtime (24+ hours).

---

### Pitfall 15: libcdio/paranoia USB Drive Speed Negotiation Failure

**What goes wrong:**
`cdio_cddap_speed_set()` returns error code -405 on some USB drives that claim to support speed setting but do not. The drive may oscillate between spin-up and spin-down states, causing read timeouts and choppy audio.

**How to avoid:**
- Call `cdio_cddap_speed_set(drive, 4)` but ignore the return value if it's -405 (the reference implementation already does this).
- Set `PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP` for the paranoia mode -- this enables error correction but allows skipping sectors after max retries rather than blocking indefinitely.
- Use `cdio_paranoia_read_limited()` with a reasonable retry count (3 retries in the reference implementation) rather than `cdio_paranoia_read()` which retries infinitely.
- Log sector read latency statistics to detect drives that are underperforming.

**Phase to address:**
CD Playback phase -- configure during paranoia backend initialization.

---

### Pitfall 16: CMake Cross-Compilation Using Target moc/rcc Instead of Host

**What goes wrong:**
When cross-compiling Qt6 for ARM (Raspberry Pi), CMake accidentally uses the ARM-compiled `moc` and `rcc` tools instead of the host (x86) versions. The build fails with cryptic errors or produces a broken binary.

**How to avoid:**
- Cross-compilation requires a host Qt build first to provide native `moc`, `rcc`, and other tools.
- Use the Qt-provided toolchain file and set `QT_HOST_PATH` correctly in the CMake toolchain configuration.
- If building natively on the Pi (which is reasonable for Pi 5 with 4GB RAM), this pitfall does not apply.
- Do not install `libzstd-dev` on the build system if cross-compiling -- Qt6 will link against it and `rcc` will fail with "Bus Error" on the target.

**Phase to address:**
Build System phase -- validate the cross-compilation toolchain (or decide to build natively) before any feature work.

---

### Pitfall 17: Google Test Cannot Parse Qt MOC Macros

**What goes wrong:**
Google Mock's `MOCK_METHOD` macro cannot be used directly in a class that is also processed by Qt's `moc` (i.e., a class with `Q_OBJECT`). The `moc` tool fails to parse the Google Mock macros, producing build errors.

**How to avoid:**
- Define abstract interfaces as pure C++ classes (no `Q_OBJECT` macro) with virtual methods.
- Create Google Mock implementations of these interfaces for testing.
- For testing signal emissions, use `QSignalSpy` from `QTest` rather than trying to mock signals.
- Structure CMake to have separate test targets that link against Google Test and the application's library target (not the executable).
- Use `gtest_discover_tests()` with `DISCOVERY_MODE PRE_TEST` to avoid PATH issues when Qt libraries are not in the test environment's library path.

**Phase to address:**
Testing Infrastructure phase -- establish the interface/mock pattern in the foundation.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| `#ifdef __linux__` guards everywhere | Compiles on macOS for development | Duplicate stub classes, untestable platform code, hard to maintain | Never -- use runtime injection with platform abstraction interfaces instead |
| Scattered `QSettings::value()` reads | Quick configuration access | Settings read from random locations, no validation, key name typos go undetected | Never -- use centralized `AppConfig` struct populated once at startup |
| `static` local variables in loops | Quick state persistence | Thread-unsafe, invisible to callers, hard to test | Never (the reference implementation has `static int silenceCheckCounter` and `static size_t framesAccumulator` in the playback loop) |
| Hardcoded device paths (`hw:2,0`, `/dev/sr0`) | Works on target hardware | Breaks on different hardware configs, untestable on dev machines | Only in default config values, always overridable via settings |
| QML accessing C++ singletons directly | Less boilerplate | Tight coupling, hard to test, hard to swap implementations | Never -- use context properties or dependency injection |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| eISCP (Onkyo receiver) | Parsing `MVL` volume as decimal instead of hex | Volume is hex: `MVL1A` = 26 decimal. Use `hexVal.toInt(&ok, 16)`. |
| eISCP metadata | Using `toLatin1()` for metadata strings | Receiver sends UTF-8 for accented characters. Use `QString::fromUtf8()`. |
| eISCP polling | Querying `NSVQSTN` during playback | This query changes the receiver's internal menu state and interferes with playback. Never query it. |
| MusicBrainz API | Not setting a User-Agent header | MusicBrainz rate-limits requests without a proper User-Agent. Set `"MediaConsole/2.0 (contact@email)"`. |
| GnuDB | Trusting response data without validation | GnuDB returns malformed responses frequently. Validate track count, disc ID match, and non-empty titles before caching. |
| Spotify API | Automatic play retry/transfer on 404 | If play fails with 404 (device inactive) or 403 (device controlled elsewhere), do NOT auto-retry or force-transfer -- this creates a tug-of-war with other devices. Log and fail gracefully. |
| Spotify Connect | Sending volume with `device_id` parameter | Queue requests should NOT include `device_id` -- they target whatever device is currently active, which is correct behavior when a phone has taken over. |
| SQLite via Qt | Using the default connection for multiple databases | Qt's `QSqlDatabase::database()` returns the default connection. If you have both CD cache and library databases, you must use named connections or they overwrite each other. |
| ALSA S/PDIF | Using `snd_pcm_recover()` for EIO errors | `snd_pcm_recover()` only handles EPIPE (underrun) and ESTRPIPE (suspend). EIO requires a full close-reopen cycle. |
| libgpiod v2 | Using v1 API function names | libgpiod v2 on Raspberry Pi 5 has a completely different API. Functions like `gpiod_chip_get_line` no longer exist. Use `gpiod_chip_request_lines` with `gpiod_line_config`. |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| QML property binding reevaluation cascade | High CPU, laggy scrolling, delayed touch response | Minimize binding chains, use `Connections` instead of inline bindings for complex logic | Noticeable with >20 active bindings on visible items |
| Album art image not using `asynchronous: true` | UI stutter when changing tracks or scrolling library | Always set `asynchronous: true` on `Image` components loading external URLs | Noticeable on first load of uncached images |
| ALSA buffer too small for S/PDIF | Frequent XRUN warnings, audio clicks/pops | Use period size >= 512 frames and buffer >= 4x period (reference: period=512, buffer=4096 at 44100Hz) | Under any CPU load spike (e.g., during GC or metadata fetch) |
| Polling receiver status too frequently | Network congestion on TCP socket, receiver unresponsive | Poll every 2.5 seconds (reference implementation value), not faster | If polling interval < 1 second |
| CD read blocking ALSA write | Audio underrun during disc read stalls (scratched CD) | Size the ring buffer for >= 3 seconds of audio (reference: configurable `bufferSeconds`, default sufficient) | When a scratched sector causes a >1 second read pause |
| `ddcutil` fade animation at 50ms intervals | CPU spent spawning processes, I2C bus saturated | Increase fade step interval to 100-200ms, limit to ~10 brightness steps per fade | On any fade animation |

## Security Considerations

| Concern | Risk | Prevention |
|---------|------|------------|
| Spotify client secret in QSettings (plaintext) | Credential exposure if filesystem is accessed | Use file permissions (chmod 600 on config file), or better: store in a separate credentials file outside the app data dir |
| HTTP API server without authentication | Anyone on the network can control the console | Bind to localhost only, or implement simple token auth for the API endpoints |
| Spotify OAuth redirect URI on HTTP | Token interception in transit | Use HTTPS for the redirect URI (required by Spotify after November 2025) |
| ddcutil runs as subprocess | Command injection if display ID comes from user input | Always validate and sanitize `--bus` parameter value as integer |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Showing "Loading..." for more than 2 seconds without progress indication | User thinks kiosk is frozen, starts tapping randomly | Show immediate placeholder content (track count, disc icon) while metadata loads in background |
| Volume overlay appearing on unsolicited receiver volume changes | Distracting overlay when Spotify adjusts volume to 40 | Only show overlay for user-initiated volume changes (encoder turn, touchscreen drag) |
| CD eject blocking the UI thread | 5-second freeze when pressing eject button | Run eject command async with QProcess signals |
| No visual feedback during CD drive spin-up | User presses play, nothing happens for 2-3 seconds | Show a "Buffering..." state or spinner during the prefill wait |
| Displaying "Unknown (FF)" briefly when switching streaming service | Jarring flash of technical internal state | Cache the last known service name and display it until the new service is confirmed |

## "Looks Done But Isn't" Checklist

- [ ] **ALSA playback:** Often missing EIO recovery -- verify ALSA write handles -EIO with close-reopen, not just snd_pcm_recover
- [ ] **ALSA playback:** Often missing sample rate verification -- verify actual negotiated rate matches requested rate for S/PDIF
- [ ] **CD metadata:** Often missing cache validation -- verify cached GnuDB data has non-empty titles and matching disc ID before displaying
- [ ] **CD metadata:** Often missing progressive display -- verify TOC displays immediately, not after network lookup completes
- [ ] **Volume encoder:** Often missing gesture coalescing -- verify only one eISCP command is sent per rotation gesture, not per encoder tick
- [ ] **Mute button:** Often missing edge filtering -- verify only one edge (falling) triggers the mute toggle
- [ ] **Spotify auth:** Often missing token refresh scheduling -- verify timer is set for 5 minutes before expiry after every successful token refresh
- [ ] **eISCP reconnect:** Often missing auto-reconnect -- verify receiver reconnects automatically after network disruption
- [ ] **Display timeout:** Often missing music-playing inhibit -- verify screen doesn't dim/off while music is actively playing
- [ ] **SQLite:** Often missing WAL mode -- verify both CD cache and library database enable WAL pragma
- [ ] **QML images:** Often missing cache:false for dynamic content -- verify album art Image components don't leak memory over 24+ hours

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| ALSA EIO stale handle | LOW | Close handle, exponential backoff reopen (3 attempts), reconfigure, resume playback |
| UI frozen from blocking I/O | MEDIUM | Kill process, fix async pattern, redeploy. No data loss but service interruption. |
| God object AppState | HIGH | Incremental extraction of subsystems into focused classes. Requires touching every QML binding. Plan for a full sprint. |
| Spotify token permanently revoked | LOW | User re-authenticates via HTTP API's `/setup/spotify` endpoint on phone browser |
| SQLite database corruption | LOW | Delete the database file, rebuild from scan (library) or re-fetch on next disc insert (CD cache) |
| QML memory leak over days | MEDIUM | Restart the application via systemd watchdog. Long-term fix: upgrade Qt, add cache management. |
| libgpiod v1 API used on Pi 5 | HIGH | Complete rewrite of GPIO monitor code. v1 and v2 APIs are incompatible. |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Event loop blocking (P1) | Foundation | Zero `waitFor*` calls in code review; async patterns enforced in interface contracts |
| ALSA EIO recovery (P2) | Audio/Playback | Test: unplug/replug S/PDIF cable during playback, verify recovery within 5 seconds |
| ALSA rate mismatch (P3) | Audio/Playback | Test: request 48000Hz on a 44100-only device, verify open fails with error log |
| GPIO double-toggle (P4) | GPIO/Hardware | Test: press mute button 20 times rapidly, verify exactly 20 mute toggles |
| Volume event storm (P5) | Receiver Control | Test: rapid encoder rotation, verify <= 1 eISCP command per 100ms window |
| God object (P6) | Foundation/Architecture | AppState < 300 lines, each subsystem independently testable |
| CD metadata freeze (P7) | CD Playback | Test: insert CD with no network, verify UI remains responsive |
| SQLite thread safety (P8) | Data Layer | Test: concurrent scan + metadata lookup, verify no thread warnings |
| Spotify token expiry (P9) | Spotify Integration | Test: fast-forward system clock 2 hours, verify automatic re-authentication |
| ddcutil serialization (P10) | Display Control | Test: rapid dim/brighten commands, verify no ddcutil errors |
| eISCP metadata atomicity (P11) | Receiver Control | Test: track change on Spotify, verify no partial metadata flash |
| Ring buffer safety (P12) | Audio/Playback | Test: seek during playback, verify no audio glitches or crashes |
| QML binding loops (P13) | UI | Zero "Binding loop detected" messages in application log |
| QML memory leak (P14) | UI + Deployment | 24-hour soak test, RSS delta < 50MB |
| libcdio speed (P15) | CD Playback | Log confirms speed_set called, no drive oscillation in audio quality |
| CMake cross-compile (P16) | Build System | Clean cross-build succeeds, binary runs on Pi 5 |
| GTest + Qt moc (P17) | Testing Infrastructure | All mock classes compile, tests pass, moc processes test helpers |

## Sources

- Reference implementation analysis: `~/Code/media-console/src/` (all subsystems examined directly)
  - AlsaOutput.cpp: EIO recovery pattern, sample rate verification, xrun handling
  - CdAudioStream.cpp: Ring buffer implementation, prefill mechanism, thread priority
  - CdPlaybackController.cpp: Playback loop, seek handling, spin-up timing
  - EiscpReceiverController.cpp: eISCP protocol parsing, metadata handling, reconnection
  - VolumeEncoderMonitor.cpp: libgpiod v2 API usage, quadrature decoding, debouncing
  - DisplayControl.cpp: ddcutil command execution, fade animation, bus detection
  - SpotifyAuthManager.cpp: OAuth flow, token refresh, device discovery, play request handling
  - CDMetadataFetcher.cpp: Async disc ID calculation, MusicBrainz/GnuDB/Discogs lookup chain
  - CDMetadataCache.cpp: SQLite WAL mode, named connections, transaction handling
  - CDMonitor.cpp: ioctl-based disc detection, debounce state machine
  - ScreenTimeoutManager.cpp: State machine for dim/off/door, music-playing inhibit
- [Qt6 Performance Considerations (official)](https://doc.qt.io/qt-6/qtquick-performance.html) -- MEDIUM confidence
- [ALSA Xrun Documentation](https://alsa.opensrc.org/Xruns) -- HIGH confidence
- [Qt QSqlDatabase Thread Safety (official)](https://doc.qt.io/qt-6/qsqldatabase.html) -- HIGH confidence
- [Spotify OAuth Migration (November 2025)](https://developer.spotify.com/blog/2025-10-14-reminder-oauth-migration-27-nov-2025) -- HIGH confidence
- [Spotify Rate Limits](https://developer.spotify.com/documentation/web-api/concepts/rate-limits) -- HIGH confidence
- [Spotify Token Refresh](https://developer.spotify.com/documentation/web-api/tutorials/refreshing-tokens) -- HIGH confidence
- [ddcutil Raspberry Pi Issues](https://github.com/rockowitz/ddcutil/issues/356) -- MEDIUM confidence
- [libgpiod v2 on Pi 5 (Raspberry Pi Forums)](https://forums.raspberrypi.com/viewtopic.php?t=394138) -- MEDIUM confidence
- [Raspberry Pi GPIO White Paper](https://pip-assets.raspberrypi.com/categories/685-whitepapers-app-notes/documents/RP-006553-WP/A-history-of-GPIO-usage-on-Raspberry-Pi-devices-and-current-best-practices) -- HIGH confidence
- [libcdio-paranoia Drive Speed Issue](https://github.com/libcdio/libcdio-paranoia/issues/33) -- MEDIUM confidence
- [Qt6 Cross-Compile for RPi (Qt Wiki)](https://wiki.qt.io/Cross-Compile_Qt_6_for_Raspberry_Pi) -- HIGH confidence
- [Google Mock with Qt Slots (community)](https://groups.google.com/g/googlemock/c/RTgynKPa6ew) -- MEDIUM confidence
- [KDAB Binding Loops](https://www.kdab.com/binding-loops-video/) -- MEDIUM confidence
- [Multithreaded databases with QtSql](https://lnj.gitlab.io/post/multithreaded-databases-with-qtsql/) -- MEDIUM confidence
- [RPi 5 ALSA Issues](https://github.com/raspberrypi/linux/issues/6201) -- MEDIUM confidence

---
*Pitfalls research for: Qt6/QML embedded music console kiosk (Raspberry Pi 5)*
*Researched: 2026-02-28*
