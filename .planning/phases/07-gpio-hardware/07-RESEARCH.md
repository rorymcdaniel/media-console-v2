# Phase 7: GPIO Hardware - Research

**Researched:** 2026-02-28
**Domain:** libgpiod v2 C++ bindings, rotary encoder quadrature decoding, GPIO edge event monitoring on Raspberry Pi 5
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Volume encoder: PEC11R-4020F-S0024, 24 PPR, 0 detents, GPIO 27/22/23, +2 per detent, no acceleration, full range ~4.2 revolutions
- Input encoder: PEC11R-4320F-S0012, 12 PPR, 12 detents, GPIO 16/20/5, wrap-around cycling through 6 sources, 1 detent = 1 source
- Input encoder drives UI carousel with 4-second auto-select timeout (matches Phase 10 InputCarousel)
- GPIO layer emits navigation signals; carousel handles timeout and receiver command
- Push button context-dependent: during carousel browsing confirms source immediately (skips 4s timeout); when idle toggles mute
- Push button: falling edge trigger with 250ms debounce
- Reed switch: GPIO 17, updates UIState.doorOpen, 500ms debounce, magnets apart = door open (display on), magnets together = door closed (display off)
- Reed switch reads pin value immediately on start() -- no ambiguous initial state
- Mute button: falling edge only (fixes double-toggle bug GPIO-06), 250ms debounce
- All GPIO monitors via libgpiod v2 on /dev/gpiochip4
- Linux-only with no-op stubs on other platforms

### Claude's Discretion
- Signal design: whether to keep separate volumeUp()/volumeDown() or change to single volumeChanged(int delta)
- Whether GPIO monitor emits raw detents (+1/-1) or pre-scaled values (+2/-2)
- Interface updates to IGpioMonitor and StubGpioMonitor as needed
- Quadrature decoding implementation details
- Background thread architecture for libgpiod v2 event monitoring
- Error handling when libgpiod fails to open chip

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| GPIO-01 | Volume rotary encoder monitor (PEC11R-4020F-S0024): GPIO 27/22/23, quadrature decoding via libgpiod v2 API, gesture-based coalescing | libgpiod v2 C++ API for edge monitoring on multiple lines; quadrature state machine algorithm; VolumeGestureController.onEncoderTick(int delta) integration |
| GPIO-02 | Input rotary encoder monitor (PEC11R-4320F-S0012): GPIO 16/20/5, quadrature decoding, 250ms switch debounce, 1:1 detent-to-input mapping | Same quadrature decoding; libgpiod hardware debounce for push button; existing IGpioMonitor inputNext/inputPrevious signals |
| GPIO-03 | Reed switch monitor: GPIO 17, 500ms debounce, magnets apart = display on, together = off | libgpiod hardware debounce (500ms); single line edge monitoring; UIState needs doorOpen Q_PROPERTY |
| GPIO-04 | All monitors run in background threads via libgpiod v2 event monitoring on /dev/gpiochip4 | libgpiod line_request.fd() + poll() for async blocking; QThread pattern established in LocalPlaybackController |
| GPIO-05 | Linux-only with no-op stubs on other platforms (via IGpioMonitor interface) | PlatformFactory pattern with HAS_GPIOD compile flag; StubGpioMonitor already complete |
| GPIO-06 | Mute button triggers on ONE edge only (falling or rising, not both) -- fixes known double-toggle bug | libgpiod line::edge::FALLING for single-edge detection; 250ms debounce via set_debounce_period |
</phase_requirements>

## Summary

This phase implements the real GPIO hardware layer for the media console, connecting physical rotary encoders and a reed switch to the application via the libgpiod v2 C++ API on a Raspberry Pi 5. The core work is: (1) a `LinuxGpioMonitor` class implementing `IGpioMonitor` that runs a background thread monitoring GPIO edge events via `poll()` on a libgpiod file descriptor, (2) quadrature decoding of two rotary encoders to determine rotation direction, and (3) debounced switch/reed-switch handling. The existing `StubGpioMonitor` and `IGpioMonitor` interface provide the scaffolding; the real implementation slots into `PlatformFactory::createGpioMonitor()`.

