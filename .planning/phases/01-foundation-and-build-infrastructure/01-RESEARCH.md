# Phase 1: Foundation and Build Infrastructure - Research

**Researched:** 2026-02-28
**Domain:** CMake build system, Qt6 project skeleton, platform abstraction, composition root, Google Test integration
**Confidence:** HIGH

## Summary

Phase 1 establishes the entire project skeleton: CMake build system targeting Qt6 6.8.2, platform abstraction interfaces with stub implementations, a composition root (AppBuilder) replacing the original 325-line main.cpp, typed configuration (AppConfig), logging categories, code formatting/linting, and Google Test integration. The original codebase at `~/Code/media-console` is the reference implementation -- its CMakeLists.txt, main.cpp, and Logging.h provide the blueprint for what the new skeleton must support.

The key architectural shift is from compile-time `#ifdef __linux__` guards to runtime interface injection via PlatformFactory. This enables full macOS development with stub hardware, which the original could not do. The composition root (AppBuilder) replaces scattered object construction in main.cpp with a structured object graph that returns an AppContext struct.

**Primary recommendation:** Build the CMake project with `qt_standard_project_setup(REQUIRES 6.8)` and `qt_add_qml_module()`, use FetchContent for Google Test, create four platform abstraction interfaces (IAudioOutput, ICdDrive, IGpioMonitor, IDisplayControl) with stub implementations, and wire everything through AppBuilder returning AppContext.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Base style: WebKit** -- 4-space indent, Allman braces (opening brace on own line). Closest to Qt's own code style.
- **Line length: 120 characters** -- room for Qt's verbose type names (Q_PROPERTY, signal/slot signatures) without excessive wrapping.
- **Include order: strict** -- corresponding header first, then Qt headers, then system headers, then project headers. Each group separated by blank line. clang-format sorts within groups.
- **Naming: Qt standard** -- PascalCase for classes and enums, camelCase for methods and variables, m_ prefix for members, SCREAMING_SNAKE for compile-time constants. No project namespace.
- **Header guards: `#pragma once`** -- as specified in rewrite document.
- **Primary dev platform: macOS** -- develop on Mac, deploy to Pi. Stubs must be robust enough for real development, not just compilation.
- **macOS builds: full dev cycle** -- compile, run tests, AND launch app with stub hardware. Stubs must provide simulated behavior (e.g., stub ALSA output that accepts frames silently, stub GPIO that can be driven programmatically, stub display control that logs commands).
- **CI: local only** -- no GitHub Actions pipeline. Run tests manually on dev machine.
- **Pre-commit hook: format only** -- pre-commit runs clang-format (auto-fix). clang-tidy runs during CMake build but does NOT block commits.

### Claude's Discretion
- Config organization: nested structs per subsystem vs flat AppConfig. Claude chooses based on what works cleanly with const reference passing.
- Interface granularity: one interface per hardware unit (IAudioOutput, ICdDrive, IGpioMonitor, IDisplayControl) as specified in requirements. Claude decides exact method signatures during implementation.
- AppBuilder initialization order and error handling strategy.
- Exact clang-tidy check set (start with modernize-*, bugprone-*, performance-*, readability-* and tune from there).
- Google Test + CTest configuration details.

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FOUND-01 | Project builds with CMake/Ninja targeting Qt6 6.8.2 on Raspberry Pi OS Trixie (aarch64) | CMake project structure, qt_standard_project_setup, find_package Qt6 modules |
| FOUND-02 | clang-format enforced via .clang-format config with pre-commit hook | .clang-format WebKit base style config, git pre-commit hook script |
| FOUND-03 | clang-tidy integrated into CMake build with modernize-*, bugprone-*, performance-*, readability-* checks | CMAKE_CXX_CLANG_TIDY variable, .clang-tidy config |
| FOUND-04 | Google Test framework integrated with CTest, test discovery via gtest_discover_tests() | FetchContent for GoogleTest, enable_testing(), gtest_discover_tests() |
| FOUND-05 | Platform abstraction interfaces defined (IAudioOutput, IGpioMonitor, ICdDrive, IDisplayControl) with stub implementations | Pure virtual interface pattern, stub classes with simulated behavior |
| FOUND-06 | PlatformFactory provides real or stub implementations based on runtime detection | Factory pattern with QSysInfo::kernelType() runtime check |
| FOUND-07 | AppConfig struct loaded once at startup from QSettings INI, passed by const reference | Typed config struct, QSettings one-time load, const ref passing |
| FOUND-08 | Composition root (AppBuilder) constructs full object graph with clear ownership and signal/slot wiring | Builder pattern returning AppContext struct, replaces original main.cpp |
| FOUND-09 | CODING_STANDARDS.md created and enforced from first commit | Document covering style, naming, patterns, testing conventions |
| FOUND-10 | Logging categories defined with configurable levels | Q_DECLARE_LOGGING_CATEGORY, Q_LOGGING_CATEGORY macros, 8 categories from original |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6 | 6.8.2 | Application framework (Core, Gui, Quick, Network, Concurrent, Sql) | Target platform ships with Qt 6.8.x on Debian Trixie |
| CMake | >= 3.16 | Build system generator | Required by Qt6, used by original project |
| Ninja | latest | Build backend | Faster than Make for incremental builds |
| Google Test | 1.17.0 | C++ unit testing framework | Industry standard for C++ testing, CMake integration via FetchContent |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| clang-format | >= 18 | Code formatting | Pre-commit hook, auto-fix on every commit |
| clang-tidy | >= 18 | Static analysis | CMake build integration, catches bugs and enforces modernization |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Google Test | Catch2 | Catch2 is header-only (simpler), but GTest has better CMake integration and mock support via GMock |
| FetchContent | system-installed GTest | FetchContent pins the version and avoids system dependency variance |

