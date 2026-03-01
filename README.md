# Media Console v2

A Qt6/QML touchscreen kiosk for a hi-fi music system. Runs on a Raspberry Pi mounted near the stereo and controls an Onkyo receiver over eISCP, plays CDs and a FLAC library locally via ALSA, and shows Spotify/streaming metadata.

**Target platform**: Raspberry Pi OS Trixie (aarch64)

## Features

- Onkyo receiver control over eISCP (volume, input selection, power, mute)
- CD playback with error correction via libcdio-paranoia and async MusicBrainz/CDDB metadata lookup
- FLAC library playback with SQLite-backed library database and hierarchical browse
- Spotify and streaming service metadata display
- GPIO hardware controls: volume encoder, input selector, reed switch
- DDC/CI display brightness and screen timeout
- REST HTTP API for remote control
- Kiosk deployment via systemd on Raspberry Pi OS

## Dependencies

### Build tools

```
cmake >= 3.16
ninja-build
pkg-config
clang-format
clang-tidy   # optional, enables static analysis during build
```

### Qt6

Qt 6.8+ is required. Install via the [Qt Online Installer](https://www.qt.io/download-qt-installer) or your distro's package manager if Qt 6.8 is available.

Required Qt modules: Core, Gui, Quick, Qml, Network, Sql, Concurrent, NetworkAuth
Optional: Qt6::HttpServer (enables the REST API; not available on all platforms)

### System libraries (Raspberry Pi OS / Debian)

```
sudo apt-get install \
    libasound2-dev \
    libcdio-dev \
    libcdio-paranoia-dev \
    libcdio-cdda2 \
    libdiscid-dev \
    libsndfile1-dev \
    libsamplerate0-dev \
    libtag1-dev \
    libgpiod-dev \
    ddcutil
```

| Package | Purpose |
|---|---|
| `libasound2-dev` | ALSA audio output (required) |
| `libcdio-dev` | CD drive access |
| `libcdio-paranoia-dev` | CD ripping with error correction |
| `libcdio-cdda2` | CD digital audio |
| `libdiscid-dev` | MusicBrainz disc ID calculation |
| `libsndfile1-dev` | FLAC/audio file decoding |
| `libsamplerate0-dev` | Audio resampling |
| `libtag1-dev` | Audio file metadata (TagLib) |
| `libgpiod-dev` | GPIO hardware controls (volume encoder, etc.) |
| `ddcutil` | DDC/CI monitor brightness control (optional) |

Subsystems with missing libraries are automatically disabled at configure time — the build will still succeed without them.

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

CMake will report which subsystems are enabled based on what it finds:

```
-- CD subsystem: enabled (libcdio + libdiscid found)
-- FLAC library: enabled (libsndfile + libsamplerate + taglib found)
-- GPIO subsystem: enabled (libgpiodcxx found)
-- HTTP API server: enabled (Qt6::HttpServer found)
-- DDC/CI display: enabled (ddcutil found at /usr/bin/ddcutil)
```

### Running tests

```bash
cmake --build build --target test
# or
cd build && ctest --output-on-failure
```

## Configuration

The app reads `~/.config/MediaConsole/media-console.conf` (INI format) on startup.

```ini
[receiver]
host=192.168.1.100   # IP address of Onkyo receiver
port=60128           # eISCP port (default: 60128)

[audio]
device=hw:2,0        # ALSA device for local playback

[display]
dim_timeout_sec=300    # Seconds of inactivity before dimming
off_timeout_sec=1200   # Seconds before screen off
dim_brightness=25      # Brightness percentage when dimmed (0–100)

[api]
port=8080            # HTTP API port

[library]
root=/media/flac     # Root directory of FLAC music library
```

A default config is created automatically by the kiosk installer if none exists.

## Kiosk deployment

Install the binary, then run the kiosk installer as root:

```bash
sudo cmake --build build --target install   # installs to /usr/local/bin/media-console
sudo ./deploy/install-kiosk.sh
```

The installer:
- Installs and enables the systemd service
- Configures auto-login on tty1
- Disables screen blanking
- Creates a default config if one doesn't exist

```bash
sudo systemctl start media-console   # start now
journalctl -u media-console -f       # view logs
sudo systemctl status media-console  # check status
```

To uninstall:

```bash
sudo ./deploy/uninstall-kiosk.sh
```
