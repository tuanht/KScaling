# KScaling

**A small BetterDisplay-like tray app for KDE Plasma Wayland.**  
It calculates 2× / 200% HiDPI “looks like” modes from the current monitor and applies them with `kscreen-doctor`, including on login.

| Field | Value |
|---|---|
| Name | **KScaling** |
| Version target | 0.1 (MVP) |
| Platform | KDE Plasma 6.6+ Wayland |
| UI toolkit | Qt 6 |
| License | **GPL-3.0-or-later** |
| Status | Concept / seed spec |
| Created | 2026-09-05 |

This file is the product concept and the seed for **GitHub Spec Kit** (`/speckit.constitution`, `/speckit.specify`, `/speckit.plan`) and **Superpowers** (`brainstorming` → `writing-plans` → implement). It is not the final `spec.md`; it is the source those tools should read first.

---

## How to use this document

### Spec Kit

From a new repo after `specify init kscaling`:

1. `/speckit.constitution` — fold in [Project constitution](#project-constitution)
2. `/speckit.specify` — paste the one-liner plus [User scenarios](#user-scenarios--testing) and [Functional requirements](#functional-requirements)
3. Walk [Roadmap slices](#roadmap-slices-for-spec-kit) one entry at a time (`R1` → `R5`)
4. Each slice gets its own `specs/<slice>/spec.md`, `plan.md`, `tasks.md`

Every sub-spec should say:

> **Input**: Parent roadmap: `KSCALING.md` → entry **R#**.

### Superpowers

1. Skill `brainstorming` — treat this file as the approved design; only ask about [Open questions](#open-questions)
2. Skill `writing-plans` — start with **R1** (engine + CLI backend, no GUI)
3. Skill `test-driven-development` — formula + JSON parser first
4. Do not invent virtual screens, EDID hacks, or X11 backends in v1

### Origin context

KScaling replaces two existing artifacts:

- `kscreen_doctor_guide.md` — manual `kscreen-doctor` recipes for DP-3
- `apply_custom_res.sh` — login script that injects 4096×2304@60 and switches to it

The first monitor fixture is a **2560×1440** panel on connector **DP-3**, Plasma scale locked at **200%**.

---

## One-liner

KScaling detects the native resolution of the current Plasma Wayland output, derives 2× HiDPI custom modes from UI-size percentages, applies the chosen mode through `kscreen-doctor`, and reapplies the last profile at login.

---

## Problem

Plasma’s default 200% scale on a 2560×1440 panel yields a logical desktop of 1280×720. UI is sharp but huge; workspace is tiny.

macOS (and BetterDisplay) keep 2× backing pixels and change the **logical** “looks like” size. On Linux, Plasma 6.6+ can inject custom modes:

```text
kscreen-doctor output.DP-3.addCustomMode.4096.2304.60000.full
kscreen-doctor output.DP-3.scale.2
kscreen-doctor output.DP-3.mode.4096x2304@60
```

Those modes do **not** persist across reboot or sleep. A tiny app that computes, applies, and restores them is the product.

---

## Goals and non-goals

### Goals (v1)

- Work on the user’s current output without hard-coding `DP-3`
- Compute HiDPI modes from native size + UI-scale percentage
- Ship four named presets that reproduce the 2560×1440 table below
- Apply and revert through `kscreen-doctor`
- Restore the last good profile on login
- Live in the system tray
- Be readable, small, and GPL-3.0-or-later

### Non-goals (v1)

- Virtual / dummy displays
- EDID override
- DDC/CI brightness
- HDR, ICC, 10-bit color pipeline
- X11 or non-Plasma compositors
- Linking `libkscreen` (CLI wrapper only)
- Multi-seat, mirroring, or layout editors
- Windows / macOS

---

## Project constitution

Use these as KScaling’s constitution principles.

### I. Looks-like first

Users pick a **logical** desktop size (or a UI-scale %). The app derives the 2× framebuffer mode. Never ask the user to invent a raw modeline unless they opt into Advanced.

### II. Connector identity, not numeric id

Persist and address outputs by connector name (`DP-3`, `eDP-1`, `HDMI-A-1`). Numeric KScreen ids move across boots.

### III. CLI is the backend

v1 talks to the compositor only through `kscreen-doctor` (`-j` for read, dotted properties for write). A future `libkscreen` backend must keep the same `DisplayBackend` interface.

### IV. Revert is a feature

Every apply path has a revert-to-preferred-EDID-mode path. A rejected mode must not leave the session unusable.

### V. Custom modes are ephemeral

KWin custom modes vanish after reboot and often after sleep. Autostart / `--apply-saved` is part of the core product, not an add-on.

### VI. Sharp by default

v1 always targets Plasma scale **2.00**. Do not mix 125%/150%/175% compositor scale with custom modes in MVP.

### VII. Small surface

Tray + compact window + optional CLI flags. No settings cathedral.

### VIII. Open source by default

GPL-3.0-or-later. SPDX headers on every source file. No proprietary telemetry.

---

## Core model

Keep compositor scale at **2.00**. Vary only the custom mode.

```text
native      = panel preferred / native mode     e.g. 2560×1440
ui_scale    = apparent UI size vs native 1:1    e.g. 1.25
looks_like  = native / ui_scale                 logical workspace
mode        = looks_like × 2                    custom HiDPI mode
```

Worked example (the reason 25% is “perfect”):

```text
2560 / 1.25 = 2048
2048 × 2    = 4096
1440 / 1.25 = 1152
1152 × 2    = 2304
→ 4096×2304 @ 200% looks like 2048×1152
→ UI is 125% of native 1:1 (25% larger than 1:1, much smaller than native@200%)
```

Native @ 200% without a custom mode is logical **1280×720** (UI 200% of 1:1). All KScaling presets sit between that extreme and true 1:1 HiDPI.

### Rounding and aspect

```text
looks_w = snap8(native_w / ui_scale)
looks_h = snap8(looks_w * native_h / native_w)   # keep panel aspect
mode_w  = looks_w * 2
mode_h  = looks_h * 2
```

`snap8` = nearest multiple of 8, minimum 640×360. Prefer rounding toward even values that stay 16:9 when the panel is 16:9.

### Refresh-rate heuristic

Higher framebuffer × higher Hz hits DisplayPort bandwidth.

- If `mode_w ≥ 2 × native_w` (5K-class buffer on a 1440p panel) → **60 Hz** (`60000` millihertz)
- Else → prefer the output’s current max, capped at **120 Hz** (`120000`)
- Always add the mode with `.full` (fill the panel; do not letterbox)

After `addCustomMode`, re-read `kscreen-doctor -j` and switch using the **actual** `@Hz` string KWin registered (it may be 59.94, not 60).

### Two equivalent UI labels

Show both; they are the same number:

- Looks like **2048×1152**
- UI **125%** of native 1:1

---

## Built-in presets

Percent labels match the original guide. `ui_scale` is the precise input to the formula.

| Id | Name | Meaning | `ui_scale` | 2560×1440 looks like | 2560×1440 mode @ 2× | Guide label |
|---|---|---|---|---|---|---|
| `perfect` | Perfect | 25% off 1:1 | **1.25** | 2048×1152 | **4096×2304** | Perfect Fit (25%) |
| `max-space` | Max space | 1:1 HiDPI | **1.00** | 2560×1440 | **5120×2880** | 44% smaller UI |
| `comfort` | Comfort | larger UI | **1.60** | 1600×900 | **3200×1800** | 20% larger UI |
| `large` | Large | largest UI | **1.778** | 1440×810 | **2880×1620** | 33% larger UI |

Preset ids are stable strings. Do not rename them in config.

A later slider may expose `ui_scale` from **1.00 to 2.00** continuously; v1 can ship presets only.

---

## User scenarios and testing

### User Story 1 — Apply Perfect Fit on this monitor (P1)

A Plasma Wayland user on a 2560×1440 display opens KScaling, sees DP-3 detected as 2560×1440, clicks **Perfect**, and the desktop becomes a sharp 2048×1152 logical workspace at 200% scale.

**Why P1:** This is the whole product. If this fails, nothing else matters.

**Independent test:** On a fixture output whose native mode is 2560×1440, applying `perfect` issues `addCustomMode.4096.2304.<hz>.full`, sets scale 2, and activates `4096x2304@<hz>`.

### User Story 2 — Survive login (P1)

The user enables **Apply on login**, reboots, and gets the last preset without running a shell script.

**Why P1:** Custom modes are ephemeral. Login restore is required for daily use.

**Independent test:** Write a profile to config, run `kscaling --apply-saved`, observe the saved mode become current after outputs are up.

### User Story 3 — Switch presets and revert (P1)

The user tries Max space, then Comfort, then **Revert native**. The panel returns to the EDID preferred mode. Scale remains 200% unless revert is defined to leave scale untouched (see open questions).

**Why P1:** Experimenting is unsafe without an undo.

**Independent test:** After two preset applies, revert restores the preferred non-custom mode listed by `kscreen-doctor`.

### User Story 4 — Multi-output awareness (P2)

A laptop (eDP-1) plus DP-3 shows both outputs. The user applies Perfect only on DP-3. eDP-1 is unchanged.

**Why P2:** Connector-scoped profiles are how this survives docking.

**Independent test:** Two connected outputs; apply on one; the other output’s mode id is unchanged.

### User Story 5 — Headless apply for autostart (P2)

`kscaling --apply-saved` works with no tray and no `$DISPLAY` issues beyond a running `kwin_wayland` / KScreen backend.

**Why P2:** Autostart must not depend on the window being visible.

### User Story 6 — Continuous slider (P3)

A slider from 100% to 200% UI live-updates the computed mode line, then Apply commits it.

**Why P3:** Presets cover the original four percentages; the slider is BetterDisplay-like polish.

---

## Functional requirements

- **FR-001**: KScaling MUST detect connected Plasma outputs and display connector name, native/preferred resolution, current mode, and current scale.
- **FR-002**: KScaling MUST compute `looks_like` and `mode` from native size and `ui_scale` using the formula in [Core model](#core-model).
- **FR-003**: For native 2560×1440, presets MUST produce the exact modes in [Built-in presets](#built-in-presets) after `snap8` (already aligned).
- **FR-004**: Apply MUST (1) add the custom mode if missing, (2) set scale to 2.00, (3) switch to that mode using the Hz string KWin actually registered.
- **FR-005**: KScaling MUST persist last-used profile per connector name in a user config file.
- **FR-006**: `kscaling --apply-saved` MUST re-add and re-activate saved modes after a short configurable delay (default 2 seconds).
- **FR-007**: KScaling MUST offer Revert to the output’s preferred EDID mode.
- **FR-008**: KScaling MUST identify custom modes by `WxH@Hz`, never by `removeCustomMode` index, when matching or cleaning.
- **FR-009**: If KWin rejects a configuration, KScaling MUST surface the error and attempt revert to the last known good mode.
- **FR-010**: Autostart MUST be installable as an XDG `.desktop` in `~/.config/autostart/`.
- **FR-011**: The tray MUST expose the four presets, current looks-like text, Apply on login toggle, and Revert.
- **FR-012**: KScaling MUST NOT hard-code `DP-3`; DP-3 is only the first fixture.

---

## Success criteria

- **SC-001**: On the 2560×1440 DP-3 fixture, one click on Perfect yields logical 2048×1152 at scale 2 within 3 seconds of a successful KWin apply.
- **SC-002**: After reboot with Apply on login enabled, the saved mode is active without any manual terminal command.
- **SC-003**: Revert returns a usable native/preferred mode on the same output.
- **SC-004**: Formula unit tests pass for 1920×1080, 2560×1440, 3440×1440, and 3840×2160 natives for all four preset `ui_scale` values (aspect preserved, multiples of 8).
- **SC-005**: A new contributor can build and run the tray from README in one sitting on Plasma 6.6+.

---

## Architecture

```text
┌─────────────────────────────────────┐
│  app/                               │
│  Tray + compact MainWindow (Qt 6)   │
│  CLI: --apply-saved, --apply <id>   │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│  core/ProfileEngine                 │
│  native + ui_scale → looks + mode   │
│  snap8, aspect, Hz heuristic        │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│  backend/KScreenDoctorBackend       │
│  parse `kscreen-doctor -j`          │
│  addCustomMode / scale / mode       │
│  revert, list custom modes          │
└──────────────────┬──────────────────┘
                   │
            kscreen-doctor
                   │
              KWin + KScreen
```

v1 dependency choice: **Qt 6 Widgets** (or Quick if the implementer prefers; Widgets is enough for a tray). No KF6 required. Spawn `kscreen-doctor` via `QProcess`.

Suggested source layout:

```text
kscaling/
├── CMakeLists.txt
├── LICENSE                 # GPL-3.0-or-later
├── README.md
├── KSCALING.md             # this file
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── ResolutionMath.h/.cpp
│   │   ├── Profile.h
│   │   └── Settings.h/.cpp
│   ├── backend/
│   │   ├── DisplayBackend.h
│   │   └── KScreenDoctorBackend.h/.cpp
│   └── ui/
│       ├── Tray.h/.cpp
│       └── MainWindow.h/.cpp
├── autostart/
│   └── kscaling.desktop.in
└── tests/
    └── ResolutionMathTest.cpp
```

---

## Apply sequence

For output `DP-3`, preset `perfect`, 60 Hz:

```bash
kscreen-doctor output.DP-3.addCustomMode.4096.2304.60000.full
kscreen-doctor output.DP-3.scale.2
kscreen-doctor output.DP-3.mode.4096x2304@60
```

Notes:

- Refresh argument to `addCustomMode` is **millihertz**: 60 Hz → `60000`, 120 Hz → `120000`.
- Last token is `full` or `reduced`. v1 always uses `full`.
- `kscreen-doctor` accepts connector names or numeric ids. Prefer names.
- Read state with `kscreen-doctor -j` (JSON) rather than scraping `-o` text.
- Removing a custom mode uses an index that is **not stable**. Only use `removeCustomMode` after reading the current custom-mode list in the same process.

Known platform facts to encode in comments / README:

- Custom modes need Plasma 6.6+ / libkscreen custom-mode support.
- Custom modes do not persist across reboot; often not across sleep.
- KWin may reject a mode (`applying config failed! The driver rejected the output configuration`).
- Oversized modes cost GPU fillrate; 5120×2880@120 on a 1440p panel is the risky combination.

---

## Config

Path: `~/.config/kscaling/profiles.json`  
(or `QStandardPaths::AppConfigLocation`)

```json
{
  "version": 1,
  "scale": 2.0,
  "applyOnLogin": true,
  "applyDelayMs": 2000,
  "outputs": {
    "DP-3": {
      "preset": "perfect",
      "uiScale": 1.25,
      "mode": [4096, 2304],
      "hz": 60
    }
  }
}
```

Rules:

- Key in `outputs` is the connector name.
- If both `preset` and `uiScale` exist, `uiScale` wins (slider / custom).
- `mode` + `hz` are the last **successfully applied** values; `--apply-saved` uses those first, and only recomputes if they are missing.

---

## CLI

```text
kscaling                     # tray + window
kscaling --apply-saved       # autostart / headless restore
kscaling --apply perfect     # apply named preset on the primary/focused output
kscaling --list              # print parsed outputs (debug)
kscaling --revert            # preferred mode on primary/focused output
```

Exit codes: `0` ok, `1` backend/parse error, `2` apply rejected, `3` no outputs.

---

## UI sketch

```text
KScaling
─────────────────────────────────────────
DP-3  ·  native 2560×1440  ·  scale 200%
Now: 4096×2304 @ 60  →  looks like 2048×1152

[ Perfect 125% ] [ Max 100% ] [ Comfort 160% ] [ Large 178% ]

Looks like   2048 × 1152
UI vs 1:1    125%
Hz           Auto (60)

[ Apply ]    [ Revert native ]    ☑ Apply on login
```

Tray icon tooltip = connector + looks-like. Left-click opens the compact window. Context menu: four presets, revert, quit.

No images required for v1. Keep the window small enough to feel like a utility, not a control center.

---

## Autostart

`~/.config/autostart/kscaling.desktop`

```ini
[Desktop Entry]
Type=Application
Name=KScaling
Comment=Restore HiDPI custom modes on login
Exec=kscaling --apply-saved
Icon=kscaling
Terminal=false
Categories=Qt;KDE;Settings;
X-KDE-AutostartPhase=2
X-GNOME-Autostart-enabled=true
```

`--apply-saved` waits `applyDelayMs` (default 2000) so KWin/KScreen exist, then applies each saved connector that is currently connected.

Sleep/resume restore is **out of v1** unless it falls out of the same autostart path. A later slice can listen to logind `PrepareForSleep`.

---

## License and publishing

- SPDX license identifier: `GPL-3.0-or-later`
- Add `LICENSE` with the full GPL-3.0 text
- Every source file:

```c
// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors
```

- README must include license, Plasma 6.6+ requirement, and a short safety note: custom modes can be rejected by the driver; Revert exists for a reason
- Suggested host: GitHub / Codeberg under a `kscaling` repo
- Suggested metadata: `io.github.<user>.KScaling` when an AppStream file is added (not v1-blocking)

Why GPL-3.0-or-later: Qt/KDE companion utilities conventionally use GPL; it matches the stack users already run. Switch to MIT only if the maintainer explicitly wants proprietary forks.

---

## Roadmap slices for Spec Kit

Parent roadmap: this file.

| Id | Name | Intent | In | Out | Depends | Status |
|---|---|---|---|---|---|---|
| **R1** | Resolution engine + doctor backend | Compute modes; parse `-j`; apply/revert via CLI; `--list` / `--apply` / `--revert` / `--apply-saved` with no GUI | Formula, presets, JSON config, QProcess backend, tests for 1440p fixture | Tray, slider, autostart desktop install UI | — | todo |
| **R2** | Tray + compact window | Four presets, current state, apply, revert, apply-on-login checkbox | R1 public API | Slider, multi-output picker polish | R1 | todo |
| **R3** | Login restore packaging | Install/remove XDG autostart file; delay; README | `--apply-saved` from R1 | Sleep/resume watcher | R1 | todo |
| **R4** | Multi-output profiles | Per-connector apply; ignore disconnected connectors on restore | Settings map keyed by connector | Dock hotplug animations | R1, R2 | todo |
| **R5** | Looks-like slider | Continuous `ui_scale` 1.00–2.00; live preview line | Preset math | BetterDisplay extras | R2 | todo |

Implement **R1 first**. It already replaces `apply_custom_res.sh`.

---

## Test fixtures

Use these as frozen golden values.

### Fixture A — 2560×1440 (author’s panel)

| preset | ui_scale | looks_like | mode |
|---|---|---|---|
| perfect | 1.25 | 2048×1152 | 4096×2304 |
| max-space | 1.00 | 2560×1440 | 5120×2880 |
| comfort | 1.60 | 1600×900 | 3200×1800 |
| large | 1.778 | 1440×810 | 2880×1620 |

`large` ui_scale is `2560/1440 = 1.7̄` — store as `1.778` rounded to 3 decimals, then snap. For this panel the division is exact: `2560/1.778 ≈ 1440`. Prefer implementing `large` as **looks_like height = native_w / 16×9 chain** or as `ui_scale = native_w / 1440` only on this fixture; in general `large` = `ui_scale 1.778` with snap8.

Safer definition for portable presets:

- `perfect` = 1.25
- `max-space` = 1.00
- `comfort` = 1.60
- `large` = 1.78 (snap8 after divide)

Document the golden table above as the acceptance target for Fixture A.

### Fixture B — 1920×1080

`perfect` (1.25) → looks like 1536×864 → mode 3072×1728.

### Fixture C — 3840×2160

`perfect` (1.25) → looks like 3072×1728 → mode 6144×3456.  
Hz heuristic should force 60 Hz. Driver rejection is likely; tests should still compute the numbers.

---

## Open questions

Resolve these in `/speckit.clarify` or Superpowers brainstorming before R2.

1. On **Revert**, should scale stay at 200% or also reset to whatever the user had before KScaling touched it?
2. Primary output vs last-focused output for `--apply <preset>` when several displays are connected?
3. Clean up unused custom modes after a successful switch, or leave them so switching presets is faster?
4. Widgets vs Qt Quick for the compact window?
5. C++ only, or Python + PySide6 for faster v1? (Recommendation: **C++ Qt 6** if this should feel like a Plasma citizen; **PySide6** if the author wants a weekend MVP.)
6. App id / D-Bus name: `io.github.<user>.KScaling` vs `org.kscaling.KScaling`?

Defaults if nobody answers:

1. Revert mode only; leave scale at 2.00
2. Prefer the output that already has a saved profile; else the current primary
3. Leave custom modes in place; offer “Remove unused modes” later
4. Widgets
5. C++ Qt 6
6. `org.kscaling.KScaling`

---

## Agent kickoff prompts

### Spec Kit

```text
/speckit.constitution
Use KSCALING.md section “Project constitution” as the project constitution
for KScaling (GPL-3.0-or-later Plasma Wayland HiDPI tray).
```

```text
/speckit.specify
Build KScaling slice R1 from KSCALING.md: Resolution engine + kscreen-doctor
backend. Headless CLI only. Must reproduce Fixture A golden modes for a
2560×1440 panel and apply them with kscreen-doctor. No GUI in this slice.
```

### Superpowers

```text
Read KSCALING.md. Using the brainstorming skill only for the Open questions
(use the documented defaults unless I object). Then write an implementation
plan for roadmap entry R1 only.
```

---

## Reference commands (from the original guide)

```bash
# Add
kscreen-doctor output.DP-3.addCustomMode.4096.2304.60000.full
kscreen-doctor output.DP-3.addCustomMode.5120.2880.60000.full
kscreen-doctor output.DP-3.addCustomMode.3200.1800.120000.full
kscreen-doctor output.DP-3.addCustomMode.2880.1620.120000.full

# Switch
kscreen-doctor output.DP-3.mode.4096x2304@60
kscreen-doctor output.DP-3.scale.2

# Inspect
kscreen-doctor --outputs
kscreen-doctor -j

# Remove (index is session-specific)
kscreen-doctor output.DP-3.removeCustomMode.YOUR_INDEX_NUMBER
```

Inconsistencies in the original attachments (do not copy blindly):

- Guide adds 5120×2880 at **60 Hz**; `apply_custom_res.sh` adds it at **120 Hz**. KScaling heuristic uses **60 Hz**.
- Guide’s “activate Perfect Fit” example used `@120`; add commands and the script use **60 Hz**. KScaling uses **60 Hz** for 4096×2304.
- Script never added 2880×1620. KScaling preset `large` does.

---

## Glossary

| Term | Meaning |
|---|---|
| Native | Panel preferred resolution from EDID / KScreen |
| Looks like | Logical desktop size after 200% scale (`mode / 2`) |
| Mode / framebuffer | Custom resolution injected into KWin |
| ui_scale | Apparent UI size versus native 1:1. `1.25` = 25% larger UI than 1:1 |
| Connector | Stable output name (`DP-3`), not numeric id |
| Custom mode | KWin/libkscreen user-added mode; ephemeral |

---

## License blurb (short)

Copyright 2026 KScaling contributors.

KScaling is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