The libgpiod v2 C++ API (version 2.2.1 on Raspberry Pi OS Trixie) provides a clean RAII-based interface: open a `gpiod::chip`, configure `gpiod::line_settings` with direction, edge detection, bias, and debounce, then call `prepare_request().add_line_settings().do_request()` to get a `gpiod::line_request`. The request exposes an `fd()` for `poll()`-based async waiting plus `read_edge_events()` and `get_value()` methods. All lines can be requested in a single request, meaning one thread can monitor all GPIO pins. Hardware debounce is supported directly in the kernel (debounce_period in microseconds), which handles the push button 250ms debounce and reed switch 500ms debounce without software timers.

The two PEC11R encoders require quadrature decoding: the volume encoder (24 PPR, 0 detents) generates 24 quadrature state transitions per revolution, and the input encoder (12 PPR, 12 detents) generates 12 transitions per revolution with 1 pulse per detent. A simple state-machine approach using the two-channel Gray code (A xor B) reliably determines CW vs CCW rotation. The volume encoder maps each decoded step to a +2/-2 delta fed to `VolumeGestureController::onEncoderTick()`. The input encoder maps each decoded step to an `inputNext`/`inputPrevious` signal.

**Primary recommendation:** Use the libgpiod v2 C++ bindings with a single `gpiod::line_request` covering all 7 GPIO lines (3 volume encoder + 3 input encoder + 1 reed switch), monitored from a single QThread using `poll()` on the request's file descriptor, with hardware debounce for switches and a quadrature state machine for encoders.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libgpiod (C++) | 2.2.1 | GPIO chip access, line requests, edge event monitoring | Official Linux GPIO userspace API; replaces deprecated sysfs; only option for Pi 5 RP1 GPIO |
| Qt6 Core | 6.8.2 | QThread, QObject, signals/slots, QTimer | Project standard; thread-safe cross-thread signal emission |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| libgpiod (C) | 2.2.1 | Underlying C library linked by C++ bindings | Automatically linked as dependency |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| libgpiod C++ bindings | libgpiod C API | C++ bindings provide RAII, type safety, and cleaner code; C API is lower-level but functionally identical |
| libgpiod | lgpio | lgpio is also viable on Pi 5 but libgpiod is the kernel-blessed standard; lgpio is a third-party library |
| libgpiod | wiringPi/RPi.GPIO | Neither works on Pi 5 due to RP1 memory-mapped GPIO changes |

**Installation (on Raspberry Pi OS Trixie):**
```bash
sudo apt install libgpiod-dev
```

**CMake integration:**
```cmake
pkg_check_modules(GPIOD IMPORTED_TARGET libgpiodcxx)
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── platform/
│   ├── IGpioMonitor.h           # Interface (already exists)
│   ├── LinuxGpioMonitor.h       # Real implementation (NEW)
│   ├── LinuxGpioMonitor.cpp     # Real implementation (NEW)
│   ├── PlatformFactory.h        # Factory (update for HAS_GPIOD)
│   ├── PlatformFactory.cpp      # Factory (update for HAS_GPIOD)
│   └── stubs/
│       ├── StubGpioMonitor.h    # Stub (already exists)
│       └── StubGpioMonitor.cpp  # Stub (already exists)
├── state/
│   └── UIState.h                # Add doorOpen Q_PROPERTY
├── app/
│   ├── AppBuilder.cpp           # Wire GPIO signals
│   └── AppConfig.h              # Add GpioConfig (optional)
```