## Architecture Patterns

### Recommended Project Structure
```
media-console-v2/
├── CMakeLists.txt              # Top-level CMake
├── .clang-format               # WebKit-based format config
├── .clang-tidy                 # Static analysis config
├── .githooks/
│   └── pre-commit              # clang-format auto-fix hook
├── CODING_STANDARDS.md         # Project conventions
├── src/
│   ├── main.cpp                # Minimal: create QGuiApplication, call AppBuilder, load QML
│   ├── app/
│   │   ├── AppBuilder.h        # Composition root
│   │   ├── AppBuilder.cpp
│   │   ├── AppContext.h        # Struct holding all constructed objects
│   │   ├── AppConfig.h         # Typed configuration struct
│   │   └── AppConfig.cpp       # QSettings -> AppConfig one-time load
│   ├── platform/
│   │   ├── IAudioOutput.h      # Pure virtual interface
│   │   ├── ICdDrive.h
│   │   ├── IGpioMonitor.h
│   │   ├── IDisplayControl.h
│   │   ├── PlatformFactory.h   # Runtime factory
│   │   ├── PlatformFactory.cpp
│   │   └── stubs/
│   │       ├── StubAudioOutput.h/.cpp
│   │       ├── StubCdDrive.h/.cpp
│   │       ├── StubGpioMonitor.h/.cpp
│   │       └── StubDisplayControl.h/.cpp
│   ├── utils/
│   │   ├── Logging.h           # Q_DECLARE_LOGGING_CATEGORY for 8 categories
│   │   └── Logging.cpp         # Q_LOGGING_CATEGORY definitions + init function
│   └── qml/
│       ├── main.qml            # Minimal placeholder
│       └── Theme.qml           # Singleton placeholder
├── tests/
│   ├── CMakeLists.txt          # Test target configuration
│   ├── test_AppConfig.cpp      # Config loading tests
│   ├── test_AppBuilder.cpp     # Composition root tests
│   ├── test_PlatformFactory.cpp # Factory tests
│   ├── test_StubAudioOutput.cpp
│   ├── test_StubCdDrive.cpp
│   ├── test_StubGpioMonitor.cpp
│   └── test_StubDisplayControl.cpp
└── icons/                      # Copied from original
```

### Pattern 1: Composition Root (AppBuilder)
**What:** A single class that constructs the entire object graph and returns a value struct (AppContext) containing all wired objects. Replaces the 325-line main.cpp.
**When to use:** At application startup, exactly once.
**Example:**
```cpp
// AppContext.h
struct AppContext
{
    // Ownership held by AppBuilder (QObject parent chain)
    // These are non-owning pointers for consumers
    IAudioOutput* audioOutput = nullptr;
    ICdDrive* cdDrive = nullptr;
    IGpioMonitor* gpioMonitor = nullptr;
    IDisplayControl* displayControl = nullptr;
    // Future phases add state objects, controllers, etc.
};

// AppBuilder.h
class AppBuilder : public QObject
{
    Q_OBJECT
public:
    explicit AppBuilder(QObject* parent = nullptr);
    AppContext build(const AppConfig& config);
};
```

