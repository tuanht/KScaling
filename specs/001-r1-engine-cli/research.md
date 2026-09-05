# Research: R1 Engine and CLI

Source of truth: `docs/discovery-r1.md`, constitution v1.0.0, spec
`001-r1-engine-cli`. No Technical Context items remain NEEDS CLARIFICATION.

## Display API

- Decision: Default backend is KF6Screen 6.6 (`GetConfigOperation` /
  mutate `Output` / `SetConfigOperation`). `kscreen-doctor` is debug and
  fixture capture only.
- Rationale: Doctor is already a thin wrapper around `setCustomModes`.
  Constitution III forbids `QProcess` doctor as runtime.
- Alternatives considered: Doctor CLI backend (v1; rejected). One-phase
  `setCustomModes` + `setCurrentModeId` (null deref on 6.6; rejected).

## Apply sequence

- Decision: Two-phase. Always fresh `GetConfigOperation` after phase A.
  Skip phase A when a custom mode of the same size and `qRound(refreshRate)`
  already exists. `setCustomModes` sends the full list (protocol replace).
  Native size is `preferredMode()`, not `currentMode()`. R1 CLI may
  `ConfigOperation::exec()`.
- Rationale: Wayland mode objects do not exist until after `set_custom_modes`
  is applied. `SetConfigOperation::config()` is the same pointer, not a
  refresh.
- Alternatives considered: Merge A+B (rejected). Fake backend CustomModes
  (parser does not implement it; rejected).

## Identity and persistence

- Decision: Key outputs by `Output::name()` (`DP-3`). Persist last successful
  preset, canvas WxH, hz, and originalScale. Original scale is written only
  when missing for that connector. Revert does not clear preset/canvas/hz.
- Rationale: Numeric ids move. Clarify session 2026-09-05.
- Alternatives considered: uuid/hash in R1 (deferred). Clear profile on
  revert (rejected). Overwrite originalScale on every apply (rejected).

## Refresh heuristic

- Decision: If `mode_w ≥ 2 × native_w` → request 60 Hz, else cap 120 Hz.
  Flags: `ModeInfo::Flag::Custom` only (no ReducedBlanking). On reject:
  report `errorString()`, revert preferred mode, do not loop Hz.
- Rationale: Constitution. Fixture A already has 4096@~60 and 5120@~120
  leftovers; heuristic is unproven but must not become a retry loop.
- Alternatives considered: Match fixture leftover Hz (not portable). Agent
  retry 120 vs 60 (forbidden).

## snap8

- Decision: `looks_like_w = round_nearest_multiple_8(native_w / ui_scale)`;
  `looks_like_h = round_nearest_multiple_8(looks_like_w * native_h / native_w)`
  so aspect is preserved. Canvas = looks_like × 2. `large` ui_scale is 1.778.
- Rationale: Fixture A `large` is 1440×810. Independent snap of both axes
  can drift aspect on odd natives.
- Alternatives considered: Snap both axes independently (can break aspect).
  `ui_scale = native_w / 1440` only on Fixture A (not portable).

## Testing

- Decision: Qt Test + CTest for math and a hand-written `DisplayBackend`
  mock. Live DP-3 verification is manual (quickstart).
- Rationale: Discovery: Fake backend cannot test CustomModes.
- Alternatives considered: KScreen Fake `.so` (incomplete). Live-only tests
  (not CI-safe).

## Build

- Decision: CMake, C++20, `find_package(Qt6 REQUIRED COMPONENTS Core)`,
  `find_package(KF6Screen REQUIRED)`, link `KF6::Screen`. Qt Widgets not
  linked in R1.
- Rationale: v2 architecture; R1 CLI only.
- Alternatives considered: qmake (rejected). Widgets in R1 (out of scope).

## Config path

- Decision: `{QStandardPaths::AppConfigLocation}/profiles.json` i.e.
  `~/.config/kscaling/profiles.json` with `version: 1` and `outputs` map.
  No `applyOnLogin` / `applyDelayMs` in R1 (those are R3).
- Rationale: Matches v1 path; R1 spec has no autostart delay.
- Alternatives considered: QSettings native format (less inspectable).
  Include R3 keys unused (YAGNI).