### Pattern 1: Single-Request Multi-Line Monitoring
**What:** Request all 7 GPIO lines in a single `gpiod::line_request` with per-line settings, then monitor the single file descriptor with `poll()`.
**When to use:** Always -- libgpiod v2 supports per-line configuration within a single request. This is more efficient than separate requests and simpler (one fd, one thread).
**Example:**
```cpp
// Source: libgpiod v2 official examples + API docs
#include <gpiod.hpp>
#include <poll.h>

// Configure encoder lines: BOTH edges, no debounce, pull-up
gpiod::line_settings encoderSettings;
encoderSettings.set_direction(gpiod::line::direction::INPUT)
    .set_edge_detection(gpiod::line::edge::BOTH)
    .set_bias(gpiod::line::bias::PULL_UP);

// Configure push button: FALLING edge only, 250ms debounce
gpiod::line_settings buttonSettings;
buttonSettings.set_direction(gpiod::line::direction::INPUT)
    .set_edge_detection(gpiod::line::edge::FALLING)
    .set_bias(gpiod::line::bias::PULL_UP)
    .set_debounce_period(std::chrono::milliseconds(250));

// Configure reed switch: BOTH edges, 500ms debounce
gpiod::line_settings reedSettings;
reedSettings.set_direction(gpiod::line::direction::INPUT)
    .set_edge_detection(gpiod::line::edge::BOTH)
    .set_bias(gpiod::line::bias::PULL_UP)
    .set_debounce_period(std::chrono::milliseconds(500));

// Build single request with per-line settings
auto request = gpiod::chip("/dev/gpiochip4")
    .prepare_request()
    .set_consumer("media-console-gpio")
    // Volume encoder A/B (GPIO 27, 22)
    .add_line_settings({27, 22}, encoderSettings)
    // Volume encoder switch / mute (GPIO 23) -- actually this is
    // the volume encoder push. Per CONTEXT: mute is on input encoder
    // push (GPIO 5). Adjust pin assignments per actual wiring.
    .add_line_settings(5, buttonSettings)  // Input encoder push/mute
    // Input encoder A/B (GPIO 16, 20)
    .add_line_settings({16, 20}, encoderSettings)
    // Reed switch (GPIO 17)
    .add_line_settings(17, reedSettings)
    .do_request();

// Monitor with poll()
struct pollfd pfd;
pfd.fd = request.fd();
pfd.events = POLLIN;

gpiod::edge_event_buffer buffer(64);

while (!stopRequested) {
    int ret = poll(&pfd, 1, 100); // 100ms timeout for stop check
    if (ret > 0) {
        request.read_edge_events(buffer);
        for (const auto& event : buffer) {
            processEvent(event.line_offset(), event.type());
        }
    }
}
```

### Pattern 2: Quadrature State Machine
**What:** Decode rotary encoder direction from two-channel Gray code using a lookup table. Each pair of edge events on channels A and B produces a state transition that maps to CW (+1) or CCW (-1).
**When to use:** For both volume and input rotary encoders.
**Example:**
```cpp
// Source: Standard quadrature decoding algorithm
// Gray code state transitions: previous_state | current_state -> direction
// States: 00=0, 01=1, 11=3, 10=2 (note: Gray code order)
static constexpr int kQuadratureTable[4][4] = {
    // To:  00  01  11  10   From:
    {  0,  +1,   0,  -1 },  // 00
    { -1,   0,  +1,   0 },  // 01
    {  0,  -1,   0,  +1 },  // 11
    { +1,   0,  -1,   0 },  // 10
};

int decodeQuadrature(int prevState, int newState) {
    return kQuadratureTable[prevState][newState];
}

// In event handler:
void processEncoderEvent(unsigned int pinA, unsigned int pinB,
                         gpiod::line_request& request, int& prevState) {
    auto valA = request.get_value(pinA);
    auto valB = request.get_value(pinB);
    int newState = (static_cast<int>(valA) << 1) | static_cast<int>(valB);
    int direction = decodeQuadrature(prevState, newState);
    prevState = newState;
    // direction: +1 = CW, -1 = CCW, 0 = invalid/bounce
}
```

