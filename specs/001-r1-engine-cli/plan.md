# Implementation Plan: R1 Engine and CLI

**Branch**: `001-r1-engine-cli` | **Date**: 2026-09-05 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/001-r1-engine-cli/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command; its definition describes the execution workflow.

## Summary

R1 is a headless Plasma Wayland CLI that computes 2× HiDPI “looks like” canvases
from four named presets and applies them through libkscreen in two phases
(register custom mode, reload, switch + scale 2.00). It lists outputs by
connector name, reverts to the preferred EDID mode while keeping the saved
preset, and restores last successful profiles with `--apply-saved`. Math is
unit-tested against Fixture A/B; the compositor Fake backend is not used.

## Technical Context

**Language/Version**: C++20

**Primary Dependencies**: Qt 6 Core (CLI; no Widgets), KF6Screen 6.6
(`KF6::Screen`: `GetConfigOperation`, `SetConfigOperation`, `Output`)

**Storage**: JSON file under `QStandardPaths::AppConfigLocation`
(`~/.config/kscaling/profiles.json`)

**Testing**: Qt Test + CTest. `ResolutionMath` unit tests (no compositor).
`DisplayBackend` mock for apply/revert/list orchestration. Live DP-3 is an
author gate, not CI.

**Target Platform**: KDE Plasma 6.6+ Wayland (author box: openSUSE, 6.6.4)

**Project Type**: CLI (desktop display utility; tray is R2)

**Performance Goals**: A full apply (two config round-trips) completes in
under 5 seconds on a local session; no throughput target.

**Constraints**: Plasma 6.6 symbols only (no `add_cvt` / protocol v22). No
`QProcess` to `kscreen-doctor`. No guessed mode ids. Abort if
`Capability::CustomModes` missing. GPL-3.0-or-later, SPDX on every file, no
telemetry.

**Scale/Scope**: One binary, four presets, one saved profile map keyed by
connector name, no GUI.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Gate | Plan |
|-----------|------|------|
| I. Looks-like first | PASS | Users pass preset ids; engine derives canvas |
| II. Connector identity | PASS | `Output::name()` only; never persist numeric id |
| III. libkscreen display API | PASS | `LibKScreenBackend` default; doctor debug-only |
| IV. Two-phase apply | PASS | Register → fresh GetConfig → switch; never one-phase |
| V. Revert is a feature | PASS | `--revert` and failed phase B restore preferred mode |
| VI. Custom modes ephemeral | PASS | `--apply-saved` in R1; autostart file is R3 |
| VII. Sharp by default | PASS | Scale 2.00 only |
| VIII. Discover before specifying | PASS | Bound to `docs/discovery-r1.md` |
| IX. Small surface + GPL | PASS | CLI only; SPDX; no telemetry; no virtual/EDID/X11 |

Post-design re-check: PASS. Contracts and data model do not add a doctor
runtime, one-phase apply, numeric ids, fractional scale, or tray.

## Project Structure

### Documentation (this feature)

```text
specs/001-r1-engine-cli/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
└── tasks.md             # Phase 2 — NOT created by /speckit.plan
```

### Source Code (repository root)

```text
CMakeLists.txt
LICENSE
src/
├── main.cpp
├── core/
│   ├── ResolutionMath.h
│   ├── ResolutionMath.cpp
│   ├── Preset.h
│   ├── Profile.h
│   └── Settings.cpp
├── backend/
│   ├── DisplayBackend.h
│   ├── LibKScreenBackend.cpp
│   └── MockDisplayBackend.cpp   # tests only (or tests/)
└── cli/
    └── Cli.cpp
tests/
├── ResolutionMathTest.cpp
└── ApplyOrchestratorTest.cpp
```

**Structure Decision**: Single CMake project as in `KSCALING-v2.md`. `core`
has no KF6Screen dependency. `backend` implements `DisplayBackend`. `cli`
parses args and prints. No `src/ui` in R1.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

None.
