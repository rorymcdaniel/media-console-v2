# Stack Research

**Domain:** Embedded music console kiosk (Qt6/QML on Raspberry Pi 5)
**Researched:** 2026-02-28
**Confidence:** HIGH -- all versions verified against Debian Trixie package repositories

## Platform Baseline: Debian 13 Trixie (Raspberry Pi OS)

All version numbers below are verified against the Debian Trixie stable repositories. This is the authoritative source because the target is Raspberry Pi OS Trixie. Upstream "latest" versions may differ from what `apt install` provides -- always use the Trixie version as the build target.

| System Package | Trixie Version | Upstream Latest | Notes |
|----------------|---------------|-----------------|-------|
| CMake | 3.31.6 | 3.31.x | Exceeds Qt6 minimum (3.16) |
| Ninja | 1.12.1 | 1.12.x | Default generator for Qt6 CMake projects |
| Clang | 19.x | 20.x | clang-format-19, clang-tidy-19 |
| GCC | 14.x | 14.x | Default system compiler on Trixie |
| Python | 3.13 | 3.13 | Not directly used, but affects some build tools |

## Recommended Stack

### Core Framework

| Technology | Trixie Version | Purpose | Why Recommended |
|------------|---------------|---------|-----------------|
| Qt6 | 6.8.2 | Application framework, QML UI, networking, SQL, HTTP server | LTS release (supported to Oct 2029). Trixie ships 6.8.2 which is the current LTS branch. Do NOT chase 6.9 or 6.10 -- they are non-LTS and will EOL before the hardware does. |
| C++17 | -- | Language standard | Required by Qt6.8. GTest 1.16 (Trixie) supports C++14+, but 1.17+ requires C++17. C++17 is the sweet spot: structured bindings, std::optional, if-constexpr, fold expressions all useful for this project. C++20 adds nothing critical here and complicates cross-platform mock builds on macOS. |
| CMake | 3.31.6 | Build system | Modern CMake with full Qt6 support. Use cmake_minimum_required(VERSION 3.21) to access qt_add_qml_module and modern Qt6 CMake APIs. |
| Ninja | 1.12.1 | Build generator | Faster than Make for Qt projects (parallel compilation, smaller dependency graphs). Use `-G Ninja` unconditionally. |

### Qt6 Modules (all from qt6-* packages at 6.8.2)

| Module | Package | Purpose | Notes |
|--------|---------|---------|-------|
| Qt6::Core | qt6-base-dev | Signals/slots, QObject, event loop, QSettings, threading | Foundation of everything |
| Qt6::Gui | qt6-base-dev | Graphics, image handling, platform integration | Required for QML rendering |
| Qt6::Quick | qt6-declarative-dev | QML engine, Qt Quick scene graph | The UI framework |
| Qt6::Network | qt6-base-dev | TCP sockets (eISCP), HTTP client (Spotify, MusicBrainz, Discogs), SSL | All network I/O |
| Qt6::HttpServer | qt6-httpserver-dev | Built-in HTTP API server | Fully supported since Qt 6.5 (no longer tech preview). Simpler than embedding a third-party server. Route-based API with lambda handlers. |
| Qt6::Concurrent | qt6-base-dev | QtConcurrent::run for offloading blocking work | CD metadata fetch, library scanning |
| Qt6::Sql | qt6-base-dev | QSqlDatabase for SQLite | CD metadata cache, FLAC library database |

**Qt6 modules NOT needed:**
- Qt6::Multimedia -- bypassed entirely; ALSA is used directly for S/PDIF output via Nvdigi HAT. Qt Multimedia abstracts away the hardware control needed for bit-perfect digital output.
- Qt6::Widgets -- this is a QML-only UI. Widgets use software rendering, which is wrong for embedded GPU-accelerated displays.
- Qt6::WebSockets -- eISCP uses raw TCP, Spotify uses HTTP REST. No WebSocket protocol needed.
- Qt6::Qml (standalone) -- Qt6::Quick pulls this in transitively.

### Native Audio Libraries