### Pattern 3: Background Thread with Atomic Stop Flag
**What:** QThread with `std::atomic<bool>` stop flag, following the established pattern from `LocalPlaybackController`.
**When to use:** For the GPIO monitoring thread.
**Example:**
```cpp
// Source: Project pattern from LocalPlaybackController
class LinuxGpioMonitor : public IGpioMonitor {
    Q_OBJECT
public:
    bool start() override;
    void stop() override;
private:
    void monitorLoop();
    QThread m_thread;
    std::atomic<bool> m_stopRequested{false};
};

bool LinuxGpioMonitor::start() {
    // Read initial reed switch state before starting thread
    auto initialValue = m_request->get_value(kReedSwitchPin);
    emit reedSwitchChanged(initialValue == gpiod::line::value::ACTIVE);

    m_stopRequested.store(false);
    m_thread.start();
    return true;
}

void LinuxGpioMonitor::stop() {
    m_stopRequested.store(true);
    m_thread.wait();
}
```

### Pattern 4: Interface Evolution for Delta-Based Volume
**What:** Update IGpioMonitor to emit `volumeChanged(int delta)` instead of separate `volumeUp()`/`volumeDown()` signals.
**When to use:** Recommended -- simplifies wiring to VolumeGestureController.onEncoderTick(int delta).

**Recommendation:** Change the volume signal to `volumeChanged(int delta)` where delta is the pre-scaled value (+2 or -2). This directly maps to `VolumeGestureController::onEncoderTick(int delta)` without a lambda adapter. The raw quadrature step (+1/-1) is multiplied by the scale factor (2) inside LinuxGpioMonitor before emitting. StubGpioMonitor's `simulateVolumeUp()`/`simulateVolumeDown()` methods emit `volumeChanged(+2)` and `volumeChanged(-2)` respectively.

The IGpioMonitor interface should be updated:
```cpp
signals:
    void volumeChanged(int delta);  // replaces volumeUp()/volumeDown()
    void muteToggled();
    void inputNext();
    void inputPrevious();
    void inputSelect();
    void reedSwitchChanged(bool magnetsApart);
```

### Anti-Patterns to Avoid
- **Separate threads per device:** One thread is sufficient since libgpiod v2 supports multi-line requests with a single fd. Multiple threads add complexity with no benefit.
- **Software debounce for push buttons/reed switch:** libgpiod v2 provides kernel-level hardware debounce via `set_debounce_period()`. Software timers duplicate functionality and add latency.
- **Polling line values in a loop:** Use edge-triggered event monitoring (`wait_edge_events` or `poll()` on fd), not busy polling `get_value()`. Edge events are interrupt-driven in the kernel.
- **Using gpiochip0 on Pi 5:** Must use `/dev/gpiochip4` for the RP1 GPIO controller. gpiochip0 is a different device.
- **Triggering on BOTH edges for mute button:** Causes double-toggle (GPIO-06 bug). Use `line::edge::FALLING` only.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| GPIO edge detection | Custom interrupt handler or sysfs polling | libgpiod v2 `read_edge_events()` | Kernel-level edge detection is reliable and efficient; sysfs is deprecated |
| Switch debounce | Software timer-based debounce | libgpiod `set_debounce_period()` | Kernel-level debounce is jitter-free and handles all edge cases |
| Quadrature decoding | Ad-hoc if/else direction detection | State machine lookup table | Lookup table handles all state transitions including noise rejection (0 = no action) |
| Cross-thread signal emission | Manual QMetaObject::invokeMethod | Qt signal/slot auto-connection | QObject signals emitted from worker thread automatically queued to receiver's thread |
| Platform detection | Manual #ifdef blocks in every file | PlatformFactory + compile flag | Single decision point; conditional compilation via HAS_GPIOD |

**Key insight:** The libgpiod v2 API handles the hard parts (interrupt-driven edge detection, hardware debounce, multi-line monitoring). The application only needs to decode the meaning of events (quadrature direction, button state) and emit Qt signals.

## Common Pitfalls