### Pattern 2: Platform Abstraction via Interface + Factory
**What:** Pure virtual interfaces for hardware-dependent components. PlatformFactory creates real or stub implementations based on runtime OS detection.
**When to use:** For any component that touches hardware (audio, CD, GPIO, display).
**Example:**
```cpp
// IAudioOutput.h
class IAudioOutput
{
public:
    virtual ~IAudioOutput() = default;
    virtual bool open(const QString& deviceName, int sampleRate, int channels, int bitDepth) = 0;
    virtual bool write(const char* data, qint64 frames) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};

// PlatformFactory.cpp
std::unique_ptr<IAudioOutput> PlatformFactory::createAudioOutput()
{
    if (QSysInfo::kernelType() == "linux")
    {
        // Future: return std::make_unique<AlsaAudioOutput>();
        return std::make_unique<StubAudioOutput>();  // Phase 1: stubs only
    }
    return std::make_unique<StubAudioOutput>();
}
```

### Pattern 3: Typed Configuration (AppConfig)
**What:** A struct loaded once from QSettings at startup, passed by const reference. Eliminates scattered QSettings reads.
**When to use:** Any configurable value. The original has ~20 QSettings reads in main.cpp lines 53-174.
**Example:**
```cpp
// AppConfig.h
struct ReceiverConfig
{
    QString host = "192.168.68.63";
    int port = 60128;
};

struct DisplayConfig
{
    int id = 1;
    int dimTimeoutSeconds = 300;
    int offTimeoutSeconds = 1200;
    int dimBrightness = 25;
    bool timeoutEnabled = true;
};

struct AppConfig
{
    ReceiverConfig receiver;
    DisplayConfig display;
    // ... more subsystem configs

    static AppConfig loadFromSettings();  // One-time QSettings read
};
```

### Anti-Patterns to Avoid
- **God object main.cpp:** The original main.cpp constructs everything, wires signals, reads config, and handles platform checks. AppBuilder encapsulates this.
- **`#ifdef __linux__` guards:** The original uses compile-time guards (lines 189-272 of main.cpp). The rewrite uses runtime PlatformFactory injection so macOS builds exercise the full code path with stubs.
- **`setContextProperty()` for QML binding:** Deprecated. Use `qmlRegisterSingletonInstance()` per Qt 6 best practices. Note: Phase 1 only sets up the QML module skeleton; actual singleton registration happens in Phase 2.
- **Scattered QSettings reads:** The original reads settings in 20+ places in main.cpp. AppConfig consolidates to a single load.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Code formatting | Custom scripts | clang-format with .clang-format config | WebKit base style is built-in, handles Qt patterns |
| Static analysis | Manual code review only | clang-tidy with CMake integration | Catches real bugs (bugprone-*), enforces modern C++ |
| Test discovery | Manual test registration | gtest_discover_tests() | Automatically finds TEST/TEST_F macros, integrates with CTest |
| Build system | Manual Makefile | CMake with qt_standard_project_setup() | Qt6 requires CMake, handles MOC/RCC/QML automatically |
| Version extraction | Custom script | CMake execute_process(git describe) | Already proven pattern from original CMakeLists.txt |

**Key insight:** Phase 1 is infrastructure -- maximize use of established tooling so subsequent phases focus on domain logic.

## Common Pitfalls

### Pitfall 1: Qt MOC Not Finding Headers
**What goes wrong:** Q_OBJECT macro in headers not processed by MOC, causing linker errors for vtable/meta-object.
**Why it happens:** Headers not listed in CMake target sources or CMAKE_AUTOMOC not enabled.
**How to avoid:** Always set `CMAKE_AUTOMOC ON` and list all Q_OBJECT headers in `qt_add_executable()` SOURCES.
**Warning signs:** "undefined reference to vtable for ClassName" linker errors.

### Pitfall 2: clang-tidy False Positives on Qt Code
**What goes wrong:** clang-tidy flags Qt macros (Q_OBJECT, Q_PROPERTY, SIGNAL/SLOT) as issues.
**Why it happens:** Qt macros expand to code patterns that trigger some checks.
**How to avoid:** Add targeted suppressions in .clang-tidy: `-modernize-use-trailing-return-type`, `-readability-redundant-access-specifiers` (Q_OBJECT introduces access specifiers). Use `HeaderFilterRegex` to only check project headers.
**Warning signs:** Hundreds of warnings from Qt-generated code.