| Library | Trixie Version | Upstream Latest | Purpose | Why This Version |
|---------|---------------|-----------------|---------|------------------|
| libasound2 (ALSA) | 1.2.14 | 1.2.14 | Direct PCM output to Nvdigi S/PDIF HAT | The ONLY way to get bit-perfect digital audio to the Nvdigi. Use `hw:2,0` direct hardware access (no dmix, no plug). Working params: 44100Hz, S16_LE, 2ch, period_size=1024, buffer_size=4096. |
| libcdio | 2.2.0 | 2.2.0 | CD-ROM drive control, TOC reading, disc detection | Stable C API. Used for disc presence polling, TOC extraction, eject. |
| libcdio-paranoia | 10.2+2.0.2 | 10.2+2.0.2 | Jitter-corrected CD audio sector reads | Built on top of libcdio. Provides paranoia_read() for error-correcting audio extraction. Critical for skip-free CD playback. |
| libsndfile | 1.2.2 | 1.2.2 | FLAC file decoding | Reads FLAC files into PCM buffers for ALSA output. Mature, stable API. sf_open/sf_readf_short pattern. |
| libsamplerate | 0.2.2 | 0.2.2 | Audio sample rate conversion | Converts non-44100Hz FLAC files to 44100Hz for S/PDIF output. Use SRC_SINC_MEDIUM_QUALITY (good balance of quality vs CPU on Pi 5). |

### Metadata Libraries

| Library | Trixie Version | Upstream Latest | Purpose | Why This Version |
|---------|---------------|-----------------|---------|------------------|
| TagLib | 2.0.2 | 2.1.1 | FLAC/audio file metadata extraction | Trixie ships 2.0.2, upstream is 2.1.1. The delta is minor (SHN support, MusicBrainz property map fix). 2.0.2 is fine for FLAC tag reading. Major API change was 1.x to 2.0 -- do NOT use TagLib 1.x patterns. |
| libdiscid | 0.6.4 | 0.6.5 | MusicBrainz disc ID calculation | Calculates the disc ID from CD TOC for MusicBrainz lookup. 0.6.5 is a maintenance release with no API changes vs 0.6.4. Trixie version is fine. |

### Hardware Interface Libraries

| Library | Trixie Version | Upstream Latest | Purpose | Critical Notes |
|---------|---------------|-----------------|---------|----------------|
| libgpiod | 2.2.1 | 2.2.x | GPIO line access for encoders and reed switch | **BREAKING CHANGE from v1.** Trixie ships v2.2.1. The v1 API (gpiod_chip_open, gpiod_line_get_value, gpiod_line_event_wait) is GONE. Must use v2 API: gpiod_chip_open(), gpiod_line_request_new(), gpiod_edge_event_buffer_new(), gpiod_line_request_wait_edge_events(). See "Migration Notes" section below. |

### Database

| Technology | Trixie Version | Purpose | Why Recommended |
|------------|---------------|---------|-----------------|
| SQLite3 | 3.46.x (bundled with Qt) | CD metadata cache, FLAC library index | Qt bundles SQLite via QSQLITE driver. No external dependency needed. Use QSqlDatabase::addDatabase("QSQLITE"). One connection per thread -- QSqlDatabase is NOT thread-safe across threads. |

### Testing

| Technology | Trixie Version | Purpose | Why Recommended |
|------------|---------------|---------|-----------------|
| Google Test | 1.16.0 | Unit testing framework | Use Trixie system package (libgtest-dev). Supports C++14+ which covers our C++17. Provides GTest::gtest, GTest::gmock via find_package(GTest). Use gtest_discover_tests() for CTest integration. |

### Development Tools

| Tool | Version | Purpose | Configuration Notes |
|------|---------|---------|---------------------|
| clang-format | 19 | Code formatting | Use Qt Creator's .clang-format as baseline (C++17, 100 col, 4-space indent, pointer-right-aligned). Customize for project preferences. See "clang-format Configuration" section below. |
| clang-tidy | 19 | Static analysis | Must pass `-extra-arg=-std=c++17` or clang-tidy defaults to C++14 and rejects Qt6 headers. Enable: bugprone-*, cppcoreguidelines-*, modernize-*, performance-*, readability-*. Disable: modernize-use-trailing-return-type (noisy with Qt patterns). |
| GDB | 15.x | Debugging | Works with Qt pretty-printers for QString, QList etc. |
| Valgrind | 3.23.x | Memory analysis | Useful for ALSA buffer leak detection. Suppress Qt's intentional static allocations. |

## Migration Notes: libgpiod v1 to v2

This is the single biggest API migration in the rewrite. The v1 code from media-console must be completely rewritten.

