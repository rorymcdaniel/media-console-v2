# Phase 3: Receiver Control - Research

**Researched:** 2026-02-28
**Domain:** eISCP TCP protocol, QTcpSocket networking, volume gesture coalescing
**Confidence:** HIGH

## Summary

Phase 3 implements the eISCP TCP connection to an Onkyo TX-8260 receiver on port 60128, providing full control over power, volume, input, mute, and metadata. The eISCP protocol is a well-documented binary framing protocol wrapping ISCP text commands. Qt6's QTcpSocket with QTimer-based reconnection provides a clean async pattern for persistent connections. Volume gesture coalescing requires a dedicated controller that accumulates encoder ticks, updates UI optimistically, and sends a single absolute volume command after the gesture ends.

**Primary recommendation:** Implement a single `EiscpConnection` class for TCP framing/transport, a `ReceiverController` for command/response logic and state updates, and a `VolumeGestureController` for encoder coalescing. All three are constructed by AppBuilder, wired via signals/slots.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Connect immediately on startup, retry forever with backoff
- Error banner (persistent, top of screen) when receiver is disconnected — aligns with UI-15 ErrorBanner
- Preserve last-known ReceiverState during disconnection — do not clear to defaults
- On reconnect, silently refresh all state from receiver
- VolumeGestureController built in Phase 3 (ORCH-03), not deferred to Phase 7
- Volume display: decimal 0.0–100.0 (full precision from 0-200 receiver range)
- Reconciliation after gesture: snap UI to receiver-reported value immediately (no animation)
- Command source tagging infrastructure built now: Local, External, API enum — ready for GPIO (Phase 7) and HTTP API (Phase 9)
- Volume overlay shows only for Local source; External changes update state silently
- Clear all metadata fields immediately on input switch — no stale data from previous source
- Progressive display: show each eISCP fragment (NTI, NAT, NAL, NJA2) as it arrives — don't buffer
- Non-streaming inputs (Phono, CD, Computer, Bluetooth): show source name as title placeholder
- Set both streamingService enum AND serviceName string on service detection — QML binds directly to string
- 30-second stale data warning (RECV-12): subtle indicator in status bar, not toast — clears when data resumes
- Command failures (rejected input switch, etc.): revert optimistic state silently, no notification
- Receiver power-off connection behavior: Claude's discretion based on actual eISCP protocol behavior

### Claude's Discretion
- Reconnection backoff strategy (exponential vs fixed, timing)
- Volume gesture timeout duration (when gesture is considered "ended")
- Connection behavior when receiver powers off (stay connected vs disconnect)
- eISCP protocol implementation details (framing, checksums, command queuing)

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| RECV-01 | eISCP TCP connection with auto-reconnect | eISCP framing protocol + QTcpSocket reconnect pattern via QTimer |
| RECV-02 | Volume 0-200 displayed as 0.0-100.0 | MVL command uses hex 00-C8 (0-200); display = raw / 2.0 |
| RECV-03 | Volume gesture coalescing | VolumeGestureController with QTimer gesture timeout, accumulates delta |
| RECV-04 | Optimistic UI updates during gesture | Controller updates ReceiverState.volume immediately per tick |
| RECV-05 | 6 input sources selectable via SLI | SLI command with hex codes: 2B, 22, 23, 05, 2E (existing toHexCode) |
| RECV-06 | Power on/off control | PWR command: "01" on, "00" off, "QSTN" query |
| RECV-07 | Mute toggle/on/off | AMT command: "01" on, "00" off, "TG" toggle, "QSTN" query |
| RECV-08 | State polling every 2.5s | QTimer firing QSTN queries for MVL, PWR, AMT, SLI |
| RECV-09 | Track metadata parsing | NTI (title), NAT (artist), NAL (album), NJA2 (art URL), NFI (file info), NMS (service), NTM (time) |
| RECV-10 | Streaming service detection | NMS command parsing: map service codes to StreamingService enum values |
| RECV-11 | Playback state via NST | NST command: parse play/pause/stop states into PlaybackMode enum |
| RECV-12 | Stale data detection after 30s | QTimer tracking last-received timestamp, emit warning signal |
| RECV-13 | Volume overlay only for local input | CommandSource enum tagging: Local shows overlay, External updates silently |
| ORCH-03 | VolumeGestureController | Separate class: coalesces ticks, manages gesture state, emits final volume command |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6::Network | 6.8.2 | QTcpSocket for eISCP TCP connection | Qt's async socket with signal/slot integration; already using Qt6 |
| QTimer | 6.8.2 | Reconnect backoff, polling, gesture timeout, stale data detection | Event-loop native, no threading needed for timers |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QDataStream / QByteArray | 6.8.2 | Binary packet construction and parsing | eISCP header framing (big-endian integers) |
| QElapsedTimer | 6.8.2 | Gesture timing, stale data measurement | Lightweight monotonic clock |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| QTcpSocket | Raw POSIX sockets | No Qt signal/slot integration, manual event loop |
| QTimer for reconnect | QStateMachine | Overkill for simple connect/retry state |
| Custom eISCP framing | Third-party library | No C++/Qt eISCP library exists; protocol is simple enough to implement |

