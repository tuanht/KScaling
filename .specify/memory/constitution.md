<!--
Sync Impact Report
- Version change: unset (template scaffold) → 1.0.0
- Modified principles: none (first ratification; template placeholders replaced)
- Added sections:
  - Core Principles I–IX (Looks-like first; Connector identity; libkscreen
    display API; Two-phase apply; Revert; Ephemeral custom modes; Sharp by
    default; Discover before specifying; Small surface and GPL)
  - Additional Constraints
  - Development Workflow
  - Governance
- Removed sections: none
- Follow-up TODOs: none
-->

# KScaling Constitution

## Core Principles

### I. Looks-like first
Users MUST choose a logical size or UI-scale percentage. The application MUST
derive the 2× canvas mode from that choice. The application MUST NOT require
users to enter raw canvas resolution as the primary input.

Rationale: The product is “how big the desktop looks,” not a mode editor.

### II. Connector identity, not numeric id
The application MUST persist and match outputs by `KScreen::Output::name()`
(connector name, e.g. `DP-3`, `eDP-1`). The application MUST NOT use numeric
KScreen output ids as identity. A later fallback hash MAY be persisted if
docking is shown to rename connectors; identity MUST still start from `name()`.

Rationale: Numeric ids are backend-local and move across processes and sessions.

### III. libkscreen is the display API
v1 MUST talk to KWin through KF6Screen only:

- `KScreen::GetConfigOperation`
- mutate `Output` (`setCustomModes`, `setScale`, `setCurrentModeId`)
- `KScreen::SetConfigOperation`
- read `hasError()` / `errorString()`
- refuse to run if `Output::Capability::CustomModes` is missing

`kscreen-doctor` is allowed as a human debug tool, a source of fixture JSON
from the author’s machine, and an optional later fallback behind the same
`DisplayBackend` interface. It MUST NOT be the default runtime implementation.
Any task that shells out via `QProcess` to `kscreen-doctor` MUST be rejected.

Rationale: `kscreen-doctor` is already a thin wrapper around the same
`Output::setCustomModes()` path. A C++ Plasma app MUST use that path directly.

### IV. Two-phase apply
Adding a custom mode and switching to it MUST NOT be assumed to work in one
`SetConfigOperation` without a reload. The application MUST follow the Apply
protocol in Additional Constraints. The application MUST NOT call
`setCurrentModeId` with a guessed id from before phase A.

Rationale: Wayland mode ids are backend-generated. On Plasma 6.6 the live
Wayland current-mode id is not a custom-mode id until after a reload.

### V. Revert is a feature
Every apply path MUST be able to restore the preferred EDID mode
(`preferredModeId()`). If phase B (`SetConfigOperation` after mode+scale) fails,
the application MUST revert to the preferred mode and report `errorString()`.
Driver rejection MUST NOT leave a dead session.

Rationale: Custom modes can be rejected by the driver or link. Recovery is
part of the product, not an afterthought.

### VI. Custom modes are ephemeral
KWin custom modes MUST be treated as non-persistent across reboot (and often
not across sleep). `--apply-saved` and login autostart MUST be treated as core
behavior, not optional polish.

Rationale: The compositor does not keep custom modes as EDID modes.

### VII. Sharp by default
v1 MUST target scale **2.00** only. Looks-like size is varied by changing the
canvas mode, not by using fractional scale.

Rationale: Integer 2× scale keeps pixels sharp. Fractional scale is out of
scope until a later slice explicitly amends this principle.

### VIII. Discover before specifying
Unknown KWin/libkscreen behavior MUST be researched and written down before
`/speckit.specify`. Agents MUST NOT invent `setCustomModes` semantics, mode-id
formats, or APIs that are not in the KF6Screen version this repo builds against.
If a symbol is needed, agents MUST read or cite `output.h`, doctor sources, or
`docs/discovery-r1.md`, or ask for a live `kscreen-doctor -j` dump.