### Pitfall 3: FetchContent Downloading on Every Build
**What goes wrong:** Google Test source is re-downloaded each time cmake is invoked.
**Why it happens:** FetchContent_Declare without FIND_PACKAGE_ARGS or system cache.
**How to avoid:** Use `FETCHCONTENT_UPDATES_DISCONNECTED` after first successful configure, or use a specific GIT_TAG (not branch).
**Warning signs:** Slow CMake configure phase, network errors in offline builds.

### Pitfall 4: PlatformFactory Creating Real Implementations in Phase 1
**What goes wrong:** Trying to link against ALSA/libgpiod/libcdio in Phase 1 when only stubs are needed.
**Why it happens:** Eagerness to wire real implementations before the code exists.
**How to avoid:** Phase 1 PlatformFactory returns stubs unconditionally. Future phases add conditional logic.
**Warning signs:** Build failures on macOS due to missing Linux libraries.

### Pitfall 5: AppConfig Holding QSettings Reference
**What goes wrong:** AppConfig stores QSettings& and reads lazily, reintroducing scattered reads.
**Why it happens:** Confusing "typed config" with "config proxy."
**How to avoid:** AppConfig::loadFromSettings() reads all values once, copies to struct members, returns by value. No QSettings reference stored.
**Warning signs:** QSettings appearing in any file other than AppConfig.cpp.

### Pitfall 6: Interface Methods Not Matching Future Usage
**What goes wrong:** Platform interfaces defined too narrowly, requiring breaking changes in later phases.
**Why it happens:** Not studying the original codebase's actual usage patterns.
**How to avoid:** Study the original code for each interface domain:
- IAudioOutput: see `AlsaOutput.h` -- open/write/close/isOpen + device info
- ICdDrive: see `CdDrive.h` -- readToc/getDiscId/eject
- IGpioMonitor: see `VolumeEncoderMonitor.h`, `InputEncoderMonitor.h`, `ReedSwitchMonitor.h` -- start/stop with signals
- IDisplayControl: see `DisplayControl.h` -- power/brightness/autoDetect
**Warning signs:** "I'll figure out the interface later" -- define it now from the reference code.

## Code Examples

### CMakeLists.txt Top-Level Structure
```cmake
# Source: Qt 6.8 official documentation + original project CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(media-console VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 components needed in Phase 1 (minimal set, expand in later phases)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
qt_standard_project_setup(REQUIRES 6.8)

# clang-tidy integration (runs during build, does not block commits)
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
endif()

# Version from git tags (lifted from original)
execute_process(
    COMMAND git describe --tags --long --match "v*.*.*" --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
# ... version parsing (same pattern as original)

qt_add_executable(media-console
    src/main.cpp
    # ... all source and header files
)

qt_add_qml_module(media-console
    URI MediaConsole
    VERSION 1.0
    QML_FILES
        src/qml/main.qml
        src/qml/Theme.qml
)

target_include_directories(media-console PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(media-console PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml)

# Testing
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### .clang-format Configuration
```yaml
# WebKit base style with project overrides
BasedOnStyle: WebKit
ColumnLimit: 120
IndentWidth: 4
BreakBeforeBraces: Allman

# Include ordering: corresponding header, Qt, system, project
IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^"[^/]*\.h"'       # Corresponding header
    Priority: 1
  - Regex: '^<Q'               # Qt headers
    Priority: 2
  - Regex: '^<'                # System headers
    Priority: 3
  - Regex: '^"'                # Project headers
    Priority: 4
SortIncludes: CaseSensitive
```

### .clang-tidy Configuration
```yaml
---
Checks: >
  -*,
  bugprone-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-redundant-access-specifiers,
  -readability-identifier-length
HeaderFilterRegex: 'src/.*\.h$'
WarningsAsErrors: ''
```

### Google Test Integration (tests/CMakeLists.txt)
```cmake
# Source: Google Test quickstart-cmake documentation
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

add_executable(media-console-tests
    test_AppConfig.cpp
    test_AppBuilder.cpp
    test_PlatformFactory.cpp
    test_StubAudioOutput.cpp
    test_StubCdDrive.cpp
    test_StubGpioMonitor.cpp
    test_StubDisplayControl.cpp
)

target_link_libraries(media-console-tests PRIVATE
    GTest::gtest_main
    Qt6::Core
    # Link against project library target (not executable)
)