### Pitfall 1: Wrong gpiochip on Raspberry Pi 5
**What goes wrong:** Opening `/dev/gpiochip0` instead of `/dev/gpiochip4` gives access to the wrong GPIO controller (not the user-accessible 40-pin header).
**Why it happens:** Previous Pi models (Pi 4 and earlier) used gpiochip0. Pi 5 uses the RP1 south bridge which enumerates as gpiochip4 due to PCIe probe order.
**How to avoid:** Hard-code `/dev/gpiochip4` in config or use the `GpioConfig` struct. The chip path can be made configurable for future kernel changes.
**Warning signs:** `gpiod::chip` constructor throws, or events never arrive despite correct wiring.

### Pitfall 2: Volume Encoder Generating 0 Detents (No Tactile Feedback)
**What goes wrong:** The PEC11R-4020F-S0024 has 0 detents (smooth rotation) with 24 PPR. Each pulse corresponds to one quadrature state transition, not one physical "click." Users may expect click-to-volume mapping but get continuous response.
**Why it happens:** Part number 40**20**F: the "0" means 0 detents. This is by design for smooth volume control.
**How to avoid:** This is correct behavior -- the encoder provides 24 state transitions per revolution for smooth, fine-grained volume control. Each transition maps to +2 volume steps, yielding 48 volume steps per revolution (0-200 range / 48 ~= 4.2 revolutions for full range). No detent correction needed.
**Warning signs:** N/A -- if volume changes are too coarse/fine, adjust the delta multiplier.

### Pitfall 3: Quadrature Bounce Causing Spurious Direction Changes
**What goes wrong:** Mechanical contact bounce on encoder channels A/B causes rapid state transitions that the quadrature decoder interprets as direction reversals.
**Why it happens:** Rotary encoders have contact bounce on the order of 1-5ms. Without debounce on encoder channels, multiple edges arrive per actual state change.
**How to avoid:** Do NOT use libgpiod hardware debounce on encoder A/B channels -- it filters out legitimate rapid transitions during fast rotation. Instead, the quadrature state machine naturally rejects invalid transitions (table returns 0 for impossible jumps). Accumulators can also absorb brief reversals.
**Warning signs:** Volume jitters or input selection oscillates when turning the encoder.

### Pitfall 4: Double-Toggle on Mute Button
**What goes wrong:** Pressing the mute button toggles mute on press AND release, effectively doing nothing.
**Why it happens:** Configuring edge detection as `BOTH` instead of `FALLING` for the push button.
**How to avoid:** Use `gpiod::line::edge::FALLING` for the push button (GPIO 5). Combined with 250ms hardware debounce, this ensures exactly one event per press.
**Warning signs:** Mute state doesn't change on button press, or rapidly toggles back.

### Pitfall 5: Reed Switch Ambiguous Initial State
**What goes wrong:** On startup, the reed switch state is unknown until the first edge event occurs. The display may be in the wrong power state until the door is opened/closed.
**Why it happens:** Edge events only fire on transitions. If the door is already open when the application starts, no edge event fires.
**How to avoid:** In `start()`, immediately read the current pin value with `request.get_value(17)` and emit `reedSwitchChanged()` before entering the event loop.
**Warning signs:** Display power state is wrong after reboot until door is physically moved.

### Pitfall 6: Signals from Wrong Thread
**What goes wrong:** Qt signals emitted from the GPIO monitoring thread crash or exhibit undefined behavior if receivers are not thread-safe.
**Why it happens:** QObject signals are safe for cross-thread emission IF the connection type is `Qt::AutoConnection` (default) or `Qt::QueuedConnection`. Direct connections to non-thread-safe slots crash.
**How to avoid:** Use default `connect()` (auto-connection). Since `LinuxGpioMonitor` lives in the main thread but its monitoring lambda runs on `m_thread`, signals emitted from the monitoring loop are automatically queued to the main thread's event loop. The `QThread` pattern with `moveToThread` or `QThread::create` with signal emission handles this correctly.
**Warning signs:** Crashes on signal emission, or state updates happen on wrong thread.