Rationale: Unbounded design invents APIs. Discovery is cheaper than a wrong
apply path on a live session.

### IX. Small surface and GPL-3.0-or-later
The product surface MUST stay tray + compact window + app CLI. Every source
file MUST carry an SPDX `GPL-3.0-or-later` header. The application MUST NOT
include telemetry. v1 MUST NOT add virtual screens, EDID rewriting, or X11.

Rationale: Keep the app small, inspectable, and license-clear.

## Additional Constraints

Runtime MUST be KDE Plasma 6.6+ Wayland. Build MUST use Qt 6 and KF6Screen
(`KF6::Screen`). Older stacks that lack `CustomModes` MUST fail with a clear
error, not a silent no-op.

Apply protocol (MUST NOT skip a phase without written evidence):

```text
1. GetConfigOperation
2. Find output by name()
3. Abort if !capabilities().testFlag(CustomModes)
4. If target ModeInfo is not already in customModes():
     append ModeInfo{ size, refreshRate, flags: Custom }
     setCustomModes(...)
     SetConfigOperation          // phase A: register the mode
     if error → report, stop
5. GetConfigOperation            // reload — mode ids are assigned by KWin
6. Find the Mode whose size matches and refresh is nearest
7. setCurrentModeId(that id)
8. setScale(2.0)
9. SetConfigOperation            // phase B: switch canvas
10. if error → revert preferredModeId(), report
11. Persist last successful mode+hz+preset per connector
```

`ModeInfo` MUST use `Flag::Custom` without ReducedBlanking (doctor `.full`).
Hz heuristic: if `mode_w ≥ 2 × native_w` → 60 Hz, else cap at 120 Hz.

GUI code SHOULD prefer the async `finished` signal. R1 CLI MAY use
`ConfigOperation::exec()`.

Formula (non-negotiable for v1 presets):

```text
looks_like = snap8(native / ui_scale)   # keep panel aspect
mode       = looks_like × 2
scale      = 2.00
```

Presets: `perfect` 1.25, `max-space` 1.00, `comfort` 1.60, `large` 1.778.

## Development Workflow

Canonical seed is `KSCALING-v2.md`. `KSCALING.md` is historical and MUST NOT
be used as an implementation source.

Required order for R1:

1. Discovery research written to `docs/discovery-r1.md`
2. This constitution
3. Spec Kit specify for the current slice only
4. Plan / tasks
5. Superpowers writing-plans + TDD execute

Agents MUST NOT start application code before specify. Superpowers brainstorm
for this project is a discovery session bound to KDE sources and a live
fixture, not greenfield ideation. Product name, formula, 200% scale, four
presets, libkscreen default backend, and “no virtual screens / EDID / X11”
MUST NOT be reopened in brainstorm.

Pick one implementer per slice. Do not run both Superpowers execute and
`/speckit.implement` on the same slice.

A task that guesses a mode id without a post-phase-A reload MUST be rejected.

## Governance

This constitution supersedes informal notes, v1 `KSCALING.md` guidance, and
agent defaults when they conflict.

Amendments MUST be written into this file, MUST bump `CONSTITUTION_VERSION`
(MAJOR for removed/redefined principles, MINOR for new or materially expanded
principles, PATCH for clarification), MUST set `LAST_AMENDED_DATE` to the
amendment date, and MUST include an updated Sync Impact Report.

Reviews and implementation plans MUST check: libkscreen (not doctor) as
default backend, two-phase apply, connector `name()` identity, scale 2.00,
revert on failed apply, GPL-3.0-or-later / no telemetry, and discovery citations
before new compositor behavior is specified.

Complexity (one-phase apply, doctor fallback, extra identity keys, fractional
scale) MUST be justified with evidence from a live session or KF6Screen
sources before it can amend a principle.

**Version**: 1.0.0 | **Ratified**: 2026-09-05 | **Last Amended**: 2026-09-05