target_include_directories(media-console-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(media-console-tests)
```

### Pre-commit Hook (.githooks/pre-commit)
```bash
#!/bin/bash
# Format-only pre-commit hook (clang-tidy runs during build, not here)
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(cpp|h|hpp)$')
if [ -z "$STAGED_FILES" ]; then
    exit 0
fi

for FILE in $STAGED_FILES; do
    clang-format -i "$FILE"
    git add "$FILE"
done
```

### Logging Categories (lifted from original Logging.h)
```cpp
// src/utils/Logging.h
#pragma once
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(mediaApp)
Q_DECLARE_LOGGING_CATEGORY(mediaSpotify)
Q_DECLARE_LOGGING_CATEGORY(mediaReceiver)
Q_DECLARE_LOGGING_CATEGORY(mediaAudio)
Q_DECLARE_LOGGING_CATEGORY(mediaHttp)
Q_DECLARE_LOGGING_CATEGORY(mediaLidarr)
Q_DECLARE_LOGGING_CATEGORY(mediaGpio)
Q_DECLARE_LOGGING_CATEGORY(mediaCd)

// Initialize logging from AppConfig (not QSettings directly)
void initLogging(const QString& filterRules = QString());
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `setContextProperty()` | `qmlRegisterSingletonInstance()` | Qt 5.14+ | Type-safe QML bindings, better tooling support |
| `#ifdef __linux__` | Runtime PlatformFactory | Architecture decision | Full macOS dev cycle with stubs |
| Manual MOC invocation | `CMAKE_AUTOMOC ON` | CMake 3.0+ | Automatic MOC processing |
| `qt5_add_resources()` | `qt_add_qml_module()` | Qt 6.2+ | Unified QML module setup with auto-generated qmldir |
| Manual GTest setup | `FetchContent + gtest_discover_tests()` | CMake 3.11+ | Automatic download and test discovery |

**Deprecated/outdated:**
- `QMake`: Replaced by CMake as Qt's primary build system starting Qt 6
- `qt5_*` CMake commands: Use `qt_*` (version-agnostic) commands in Qt 6
- `QT_QML_MODULE_VERSION`: Not needed when using qt_add_qml_module

## Open Questions

1. **Interface method signatures for IGpioMonitor**
   - What we know: The original has three separate monitor classes (VolumeEncoderMonitor, InputEncoderMonitor, ReedSwitchMonitor), each with start()/stop() and specific signals.
   - What's unclear: Should IGpioMonitor be a single interface covering all GPIO, or should there be separate interfaces per hardware unit?
   - Recommendation: Single IGpioMonitor with methods for each monitor type (volumeEncoder, inputEncoder, reedSwitch) since the factory creates all three as a unit. Alternatively, three separate interfaces if they need independent lifecycle management. The planner should decide based on Phase 7 requirements -- for Phase 1, a single interface with combined stubs is simpler.

2. **AppConfig nested struct depth**
   - What we know: The original reads ~20 settings across 5 groups (receiver, display, cd, spotify, library, logging, api).
   - What's unclear: Whether to nest one level (AppConfig.receiver.host) or two levels deep.
   - Recommendation: One level of nesting per QSettings group. Matches the INI file structure and keeps const reference passing straightforward: `void ReceiverController::init(const ReceiverConfig& config)`.

3. **Test library target separation**
   - What we know: Tests need to link against project code but not the main() function.
   - What's unclear: Best CMake pattern for this.
   - Recommendation: Create a `media-console-lib` static library target containing all source files except main.cpp. Both the executable and test targets link against it. This is the standard CMake pattern for testable applications.

## Sources

### Primary (HIGH confidence)
- Qt 6.8 official documentation (doc.qt.io/qt-6.8) -- CMake integration, qt_standard_project_setup, qt_add_qml_module, QML singleton registration
- Google Test GitHub repository (github.com/google/googletest) -- CMake FetchContent integration, gtest_discover_tests
- Original project `~/Code/media-console/CMakeLists.txt` -- proven Qt6 build configuration
- Original project `~/Code/media-console/src/main.cpp` -- object graph to decompose into AppBuilder
- Original project `~/Code/media-console/src/utils/Logging.h` -- 8 logging categories to lift

### Secondary (MEDIUM confidence)
- clang-format WebKit style documentation -- base style aligns with Qt conventions
- clang-tidy check documentation -- check selection for Qt/C++17 codebases

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Qt6 + CMake + GTest is well-documented, original project proves the combination works
- Architecture: HIGH - Composition root, platform abstraction, typed config are established patterns; original codebase provides concrete reference
- Pitfalls: HIGH - Identified from real issues in Qt/CMake projects and analysis of original codebase anti-patterns

**Research date:** 2026-02-28
**Valid until:** 2026-03-30 (stable domain, Qt 6.8 is LTS-adjacent)