### v1 Pattern (DEAD -- does not compile against Trixie's libgpiod)
```c
// v1: line-centric API
struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
struct gpiod_line *line = gpiod_chip_get_line(chip, 17);
gpiod_line_request_both_edges_events(line, "consumer");
struct gpiod_line_event event;
gpiod_line_event_wait(line, &timeout);
gpiod_line_event_read(line, &event);
```

### v2 Pattern (REQUIRED)
```c
// v2: request-centric API
struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip4");  // Pi 5 uses gpiochip4

struct gpiod_line_settings *settings = gpiod_line_settings_new();
gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
gpiod_line_settings_set_debounce_period_us(settings, 1000);  // 1ms debounce

struct gpiod_line_config *line_cfg = gpiod_line_config_new();
unsigned int offsets[] = {CLK_PIN, DT_PIN};
gpiod_line_config_add_line_settings(line_cfg, offsets, 2, settings);

struct gpiod_request_config *req_cfg = gpiod_request_config_new();
gpiod_request_config_set_consumer(req_cfg, "media-console");

struct gpiod_line_request *request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

// Event monitoring
struct gpiod_edge_event_buffer *event_buffer = gpiod_edge_event_buffer_new(64);
gpiod_line_request_wait_edge_events(request, timeout_ns);
int num = gpiod_line_request_read_edge_events(request, event_buffer, 64);
for (int i = 0; i < num; i++) {
    struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(event_buffer, i);
    unsigned int offset = gpiod_edge_event_get_line_offset(event);
    enum gpiod_edge_event_type type = gpiod_edge_event_get_event_type(event);
}
```

### Key v2 Differences
- **Chip path, not name**: `gpiod_chip_open("/dev/gpiochip4")` not `gpiod_chip_open_by_name("gpiochip0")`
- **Pi 5 uses gpiochip4**: The RP1 southbridge on Pi 5 exposes GPIO on `/dev/gpiochip4`, not gpiochip0
- **Request-centric**: You configure settings, build a line config, then request all lines at once
- **Event buffers**: Edge events are read into a buffer, not one-at-a-time
- **Built-in debounce**: `gpiod_line_settings_set_debounce_period_us()` -- hardware debounce, reduces software complexity
- **Thread-aware but not thread-safe**: One request per thread, or externally synchronize

## Spotify OAuth: PKCE with Loopback Redirect

As of November 27, 2025, Spotify **requires** PKCE for public clients (which includes this desktop/kiosk app). The implicit grant flow is dead.

### Required Pattern
1. App generates code_verifier (43-128 char random string) and code_challenge (SHA256 hash of verifier)
2. Open browser/webview to `https://accounts.spotify.com/authorize` with code_challenge
3. Listen on `http://127.0.0.1:<dynamic_port>/callback` for the redirect
4. Exchange authorization code + code_verifier for access_token + refresh_token
5. Refresh tokens via `https://accounts.spotify.com/api/token` (no client_secret needed for PKCE)

### Loopback Exemption
- `http://127.0.0.1` is explicitly allowed (HTTP, not HTTPS) for native apps
- Port can be dynamically assigned -- register redirect URI without port number in Spotify Dashboard
- `localhost` is NOT allowed -- must use literal `127.0.0.1` or `[::1]`

### Implementation Notes
- Use Qt6::HttpServer on 127.0.0.1 to receive the callback (already in the stack)
- Store refresh_token in QSettings (encrypted if possible, but PKCE tokens are meant for public clients)
- Token refresh is a simple POST -- no client_secret needed

## MusicBrainz / GnuDB / Discogs: Metadata Lookup Chain

### MusicBrainz (Primary)
- **API**: REST, JSON (`Accept: application/json` or `fmt=json`)
- **Disc lookup**: `GET https://musicbrainz.org/ws/2/discid/{discid}?inc=recordings+artists&fmt=json`
- **Fuzzy lookup**: Pass `toc` parameter when disc ID is unknown; MB will fuzzy-match
- **Rate limit**: 1 req/sec with proper User-Agent header
- **No auth needed** for read-only lookups

### GnuDB (Fallback)
- **API**: CDDB protocol over HTTP (legacy but functional)
- **Status**: Active, 130k+ monthly users. Inherited freedb.org database
- **Risk**: Sustainability concerns -- donations don't cover hosting costs. May eventually require registration. Not a blocker now but worth monitoring.
- **Endpoint**: `http://gnudb.gnudb.org/~cddb/cddb.cgi`