**Installation:**
```bash
# Already available — Qt6::Network needs to be added to CMakeLists.txt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Test Network)
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── receiver/
│   ├── EiscpConnection.h/cpp      # TCP transport + eISCP framing
│   ├── EiscpMessage.h/cpp         # Message construction/parsing
│   ├── ReceiverController.h/cpp   # Command logic + state updates
│   └── VolumeGestureController.h/cpp  # Encoder coalescing
├── state/
│   ├── CommandSource.h            # Local/External/API enum
│   └── (existing state files)
└── app/
    └── (AppBuilder wires new classes)
```

### Pattern 1: Layered Transport + Controller
**What:** Separate TCP framing (EiscpConnection) from command semantics (ReceiverController).
**When to use:** Always — keeps protocol details isolated from business logic.
**Example:**
```cpp
// EiscpConnection handles framing only
class EiscpConnection : public QObject
{
    Q_OBJECT
public:
    void connectToReceiver(const QString& host, int port);
    void sendCommand(const QString& command); // e.g., "MVL1A"

signals:
    void messageReceived(const QString& message); // e.g., "MVL1A"
    void connected();
    void disconnected();
    void connectionError(const QString& error);
};

// ReceiverController handles command semantics
class ReceiverController : public QObject
{
    Q_OBJECT
public:
    void setVolume(int volume);    // Sends MVL{hex}
    void setPower(bool on);        // Sends PWR01/PWR00
    void selectInput(MediaSource source); // Sends SLI{hex}
    void toggleMute();             // Sends AMTTG

private slots:
    void onMessageReceived(const QString& message);
    void onPollTimer();
};
```

### Pattern 2: QTimer-Based Reconnection with Exponential Backoff
**What:** On disconnect, start a QTimer that attempts reconnect with increasing delays.
**When to use:** Persistent connection to hardware that may temporarily go offline.
**Example:**
```cpp
// Recommended backoff: 1s, 2s, 4s, 8s, 16s, 30s (cap)
void EiscpConnection::onDisconnected()
{
    m_reconnectTimer.start(m_currentBackoff);
    m_currentBackoff = qMin(m_currentBackoff * 2, 30000);
}

void EiscpConnection::onConnected()
{
    m_reconnectTimer.stop();
    m_currentBackoff = 1000; // Reset backoff
}
```

### Pattern 3: Gesture Coalescing with Timeout
**What:** Accumulate rapid encoder ticks, update UI per-tick, send one command after gesture ends.
**When to use:** Hardware encoder input that generates many events quickly.
**Example:**
```cpp
class VolumeGestureController : public QObject
{
    Q_OBJECT
public:
    void onEncoderTick(int delta); // Called per encoder event

signals:
    void volumeChanged(int newVolume, CommandSource source); // Per-tick UI update
    void gestureEnded(int finalVolume); // Single command to receiver

private:
    QTimer m_gestureTimer; // Fires when gesture is "done"
    int m_pendingVolume;
    bool m_gestureActive = false;
};
```

