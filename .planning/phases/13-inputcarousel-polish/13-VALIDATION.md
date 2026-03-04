---
phase: 13
slug: inputcarousel-polish
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-04
---

# Phase 13 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | GoogleTest v1.17.0 + QSignalSpy (Qt 6) |
| **Config file** | tests/CMakeLists.txt (FetchContent) |
| **Quick run command** | `cd /Users/rory/Code/media-console-v2/build && ctest -R "UIState|AppBuilder" --output-on-failure` |
| **Full suite command** | `cd /Users/rory/Code/media-console-v2/build && ctest --output-on-failure` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cd /Users/rory/Code/media-console-v2/build && ctest -R "UIState|AppBuilder" --output-on-failure`
- **After every plan wave:** Run `cd /Users/rory/Code/media-console-v2/build && ctest --output-on-failure`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 13-01-T1 | 01 | 0 | UIState bridge | unit | `ctest -R UIState --output-on-failure` | ❌ Wave 0 | ⬜ pending |
| 13-01-T2 | 01 | 0 | AppBuilder wiring | unit | `ctest -R AppBuilder --output-on-failure` | ❌ Wave 0 | ⬜ pending |
| CAR-01 | — | — | Smooth rotation | manual-only | N/A — Pi 5 visual inspection | N/A | ⬜ pending |
| CAR-02 | — | — | Natural snap | manual-only | N/A — touch/encoder + visual | N/A | ⬜ pending |
| CAR-03 | — | — | Depth de-emphasis | manual-only | N/A — visual on device | N/A | ⬜ pending |
| CAR-04 | — | — | Encoder 1:1 lag | manual-only | N/A — physical encoder + visual | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/test_UIState.cpp` — add tests: `inputNextRequestedEmits`, `inputPreviousRequestedEmits`, `inputSelectRequestedEmits` (same pattern as existing `showToastSignalEmits` test at line 136)
- [ ] `tests/test_AppBuilder.cpp` — add test verifying GPIO inputNext wired to UIState signal, not ReceiverController directly (best effort — UIState signal tests are the critical coverage)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| No frame drops during carousel rotation | CAR-01 | Requires Pi 5 hardware + visual inspection at display refresh rate | On Pi 5: open carousel, swipe continuously for 5+ seconds, verify no visible jank |
| Natural deceleration snap to nearest input | CAR-02 | Requires touch/encoder gesture + visual inspection of snap curve | Release gesture mid-rotation, verify snap is smooth with deceleration (not instant, not elastic) |
| Non-selected items visibly smaller and less opaque | CAR-03 | Visual/perceptual check on device | Open carousel, confirm center card is full-size/full-opacity, flanking cards are visibly smaller and dimmer |
| Encoder drives carousel with no perceptible lag | CAR-04 | Requires physical encoder hardware | Turn encoder one detent, verify carousel moves exactly one step with no delay |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