### Discogs (Tertiary)
- **API**: REST, JSON
- **Rate limit**: 60 req/min (authenticated), 25 req/min (unauthenticated)
- **Auth**: Key/Secret pair (not OAuth needed for read-only)
- **Use case**: Album art fallback, additional metadata enrichment

## Qt6 CMake Best Practices

### Project Setup
```cmake
cmake_minimum_required(VERSION 3.21)  # Required for qt_add_qml_module
project(media-console VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Modern Qt6 project setup (sets QTP0001 policy for resource paths)
qt_standard_project_setup()

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core Gui Quick Network HttpServer Concurrent Sql
)
```

### QML Module Definition
```cmake
qt_add_executable(media-console ${SOURCES})

# QML singleton must be declared BEFORE qt_add_qml_module
set_source_files_properties(qml/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(media-console
    URI MediaConsole
    VERSION 2.0
    QML_FILES ${QML_FILES}
    RESOURCES ${RESOURCE_FILES}
    # No RESOURCE_PREFIX -- use QTP0001 default :/qt/qml/
)
```

### Google Test Integration
```cmake
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(test_example tests/test_example.cpp)
    target_link_libraries(test_example PRIVATE
        GTest::gtest GTest::gtest_main GTest::gmock
        Qt6::Core  # Only link Qt modules actually needed by tests
    )
    gtest_discover_tests(test_example
        DISCOVERY_MODE PRE_TEST  # Avoids issues with Qt event loop in test discovery
    )
endif()
```

### Native Library Detection (improved over v1)
```cmake
# Use pkg-config for native libraries (more reliable than find_library)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
pkg_check_modules(CDIO REQUIRED IMPORTED_TARGET libcdio)
pkg_check_modules(CDIO_PARANOIA REQUIRED IMPORTED_TARGET libcdio_paranoia)
pkg_check_modules(CDIO_CDDA REQUIRED IMPORTED_TARGET libcdio_cdda)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)
pkg_check_modules(DISCID REQUIRED IMPORTED_TARGET libdiscid)
pkg_check_modules(TAG REQUIRED IMPORTED_TARGET taglib)
pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
pkg_check_modules(SAMPLERATE REQUIRED IMPORTED_TARGET samplerate)

target_link_libraries(media-console PRIVATE
    PkgConfig::ALSA
    PkgConfig::CDIO
    PkgConfig::CDIO_PARANOIA
    PkgConfig::CDIO_CDDA
    PkgConfig::GPIOD
    PkgConfig::DISCID
    PkgConfig::TAG
    PkgConfig::SNDFILE
    PkgConfig::SAMPLERATE
)
```

## clang-format Configuration

Based on Qt Creator's own .clang-format, adapted for this project:

```yaml
# .clang-format
Language: Cpp
Standard: c++17
ColumnLimit: 100
IndentWidth: 4
TabWidth: 4
UseTab: Never

# Pointer alignment -- Qt convention
PointerAlignment: Right

# Braces -- Allman-ish for classes/functions, attached for control flow
BreakBeforeBraces: Custom
BraceWrapping:
  AfterClass: true
  AfterFunction: true
  AfterStruct: true
  AfterEnum: true
  AfterNamespace: false
  BeforeCatch: false
  BeforeElse: false

# Arguments/parameters
BinPackArguments: false
BinPackParameters: false
AllowShortBlocksOnASingleLine: Never
AllowShortFunctionsOnASingleLine: Inline

# Qt-specific
ForEachMacros: ['forever', 'foreach', 'Q_FOREACH', 'BOOST_FOREACH']
NamespaceIndentation: None
FixNamespaceComments: true

# Includes
SortIncludes: CaseSensitive
IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^<Q.*>'
    Priority: 2
  - Regex: '^<.*>'
    Priority: 3
  - Regex: '".*"'
    Priority: 1

# Other
AlignTrailingComments: true
SpaceAfterCStyleCast: true
BreakBeforeBinaryOperators: NonAssignment
BreakBeforeTernaryOperators: true
ConstructorInitializerBreakBeforeCommas: true
MaxEmptyLinesToKeep: 1
```

## clang-tidy Configuration