### Anti-Patterns to Avoid
- **Blocking waitForConnected/waitForReadyRead:** Never use blocking calls in the main thread. Use async signal/slot pattern exclusively.
- **Monolithic controller:** Don't put framing, reconnection, command logic, and gesture coalescing in one class.
- **Polling in a tight loop:** Don't spin-wait for data. Use QTcpSocket::readyRead signal.
- **Creating new QTcpSocket on each reconnect:** Reuse the same socket, call abort() then connectToHost().
- **Clearing state on temporary disconnect:** User decision is to preserve last-known state during disconnection.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Big-endian integer encoding | Manual byte shifting | `qToBigEndian()` / `QDataStream` with `QDataStream::BigEndian` | Endianness bugs are subtle and platform-dependent |
| Timer management | std::thread + sleep | QTimer | Qt event loop integration, no thread management needed |
| Socket reconnection | Custom thread with reconnect loop | QTcpSocket + QTimer in main thread | Event-driven, no threading complexity |

**Key insight:** The eISCP protocol is simple enough (16-byte header + text command) that a third-party library would add more complexity than implementing it directly. But use Qt's built-in byte-order and timer utilities rather than rolling your own.

## Common Pitfalls

### Pitfall 1: QTcpSocket Stuck in ConnectingState
**What goes wrong:** After a failed connection attempt, calling connectToHost() again without abort() leaves the socket in ConnectingState permanently.
**Why it happens:** QTcpSocket maintains internal state; must be reset before new connection attempt.
**How to avoid:** Always call `m_socket->abort()` before `m_socket->connectToHost()` in the reconnect path.
**Warning signs:** Socket state never transitions to Connected or Unconnected after initial failure.

### Pitfall 2: Partial eISCP Message Reads
**What goes wrong:** readyRead fires but only a partial message is available; parsing fails or reads garbage.
**Why it happens:** TCP is a stream protocol; one readyRead does not guarantee one complete eISCP frame.
**How to avoid:** Buffer incoming data. Read the 16-byte header first to get message size, then wait for that many bytes before parsing. Use QByteArray as a read buffer.
**Warning signs:** Garbled responses, intermittent parse errors.

### Pitfall 3: Volume Gesture Race Condition
**What goes wrong:** Gesture ends, command sent, but poll response arrives with old value before receiver processes command, causing UI to flicker.
**Why it happens:** Poll timer fires between command send and receiver acknowledgment.
**How to avoid:** Suppress poll-based volume updates for a brief window after sending a volume command (e.g., 500ms grace period). Or tag the pending command and ignore poll responses until ACK.
**Warning signs:** Volume display flickers between gesture value and old receiver value after gesture ends.

