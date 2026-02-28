# Coding Standards: Media Console v2

## Code Style

- **Base style:** WebKit (configured in `.clang-format`)
- **Indent:** 4 spaces (no tabs)
- **Braces:** Allman style (opening brace on its own line)
- **Line length:** 120 characters maximum
- **Header guards:** `#pragma once` (no include guards)

## Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes / Enums | PascalCase | `AppBuilder`, `MediaSource` |
| Methods / Functions | camelCase | `loadFromSettings()`, `createAudioOutput()` |
| Variables | camelCase | `audioOutput`, `filterRules` |
| Member variables | m_ prefix | `m_brightness`, `m_powered` |
| Constants (compile-time) | SCREAMING_SNAKE | `VERSION_STRING`, `DEFAULT_PORT` |
| Namespaces | None | No project namespace |

## Include Order

Each group separated by a blank line, clang-format sorts within groups:

1. Corresponding header (e.g., `"AppConfig.h"` in `AppConfig.cpp`)
2. Qt headers (`<QObject>`, `<QString>`, etc.)
3. System headers (`<memory>`, `<vector>`, etc.)
4. Project headers (`"utils/Logging.h"`, `"platform/IAudioOutput.h"`)

## Logging

- Use category macros: `qCInfo(mediaApp)`, `qCWarning(mediaReceiver)`, etc.
- Never use bare `qInfo()`, `qWarning()`, `qDebug()` -- always use the category variant
- Available categories: `mediaApp`, `mediaSpotify`, `mediaReceiver`, `mediaAudio`, `mediaHttp`, `mediaLidarr`, `mediaGpio`, `mediaCd`

## Platform Abstraction

- All hardware-dependent code behind interfaces (`IAudioOutput`, `ICdDrive`, `IGpioMonitor`, `IDisplayControl`)
- Never use `#ifdef __linux__` or compile-time platform guards
- `PlatformFactory` provides implementations at runtime
- Stubs provide simulated behavior for macOS development

## Configuration

- All configuration through `AppConfig` struct loaded once at startup
- Never access `QSettings` directly -- only `AppConfig::loadFromSettings()` reads QSettings
- Pass config by const reference: `void init(const ReceiverConfig& config)`

## Testing

- Google Test framework with CTest integration
- At least one test per platform interface using stub implementations
- Tests link against `media-console-lib` static library (not the executable)
- Test discovery via `gtest_discover_tests()`

## QML Binding

- Use `qmlRegisterSingletonInstance()` for exposing C++ objects to QML
- Never use deprecated `setContextProperty()`
- QML module URI: `MediaConsole`, version 1.0
- QML singletons require `pragma Singleton` and `QT_QML_SINGLETON_TYPE TRUE`

## Development Workflow

- **Pre-commit hook:** clang-format auto-fixes staged files (format only, no blocking)
- **clang-tidy:** Runs during CMake build, reports warnings, does not block commits
- **Primary dev platform:** macOS with stub hardware
- **Target platform:** Raspberry Pi 5 (aarch64, Debian Trixie)
- **CI:** Local only, no GitHub Actions

## Commit Messages

- Format: `type(scope): description`
- Types: `feat`, `fix`, `refactor`, `test`, `docs`, `chore`
- Scope: phase number or component name