```yaml
# .clang-tidy
Checks: >
  -*,
  bugprone-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -cppcoreguidelines-avoid-magic-numbers,
  -readability-magic-numbers,
  -cppcoreguidelines-pro-type-reinterpret-cast,
  -cppcoreguidelines-owning-memory

WarningsAsErrors: ''
HeaderFilterRegex: 'src/.*'

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelBack
  - key: readability-identifier-naming.VariableCase
    value: camelBack
  - key: readability-identifier-naming.MemberPrefix
    value: 'm_'
  - key: readability-identifier-naming.ConstantCase
    value: CamelCase
  - key: readability-identifier-naming.ConstantPrefix
    value: 'k'

FormatStyle: file

ExtraArgs:
  - '-std=c++17'
```

**Disabled checks rationale:**
- `modernize-use-trailing-return-type`: Noisy with Qt slot/signal patterns, adds no value
- `cppcoreguidelines-avoid-magic-numbers` / `readability-magic-numbers`: ALSA params, GPIO pin numbers, eISCP protocol bytes are inherently numeric -- use named constants where meaningful but don't chase every literal
- `cppcoreguidelines-pro-type-reinterpret-cast`: Required for ALSA buffer handling (char* to int16_t*)
- `cppcoreguidelines-owning-memory`: Conflicts with Qt's parent-child ownership model

## QSqlDatabase Thread Safety Pattern

```cpp
// CORRECT: One connection per thread, unique name
void MyWorker::run() {
    const QString connName = QStringLiteral("worker_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("/path/to/metadata.db");
        db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
        db.open();

        QSqlQuery query(db);  // MUST pass db to constructor
        query.exec("SELECT ...");
        // ... use results ...
    }  // db goes out of scope

    QSqlDatabase::removeDatabase(connName);  // Clean up after scope
}

// WRONG: Sharing QSqlDatabase across threads
// WRONG: Using default connection from multiple threads
// WRONG: Creating QSqlQuery without passing the connection
```

## ALSA S/PDIF Output Pattern

```cpp
// Working configuration for Nvdigi HAT (hw:2,0)
snd_pcm_t *pcm;
snd_pcm_open(&pcm, "hw:2,0", SND_PCM_STREAM_PLAYBACK, 0);

snd_pcm_hw_params_t *hw_params;
snd_pcm_hw_params_alloca(&hw_params);
snd_pcm_hw_params_any(pcm, hw_params);

snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE);
snd_pcm_hw_params_set_channels(pcm, hw_params, 2);

unsigned int rate = 44100;
snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, 0);

snd_pcm_uframes_t period_size = 1024;
snd_pcm_hw_params_set_period_size_near(pcm, hw_params, &period_size, 0);

snd_pcm_uframes_t buffer_size = 4096;
snd_pcm_hw_params_set_buffer_size_near(pcm, hw_params, &buffer_size);

snd_pcm_hw_params(pcm, hw_params);
snd_pcm_prepare(pcm);

// Write loop (in dedicated thread)
while (playing) {
    int16_t buffer[period_size * 2];  // stereo
    // Fill buffer from CdAudioStream or FlacAudioStream
    snd_pcm_sframes_t written = snd_pcm_writei(pcm, buffer, period_size);
    if (written == -EPIPE) {
        snd_pcm_prepare(pcm);  // Recover from underrun
    } else if (written < 0) {
        snd_pcm_recover(pcm, written, 0);  // Try recovery
    }
}
```

**Key ALSA notes:**
- Use `hw:2,0` directly -- no `plughw:` or `default` (they add unwanted resampling/mixing)
- S/PDIF is bit-perfect -- what you write is what the receiver gets
- Period size 1024 at 44100Hz = ~23ms latency (acceptable for music playback)
- Buffer size 4096 = ~93ms total buffer (4 periods) -- enough to survive scheduling jitter on Pi 5
- Always handle -EPIPE (underrun) -- it WILL happen during CD seek operations

## Embedded Deployment Configuration

### Platform Plugin
```ini
# /etc/systemd/system/media-console.service
[Service]
Environment="QT_QPA_PLATFORM=eglfs"
Environment="QT_QPA_EGLFS_INTEGRATION=eglfs_kms"
Environment="QT_QPA_EGLFS_KMS_ATOMIC=1"
# Force 1920x720 resolution
Environment="QT_QPA_EGLFS_KMS_CONFIG=/etc/media-console/kms.json"
```

