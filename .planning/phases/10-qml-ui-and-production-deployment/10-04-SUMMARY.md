---
phase: 10-qml-ui-and-production-deployment
plan: 04
subsystem: ui, infra
tags: [qml, systemd, kiosk, eglfs, deployment, animations]

# Dependency graph
requires:
  - phase: 10-01
    provides: Theme.qml singleton and main.qml production layout
  - phase: 10-02
    provides: InputCarousel, NowPlaying, PlaybackControls components
  - phase: 10-03
    provides: LibraryBrowser, SpotifySearch, AlphabetSidebar, SimpleKeyboard
provides:
  - Polished main.qml with smooth view transitions and consistent glass aesthetic
  - systemd service unit with auto-restart and EGLFS environment
  - Kiosk install/uninstall scripts with auto-login, cursor hiding, screen blanking
affects: []

# Tech tracking
tech-stack:
  added: [systemd, unclutter, eglfs_kms]
  patterns: [kiosk deployment, systemd user service, auto-restart on crash]

key-files:
  created:
    - deploy/media-console.service
    - deploy/install-kiosk.sh
    - deploy/uninstall-kiosk.sh
  modified:
    - src/qml/main.qml

key-decisions:
  - "OutCubic easing on all overlay fade transitions for consistent glass aesthetic"
  - "Loader opacity crossfade for smooth view switching instead of instant swap"
  - "VolumeIndicator.qml omitted from CMakeLists.txt (volume is inline in main.qml, file never created)"
  - "systemd user service with EGLFS platform and eglfs_kms integration for direct framebuffer rendering"
  - "install-kiosk.sh modifies .bashrc for unclutter (SSH sessions excluded)"

patterns-established:
  - "Kiosk deployment: systemd service + install/uninstall scripts in deploy/ directory"
  - "All overlay animations use Easing.OutCubic for entries"

requirements-completed: [UI-13, UI-14, UI-15, PROD-01, PROD-02, PROD-03]

# Metrics
duration: 3min
completed: 2026-02-28
---

# Phase 10 Plan 4: UI Polish and Production Deployment Summary

**Smooth view transitions with OutCubic easing, systemd kiosk service with EGLFS auto-restart, and install/uninstall deployment scripts**

## Performance

- **Duration:** 3 min
- **Started:** 2026-02-28T22:22:42Z
- **Completed:** 2026-02-28T22:25:26Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Added smooth opacity crossfade transitions when switching views via Loader
- Applied consistent OutCubic easing to all overlay animations (volume, toast, takeover, error dialogs)
- Created systemd service unit with Restart=always, EGLFS environment, and resource limits
- Created install-kiosk.sh with service enablement, auto-login, cursor hiding, screen blanking, default config
- Created uninstall-kiosk.sh for clean reversal of kiosk configuration

## Task Commits

Each task was committed atomically:

1. **Task 1: Polish UI integration** - `0c7d275` (feat)
2. **Task 2: Create systemd service unit and deployment scripts** - `087015f` (feat)

## Files Created/Modified
- `src/qml/main.qml` - Added Loader crossfade opacity transition, OutCubic easing on all overlay animations
- `deploy/media-console.service` - systemd user service with EGLFS, auto-restart, resource limits
- `deploy/install-kiosk.sh` - Kiosk install: service, auto-login, unclutter, screen blanking, default config (108 lines)
- `deploy/uninstall-kiosk.sh` - Kiosk uninstall: stop/disable service, remove auto-login (36 lines)

## Decisions Made
- Used OutCubic easing consistently on all overlay fade transitions for polished glass aesthetic
- Loader opacity crossfade provides smooth view switching without the complexity of dual-Loader crossfade
- VolumeIndicator.qml was listed in the plan's CMakeLists.txt template but was never created as a separate component (volume overlay is inline in main.qml) -- skipped adding non-existent file
- systemd service uses EGLFS platform with eglfs_kms integration for direct framebuffer rendering on Raspberry Pi
- install-kiosk.sh excludes SSH sessions from unclutter activation via SSH_CONNECTION check

## Deviations from Plan

None - plan executed exactly as written. The ErrorBanner, EjectButton visibility, SearchButton visibility, status bar separator, and connection indicator were already implemented in previous plans (10-01). This plan added the remaining polish (view transitions, easing refinements) and deployment scripts.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 10 complete: all 4 plans executed
- Application has full QML touch UI with Theme singleton, InputCarousel, NowPlaying, PlaybackControls, LibraryBrowser, SpotifySearch
- Production deployment ready via deploy/install-kiosk.sh on target Raspberry Pi
- All 9 QML files registered in CMakeLists.txt qt_add_qml_module

## Self-Check: PASSED

All files found, all commits verified.

---
*Phase: 10-qml-ui-and-production-deployment*
*Completed: 2026-02-28*