### Pitfall 7: Missing HAS_GPIOD Guard on Non-Linux Build
**What goes wrong:** Build fails on macOS/non-Linux when `#include <gpiod.hpp>` is unconditionally included.
**Why it happens:** libgpiod is Linux-only. The LinuxGpioMonitor source files must be conditionally compiled.
**How to avoid:** Follow the HAS_ALSA / HAS_CDIO pattern: `pkg_check_modules(GPIOD ...)`, `target_compile_definitions(... HAS_GPIOD=1)`, `#ifdef HAS_GPIOD` guards in PlatformFactory.cpp, and conditional `list(APPEND LIB_SOURCES ...)` in CMakeLists.txt.
**Warning signs:** macOS build fails with "gpiod.hpp not found."

## Code Examples

### Opening a Chip and Requesting Lines
```cpp
// Source: libgpiod v2 C++ API (async_watch_line_value.cpp example + API docs)
#include <gpiod.hpp>

// All encoder + switch + reed GPIO pins in one request
auto request = gpiod::chip("/dev/gpiochip4")
    .prepare_request()
    .set_consumer("media-console-gpio")
    .add_line_settings(
        {27, 22},  // Volume encoder A, B
        gpiod::line_settings()
            .set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::BOTH)
            .set_bias(gpiod::line::bias::PULL_UP))
    .add_line_settings(
        {16, 20},  // Input encoder A, B
        gpiod::line_settings()
            .set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::BOTH)
            .set_bias(gpiod::line::bias::PULL_UP))
    .add_line_settings(
        5,  // Input encoder push button (mute/select)
        gpiod::line_settings()
            .set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::FALLING)
            .set_bias(gpiod::line::bias::PULL_UP)
            .set_debounce_period(std::chrono::milliseconds(250)))
    .add_line_settings(
        17,  // Reed switch
        gpiod::line_settings()
            .set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::BOTH)
            .set_bias(gpiod::line::bias::PULL_UP)
            .set_debounce_period(std::chrono::milliseconds(500)))
    .do_request();
```

### GPIO Event Monitoring Loop
```cpp
// Source: libgpiod v2 async_watch_line_value.cpp pattern + project QThread pattern
#include <poll.h>
#include <gpiod.hpp>

void LinuxGpioMonitor::monitorLoop()
{
    gpiod::edge_event_buffer buffer(64);
    struct pollfd pfd;
    pfd.fd = m_request.fd();
    pfd.events = POLLIN;

    while (!m_stopRequested.load(std::memory_order_relaxed)) {
        int ret = poll(&pfd, 1, 100);  // 100ms timeout for stop check
        if (ret < 0) {
            if (errno == EINTR) continue;
            qCWarning(mediaGpio) << "poll() error:" << strerror(errno);
            break;
        }
        if (ret == 0) continue;  // timeout, check stop flag

        m_request.read_edge_events(buffer);
        for (const auto& event : buffer) {
            handleEvent(event);
        }
    }
}
```

### Quadrature Decoder
```cpp
// Source: Standard quadrature decoding (Gray code state machine)
class QuadratureDecoder {
public:
    // Returns +1 (CW), -1 (CCW), or 0 (invalid/no change)
    int update(gpiod::line::value channelA, gpiod::line::value channelB) {
        int newState = (toInt(channelA) << 1) | toInt(channelB);
        int direction = kTable[m_prevState][newState];
        m_prevState = newState;
        return direction;
    }

    // Initialize from current pin values (call before entering event loop)
    void reset(gpiod::line::value channelA, gpiod::line::value channelB) {
        m_prevState = (toInt(channelA) << 1) | toInt(channelB);
    }

private:
    static int toInt(gpiod::line::value v) {
        return v == gpiod::line::value::ACTIVE ? 1 : 0;
    }

    int m_prevState = 0;

    // Lookup: kTable[prevState][newState] -> direction
    static constexpr int kTable[4][4] = {
        //       00  01  11  10
        /* 00 */ { 0, +1,  0, -1},
        /* 01 */ {-1,  0, +1,  0},
        /* 11 */ { 0, -1,  0, +1},
        /* 10 */ {+1,  0, -1,  0},
    };
};
```