### KMS Configuration
```json
{
    "device": "/dev/dri/card1",
    "outputs": [
        {
            "name": "HDMI-A-1",
            "mode": "1920x720",
            "touchDevice": "/dev/input/eventX"
        }
    ]
}
```

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Audio output | Direct ALSA (libasound2) | PulseAudio / PipeWire | S/PDIF requires bit-perfect output. PA/PW add resampling and mixing that corrupt the digital stream. The Nvdigi HAT is a passthrough device. |
| HTTP server | Qt6::HttpServer | libmicrohttpd, Crow, cpp-httplib | Qt HttpServer is now fully supported (not tech preview), integrates with Qt event loop natively, handles routes with lambdas. No reason to add external dependency. |
| Testing | Google Test (system package) | Qt Test (QTest), Catch2 | GTest+GMock provides mock framework essential for hardware abstraction testing. Qt Test lacks mocking. Catch2 is fine but GTest has better CMake/CTest integration and is already in Trixie repos. Decision already made in project constraints. |
| GPIO | libgpiod v2 (C API) | sysfs, WiringPi, pigpio | sysfs is deprecated since kernel 4.8. WiringPi is abandoned. pigpio doesn't support Pi 5. libgpiod v2 is the only maintained option for Pi 5 GPIO. |
| CD metadata | MusicBrainz -> GnuDB -> Discogs | freedb, Gracenote | freedb is dead (redirects to GnuDB). Gracenote requires commercial license. This lookup chain is proven in v1. |
| Audio metadata | TagLib 2.x | libav/ffmpeg metadata, ID3Lib | TagLib is the standard for audio metadata. Clean C++ API. Supports FLAC, MP3, OGG. ID3Lib is unmaintained. ffmpeg is overkill for tag reading. |
| Sample rate conversion | libsamplerate | FFmpeg resampler, Sox | libsamplerate is purpose-built, small, well-tested. FFmpeg pulls in massive dependency tree. Sox is CLI-oriented. |
| Build system | CMake + Ninja | QMake, Meson | QMake is deprecated for Qt6. Meson has limited Qt6 support. CMake is the only first-class build system for Qt6. |
| Database | SQLite (via Qt QSQLITE) | PostgreSQL, LevelDB, JSON files | Single-user embedded app. SQLite is the right tool. No server process, no deployment complexity. Qt bundles the driver. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| QMake | Deprecated for Qt6. No qt_add_qml_module support. | CMake |
| Qt Multimedia | Abstracts ALSA behind a high-level API that cannot guarantee bit-perfect S/PDIF output. Cannot configure hw:2,0 directly. | Direct ALSA via libasound2 |
| WiringPi | Abandoned, does not support Pi 5 or libgpiod v2 kernel interface | libgpiod v2 |
| pigpio | Does not support Raspberry Pi 5 (uses legacy /dev/mem interface) | libgpiod v2 |
| sysfs GPIO (/sys/class/gpio) | Deprecated in kernel since 4.8, removed in some distributions | libgpiod v2 |
| TagLib 1.x API patterns | TagLib 2.0 changed API. Do not copy v1 code patterns. | TagLib 2.x (use PropertyMap interface) |
| libgpiod v1 API | Does not exist on Trixie. Will not compile. | libgpiod v2 API (request-centric model) |
| QSqlDatabase across threads | Will crash or corrupt data. Qt explicitly warns against this. | One QSqlDatabase per thread with unique connection name |
| Spotify implicit grant flow | Dead as of Nov 27, 2025. Will not work. | Authorization Code + PKCE with http://127.0.0.1 loopback |
| RESOURCE_PREFIX in qt_add_qml_module | Overridden by QTP0001 policy. Causes resource path confusion. | Omit it, use qt_standard_project_setup() defaults |
| FetchContent for GTest | Unnecessary complexity. Trixie has GTest 1.16 which supports C++17. System package is simpler and faster to build. | find_package(GTest REQUIRED) |

## Version Compatibility Matrix

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| Qt6 6.8.2 | CMake >= 3.16 | Trixie has 3.31.6, well above minimum |
| Qt6 6.8.2 | C++17 | Required minimum standard |
| GTest 1.16.0 | C++14+ | C++17 works fine. Note: GTest 1.17+ requires C++17 minimum |
| TagLib 2.0.2 | C++17 | TagLib 2.x requires C++17 |
| libgpiod 2.2.1 | Linux kernel >= 5.10 | Trixie runs kernel 6.12 LTS |
| libgpiod 2.2.1 | Pi 5 gpiochip4 | RP1 southbridge, NOT gpiochip0 |
| libcdio-paranoia 10.2+2.0.2 | libcdio >= 2.1 | Trixie has libcdio 2.2.0 |
| libsndfile 1.2.2 | FLAC format | Native FLAC decode support |