### Pitfall 4: NJA2 URL Contains Receiver IP
**What goes wrong:** Album art URL from NJA2 uses the receiver's internal IP which may not be accessible.
**Why it happens:** Onkyo receivers host album art via their built-in HTTP server (typically http://<receiver-ip>:<port>/...).
**How to avoid:** Use the receiver's configured IP (from AppConfig) to construct the art URL if the receiver returns a relative path. Verify URL accessibility.
**Warning signs:** Album art loads in some networks but not others.

### Pitfall 5: CD/Library Input Ambiguity (0x23)
**What goes wrong:** Receiver reports SLI23 but app doesn't know if it's CD or Library.
**Why it happens:** Both CD and Library share hex code 0x23 on the Onkyo receiver.
**How to avoid:** Already handled in fromHexCode() — returns CD as default. Application context (which source the user selected) disambiguates. When switching input programmatically, track the intended source.
**Warning signs:** Library input shows as CD in the UI.

## Code Examples

### eISCP Packet Construction
```cpp
// Source: miracle2k/onkyo-eiscp-dotnet Core.cs + protocol documentation
QByteArray buildEiscpPacket(const QString& command)
{
    // ISCP message: "!1" + command + "\r"
    QByteArray iscp = "!1" + command.toLatin1() + "\r";

    // eISCP header: "ISCP" + headerSize(16) + messageSize + version(1) + reserved(000)
    QByteArray packet;
    packet.append("ISCP");                           // 4 bytes: magic
    QDataStream headerSize(&packet, QIODevice::Append);
    headerSize.setByteOrder(QDataStream::BigEndian);
    headerSize << quint32(16);                       // 4 bytes: header size
    headerSize << quint32(iscp.size());              // 4 bytes: message size
    packet.append(char(1));                          // 1 byte: version
    packet.append(3, char(0));                       // 3 bytes: reserved

    packet.append(iscp);
    return packet;
}
```

### eISCP Packet Parsing
```cpp
// Source: miracle2k/onkyo-eiscp-dotnet Core.cs
// Returns empty string if incomplete packet in buffer
QString parseEiscpPacket(QByteArray& buffer)
{
    if (buffer.size() < 16) return {};  // Need full header

    // Validate magic
    if (buffer.left(4) != "ISCP") {
        buffer.clear();  // Corrupted — reset
        return {};
    }

    // Extract message size (bytes 8-11, big-endian)
    QDataStream stream(buffer.mid(8, 4));
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 messageSize;
    stream >> messageSize;

    int totalSize = 16 + static_cast<int>(messageSize);
    if (buffer.size() < totalSize) return {};  // Incomplete

    // Extract ISCP message, strip framing: "!1" prefix and CR/LF/EOF suffix
    QByteArray iscp = buffer.mid(16, messageSize);
    buffer.remove(0, totalSize);

    // Strip "!1" prefix and trailing CR/LF/EOF bytes
    QString msg = QString::fromLatin1(iscp);
    if (msg.startsWith("!1")) msg = msg.mid(2);
    while (msg.endsWith('\r') || msg.endsWith('\n') || msg.endsWith(QChar(0x1A)))
        msg.chop(1);

    return msg;  // e.g., "MVL1A" or "NTISong Title"
}
```

### Volume Hex Conversion
```cpp
// MVL command uses 2-character hex representation of 0-200
QString volumeToHex(int volume)
{
    return QString("%1").arg(qBound(0, volume, 200), 2, 16, QChar('0')).toUpper();
}

int hexToVolume(const QString& hex)
{
    bool ok;
    int val = hex.toInt(&ok, 16);
    return ok ? qBound(0, val, 200) : -1;
}
```

### Streaming Service Detection from NMS
```cpp
// NMS format: "xxpp...p" where xx = service type code
// Service codes from protocol documentation
StreamingService detectService(const QString& nmsPayload)
{
    if (nmsPayload.length() < 2) return StreamingService::Unknown;

    QString code = nmsPayload.left(2);
    if (code == "0A") return StreamingService::Spotify;
    if (code == "04") return StreamingService::Pandora;
    if (code == "18") return StreamingService::AirPlay;
    if (code == "13") return StreamingService::AmazonMusic;  // iHeartRadio repurposed? Verify.
    // Chromecast: may not have a standard eISCP code — verify on hardware
    return StreamingService::Unknown;
}
```

### NST Playback State Parsing
```cpp
// NST response format: "prs" where p=play, r=repeat, s=shuffle
// p: 'P'=playing, 'p'=paused, 'S'=stopped, 'F'=FF, 'R'=FR
PlaybackMode parseNstPlayState(const QString& nstPayload)
{
    if (nstPayload.isEmpty()) return PlaybackMode::Stopped;

    QChar playChar = nstPayload.at(0);
    if (playChar == 'P') return PlaybackMode::Playing;
    if (playChar == 'p') return PlaybackMode::Paused;
    return PlaybackMode::Stopped;  // 'S', 'F', 'R', or unknown
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Serial RS-232 ISCP | TCP eISCP over Ethernet | ~2010 | Network control, no serial cable |
| QTcpSocket with waitFor* blocking | Fully async signal/slot | Qt5+ | No UI freezes |
| Manual byte order swapping | qToBigEndian / QDataStream | Qt4+ | Cross-platform correct |

**Deprecated/outdated:**
- Blocking socket APIs (waitForConnected, waitForReadyRead): Still available but should never be used in GUI thread
- QTcpSocket with QThread worker: Unnecessary for eISCP — event loop in main thread handles the low throughput fine

## Open Questions

1. **Chromecast eISCP service code**
   - What we know: Spotify=0A, Pandora=04, AirPlay=18 are confirmed
   - What's unclear: Exact NMS code for Chromecast on TX-8260. May vary by firmware version.
   - Recommendation: Log unrecognized NMS codes at runtime. Add Chromecast mapping when verified on hardware.

2. **NJA2 vs NJA album art URL format**
   - What we know: NJA provides album art; NJA2 referenced in requirements
   - What's unclear: Whether TX-8260 firmware sends NJA or NJA2 (newer models may use NJA2 with higher-res URLs)
   - Recommendation: Handle both NJA and NJA2 command codes. Parse URL from payload.

3. **Receiver behavior on power-off**
   - What we know: PWR00 puts receiver in standby
   - What's unclear: Whether TCP connection drops on standby or stays open
   - Recommendation: Handle both cases. If connection drops, reconnect timer handles it. If stays open, poll detects power state change.

4. **Amazon Music eISCP service code on TX-8260**
   - What we know: iHeartRadio is 0x13, but Amazon Music may use a different code
   - What's unclear: Exact service code mapping for this model
   - Recommendation: Log unknown service codes. Populate mapping from runtime observation.

## Sources

### Primary (HIGH confidence)
- [miracle2k/onkyo-eiscp-dotnet Core.cs](https://github.com/miracle2k/onkyo-eiscp-dotnet/blob/master/onkyo-eiscp/Core.cs) - eISCP packet framing, header construction, response parsing
- [miracle2k/onkyo-eiscp commands YAML](https://github.com/miracle2k/onkyo-eiscp/blob/master/eiscp-commands.yaml) - MVL, PWR, AMT, SLI command definitions
- [Qt6 QTcpSocket documentation](https://doc.qt.io/qt-6/qtcpsocket.html) - Socket API reference
- [Qt6 QAbstractSocket documentation](https://doc.qt.io/qt-6/qabstractsocket.html) - State machine, error handling

### Secondary (MEDIUM confidence)
- [miracle2k/onkyo-eiscp NSV command](https://github.com/miracle2k/onkyo-eiscp/blob/master/commands/dock/NSV.yaml) - Streaming service codes (0A=Spotify, 04=Pandora, 18=AirPlay)
- [Sudo Null eISCP article](https://sudonull.com/post/9179-Onkyos-ISCP-eISCP-protocol-control-of-Onkyo-devices-over-the-network) - NTI, NAT, NAL, NJA, NTM, NST command descriptions
- [Qt Forum reconnection patterns](https://forum.qt.io/topic/136916/reconnecting-using-qabstractsocket-connecttohost) - abort() before connectToHost() pattern

### Tertiary (LOW confidence)
- Amazon Music / Chromecast service codes - Need hardware verification on TX-8260
- NJA2 vs NJA behavior on specific firmware - Need runtime testing

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Qt6::Network is the established choice for Qt TCP applications
- Architecture: HIGH - Layered transport/controller pattern well-proven in IoT/hardware control
- Protocol framing: HIGH - Multiple open-source implementations confirm header format
- Command codes: HIGH for core (MVL, PWR, AMT, SLI), MEDIUM for metadata (NTI, NAT, NMS)
- Pitfalls: HIGH - Common QTcpSocket issues well-documented in Qt forums

**Research date:** 2026-02-28
**Valid until:** 2026-06-28 (stable protocol, mature Qt6 APIs)