### UIState doorOpen Property Addition
```cpp
// Source: Project pattern from existing UIState Q_PROPERTY declarations
// Add to UIState.h:
Q_PROPERTY(bool doorOpen READ doorOpen WRITE setDoorOpen NOTIFY doorOpenChanged)

// Add member, getter, setter, signal following existing pattern:
bool doorOpen() const { return m_doorOpen; }
void setDoorOpen(bool open);
// signal: void doorOpenChanged(bool open);
// member: bool m_doorOpen = true; // default: door open (display on)
```

### PlatformFactory Integration
```cpp
// Source: Project pattern from PlatformFactory.cpp (HAS_ALSA, HAS_CDIO pattern)
#ifdef HAS_GPIOD
#include "platform/LinuxGpioMonitor.h"
#endif

std::unique_ptr<IGpioMonitor> PlatformFactory::createGpioMonitor(QObject* parent)
{
#ifdef HAS_GPIOD
    return std::make_unique<LinuxGpioMonitor>(parent);
#else
    return std::make_unique<StubGpioMonitor>(parent);
#endif
}
```

### CMakeLists.txt Addition
```cmake
# Source: Project pattern from ALSA/CDIO/FLAC sections in CMakeLists.txt
# GPIO subsystem (Linux only -- libgpiod v2)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT TARGET PkgConfig::ALSA)
        find_package(PkgConfig REQUIRED)
    endif()
    pkg_check_modules(GPIOD IMPORTED_TARGET libgpiodcxx)
    if(GPIOD_FOUND)
        list(APPEND LIB_SOURCES
            src/platform/LinuxGpioMonitor.h
            src/platform/LinuxGpioMonitor.cpp
        )
        message(STATUS "GPIO subsystem: enabled (libgpiodcxx found)")
    else()
        message(STATUS "GPIO subsystem: disabled (libgpiodcxx not found)")
    endif()
endif()

# Link section:
if(GPIOD_FOUND)
    target_link_libraries(media-console-lib PUBLIC PkgConfig::GPIOD)
    target_compile_definitions(media-console-lib PUBLIC HAS_GPIOD=1)
endif()
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| sysfs GPIO (/sys/class/gpio) | libgpiod character device API | Linux 4.8+ (2016), sysfs deprecated 5.x | sysfs is deprecated and removed in newer kernels; libgpiod is the only supported path |
| libgpiod v1 (gpiod_line_request_events) | libgpiod v2 (line_request + edge_event_buffer) | libgpiod 2.0 (2023) | v2 API is incompatible with v1; uses request builder pattern, per-line settings, kernel debounce |
| wiringPi / RPi.GPIO | libgpiod | Pi 5 (2023) | Neither works on Pi 5 RP1; libgpiod is the only option |
| gpiochip0 (Pi 4) | gpiochip4 (Pi 5) | Pi 5 launch (2023) | RP1 south bridge enumerates via PCIe, gets higher chip number |
| Software debounce timers | Hardware/kernel debounce via set_debounce_period | libgpiod v2 / Linux 5.10+ | Kernel handles debounce more reliably; microsecond precision |

**Deprecated/outdated:**
- wiringPi: Abandoned by author, does not work on Pi 5
- RPi.GPIO: Python-only, does not work on Pi 5 RP1 memory mapping
- sysfs GPIO: Deprecated since Linux 5.x, removed in recent kernels
- libgpiod v1 API: Incompatible with v2; Trixie ships v2.2.1 only

## Open Questions

1. **Pin assignment for volume encoder push switch (GPIO 23)**
   - What we know: The CONTEXT.md lists volume encoder as GPIO 27/22/23 (3 pins = A, B, push). The input encoder is GPIO 16/20/5 (A, B, push). The push button on the input encoder serves as mute/select.
   - What's unclear: Is there a separate push button on the volume encoder (GPIO 23), or is 23 the third pin of a 3-pin encoder variant? The PEC11R-4020F-S0024 has a push switch (the "S" suffix), so GPIO 23 is the volume encoder push button. But the requirements only mention mute on the input encoder push button.
   - Recommendation: GPIO 23 (volume encoder push) could be wired but unused, or could serve as an alternative mute button. The CONTEXT.md says mute is on the input encoder push button (GPIO 5). Leave GPIO 23 monitored but not wired to any action initially. The wiring layer (AppBuilder) can add a connection later if needed.

2. **gpiochip4 stability across kernel updates**
   - What we know: On Pi 5, the RP1 GPIO controller is at `/dev/gpiochip4` due to PCIe probe order. Some kernels after July 2024 may rearrange chip numbering so gpiochip0 maps to user pins.
   - What's unclear: Whether Raspberry Pi OS Trixie uses the new or old numbering.
   - Recommendation: Make the chip path configurable via `GpioConfig` struct (default `/dev/gpiochip4`). This avoids hard-coding and allows runtime adjustment.

3. **Encoder A/B channel assignment to specific GPIO pins**
   - What we know: Volume encoder uses GPIO 27, 22 (and push on 23). Input encoder uses GPIO 16, 20 (and push on 5). But which GPIO is channel A and which is channel B?
   - What's unclear: The physical wiring -- which pin is A and which is B.
   - Recommendation: Make A/B assignment configurable. If CW rotation gives -1 instead of +1, swap A and B in config. The quadrature decoder is symmetric; swapping channels just inverts direction.

## Sources

### Primary (HIGH confidence)
- libgpiod v2 C++ API docs (https://libgpiod.readthedocs.io/en/stable/cpp_api.html) - chip, line_request, line_settings, edge_event, line enums
- libgpiod v2 C++ example: async_watch_line_value.cpp (https://github.com/brgl/libgpiod/blob/master/bindings/cxx/examples/async_watch_line_value.cpp) - complete working example of edge monitoring with poll()
- libgpiod v2 edge event API (https://libgpiod.readthedocs.io/en/stable/core_edge_event.html) - event types, timestamps, buffer API
- Bourns PEC11R datasheet (https://www.bourns.com/docs/Product-Datasheets/PEC11R.pdf) - encoder specifications, part number decoding

### Secondary (MEDIUM confidence)
- Raspberry Pi Forums: gpiochip4 on Pi 5 (https://forums.raspberrypi.com/viewtopic.php?t=364196) - confirmed gpiochip4 for RP1
- Raspberry Pi Forums: GPIO C programming Pi 5 (https://forums.raspberrypi.com/viewtopic.php?t=358676) - confirmed libgpiod is required for Pi 5
- Debian Trixie libgpiod-dev package (https://packages.debian.org/stable/source/libgpiod) - version 2.2.1-2+deb13u1, includes C++ bindings
- Raspberry Pi GPIO whitepaper (https://pip-assets.raspberrypi.com/categories/685-whitepapers-app-notes/documents/RP-006553-WP/) - RP1 architecture, gpiochip numbering

### Tertiary (LOW confidence)
- gpiochip numbering may change in newer kernels (forum discussion, not officially documented) - mitigated by configurable chip path

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - libgpiod v2 is the only viable option on Pi 5, version confirmed in Trixie packages
- Architecture: HIGH - single-request multi-line monitoring is well-documented in official examples; QThread pattern proven in project
- Pitfalls: HIGH - gpiochip4 confirmed by multiple sources; quadrature decoding is a well-understood algorithm; double-toggle bug has a clear fix (FALLING edge only)
- Code examples: MEDIUM-HIGH - based on official examples but adapted to project patterns; exact pin assignments need hardware verification

**Research date:** 2026-02-28
**Valid until:** 2026-03-28 (stable -- libgpiod v2 API is mature, encoder hardware is fixed)