## Installation (Debian Trixie / Raspberry Pi OS)

```bash
# Qt6 development packages
sudo apt install -y \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-httpserver-dev \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts

# Build tools
sudo apt install -y \
    cmake \
    ninja-build \
    pkg-config \
    g++ \
    clang-format \
    clang-tidy

# Native audio libraries
sudo apt install -y \
    libasound2-dev \
    libcdio-dev \
    libcdio-paranoia-dev \
    libsndfile1-dev \
    libsamplerate0-dev

# Metadata libraries
sudo apt install -y \
    libtag1-dev \
    libdiscid-dev

# Hardware interface
sudo apt install -y \
    libgpiod-dev

# Testing
sudo apt install -y \
    libgtest-dev \
    libgmock-dev

# Runtime (for DDC/CI display control)
sudo apt install -y \
    ddcutil
```

## Sources

- [Qt Releases page](https://doc.qt.io/qt-6/qt-releases.html) -- Qt 6.8 LTS confirmed (HIGH confidence)
- [Debian Trixie package search](https://packages.debian.org/trixie/) -- all Trixie versions verified directly (HIGH confidence)
- [Raspberry Pi OS Trixie announcement](https://www.raspberrypi.com/news/trixie-the-new-version-of-raspberry-pi-os/) -- Trixie based on Debian 13, kernel 6.12 LTS (HIGH confidence)
- [libgpiod v2 migration discussion](https://github.com/brgl/libgpiod/discussions/56) -- v1 to v2 API incompatibility confirmed (HIGH confidence)
- [libgpiod documentation](https://libgpiod.readthedocs.io/) -- v2 API reference (HIGH confidence)
- [Raspberry Pi Forums - gpiod v2](https://forums.raspberrypi.com/viewtopic.php?t=394138) -- Pi 5 uses gpiochip4 (MEDIUM confidence)
- [Spotify OAuth migration blog](https://developer.spotify.com/blog/2025-02-12-increasing-the-security-requirements-for-integrating-with-spotify) -- PKCE required, implicit grant dead Nov 2025 (HIGH confidence)
- [Spotify PKCE flow docs](https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow) -- loopback redirect allowed (HIGH confidence)
- [Spotify redirect URI docs](https://developer.spotify.com/documentation/web-api/concepts/redirect_uri) -- http://127.0.0.1 exemption confirmed (HIGH confidence)
- [Qt HttpServer docs](https://doc.qt.io/qt-6/qhttpserver.html) -- fully supported module, route-based API (HIGH confidence)
- [GoogleTest releases](https://github.com/google/googletest/releases) -- v1.16.0 and v1.17.0 versions confirmed (HIGH confidence)
- [TagLib releases](https://github.com/taglib/taglib/releases) -- v2.1.1 upstream, Trixie has 2.0.2 (HIGH confidence)
- [libdiscid 0.6.5 release](https://musicbrainz.wordpress.com/2025/05/21/libdiscid-0-6-5-released/) -- maintenance release (HIGH confidence)
- [MusicBrainz API docs](https://musicbrainz.org/doc/MusicBrainz_API) -- disc lookup, JSON format (HIGH confidence)
- [GnuDB status](https://gnudb.org/) -- active, sustainability concerns noted (MEDIUM confidence)
- [Discogs API docs](https://www.discogs.com/developers) -- rate limits 60/min authenticated (MEDIUM confidence)
- [ALSA PCM API docs](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html) -- standard reference (HIGH confidence)
- [Qt Creator .clang-format](https://github.com/qt-creator/qt-creator/blob/master/.clang-format) -- Qt coding style baseline (HIGH confidence)
- [Qt for Embedded Linux](https://doc.qt.io/qt-6/embedded-linux.html) -- EGLFS platform plugin docs (HIGH confidence)
- [qt_add_qml_module docs](https://doc.qt.io/qt-6/qt-add-qml-module.html) -- QML module CMake API (HIGH confidence)
- [QSqlDatabase threading](https://doc.qt.io/qt-6/qsqldatabase.html) -- thread safety constraints (HIGH confidence)

---
*Stack research for: Media Console v2 (Qt6/C++17 embedded music kiosk)*
*Researched: 2026-02-28*
