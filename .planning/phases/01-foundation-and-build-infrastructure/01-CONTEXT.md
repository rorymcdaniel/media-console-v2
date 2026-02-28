# Phase 1: Foundation and Build Infrastructure - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Compilable, testable project skeleton with CMake build system, platform abstraction interfaces, composition root, typed configuration, logging categories, code formatting, linting, and Google Test framework. No feature code — just the scaffolding every subsequent phase lands in.

</domain>

<decisions>
## Implementation Decisions

### Code Style Baseline
- **Base style: WebKit** — 4-space indent, Allman braces (opening brace on own line). Closest to Qt's own code style.
- **Line length: 120 characters** — room for Qt's verbose type names (Q_PROPERTY, signal/slot signatures) without excessive wrapping.
- **Include order: strict** — corresponding header first, then Qt headers, then system headers, then project headers. Each group separated by blank line. clang-format sorts within groups.
- **Naming: Qt standard** — PascalCase for classes and enums, camelCase for methods and variables, m_ prefix for members, SCREAMING_SNAKE for compile-time constants. No project namespace.
- **Header guards: `#pragma once`** — as specified in rewrite document.

### Development Workflow
- **Primary dev platform: macOS** — develop on Mac, deploy to Pi. Stubs must be robust enough for real development, not just compilation.
- **macOS builds: full dev cycle** — compile, run tests, AND launch app with stub hardware. Can develop features without the Pi connected. This means platform stubs must provide simulated behavior (e.g., stub ALSA output that accepts frames silently, stub GPIO that can be driven programmatically, stub display control that logs commands).
- **CI: local only** — no GitHub Actions pipeline. Run tests manually on dev machine.
- **Pre-commit hook: format only** — pre-commit runs clang-format (auto-fix). clang-tidy runs during CMake build but does NOT block commits. This keeps the commit workflow fast while still catching lint issues during development.

### Claude's Discretion
- Config organization: nested structs per subsystem vs flat AppConfig. Claude chooses based on what works cleanly with const reference passing.
- Interface granularity: one interface per hardware unit (IAudioOutput, ICdDrive, IGpioMonitor, IDisplayControl) as specified in requirements. Claude decides exact method signatures during implementation.
- AppBuilder initialization order and error handling strategy.
- Exact clang-tidy check set (start with modernize-*, bugprone-*, performance-*, readability-* and tune from there).
- Google Test + CTest configuration details.

</decisions>

<specifics>
## Specific Ideas

- The original main.cpp (325 lines at ~/Code/media-console/src/main.cpp) is the reference for what the composition root must replace. Every object constructed and every signal/slot connection in that file must have a corresponding place in AppBuilder.
- The original uses `setContextProperty()` for QML binding (deprecated). The rewrite uses `qmlRegisterSingletonInstance()` per research findings.
- The original CMakeLists.txt uses `find_library()` for native deps with `#ifdef __linux__` guards. The rewrite replaces compile-time guards with runtime interface injection via PlatformFactory.
- Configuration organization in the original: ~20+ QSettings reads scattered across main.cpp lines 53-174. All must be consolidated into a typed AppConfig struct.

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- Original CMakeLists.txt (~/Code/media-console/CMakeLists.txt): Qt6 module list, native library find commands, QML module setup, and version extraction from git tags. Lift structure but modernize (use `qt_standard_project_setup()`, `pkg_check_modules()`).
- Original Logging.h/cpp (~/Code/media-console/src/utils/Logging.h): 8 logging categories already defined. Lift category definitions directly.
- Original icons/ directory: all icon assets can be copied as-is.

### Established Patterns
- QSettings INI format at `~/.config/MediaConsole/media-console.conf` — preserve this path and organization name.
- Application name: "media-console", organization: "MediaConsole".
- Version from git tags: `v{major}.{minor}.{patch}-{hash}` format.
- QML module URI: "MediaConsole" version 1.0.

### Integration Points
- Every subsequent phase adds source files to CMakeLists.txt and registers objects in AppBuilder.
- Platform interfaces (IAudioOutput, ICdDrive, IGpioMonitor, IDisplayControl) are consumed by Phases 4, 5, 7, and 9 respectively.
- AppConfig struct is consumed by every phase that reads configuration.
- State objects (Phase 2) will be registered as QML singletons via the binding surface established here.

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 01-foundation-and-build-infrastructure*
*Context gathered: 2026-02-28*
