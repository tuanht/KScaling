# KScaling v2 — canonical seed

**This file supersedes `KSCALING.md`.**  
v1 was a valid product sketch with a conservative `kscreen-doctor` CLI backend. v2 is the version to implement: **right backend, discovery before code, same product.**

| Field | Value |
|---|---|
| Name | **KScaling** |
| Spec generation | **v2** |
| v1 file | `KSCALING.md` (historical; do not implement from it) |
| Version target | 0.1 (MVP) |
| Platform | KDE Plasma 6.6+ Wayland |
| UI toolkit | Qt 6 Widgets (R2+); Qt 6 Core + Gui for R1 |
| Display API | **libkscreen / KF6Screen** |
| License | GPL-3.0-or-later |
| Status | Canonical seed — implement from here |
| Created | 2026-09-05 |

`kscreen-doctor` remains a **reference tool and debug aid**. It is not the v1 runtime backend.

---

## Why v2 exists

1. v1 constitution said “CLI first, libkscreen later.” That is wrong for a C++ Plasma app. `kscreen-doctor` is already a thin wrapper around `Output::setCustomModes()`.
2. You want v1 **correct**, not fastest-to-clone-the-shell-script.
3. Superpowers brainstorm can find things you have not named — but only if the session is bound to real KDE sources and a live `kscreen-doctor -j` dump from your machine. Unbounded brainstorm invents APIs.

---

## What changed from v1

| Topic | v1 | v2 |
|---|---|---|
| Display backend | `QProcess` + `kscreen-doctor` | **libkscreen** `GetConfigOperation` / `SetConfigOperation` |
| KF6 | avoided | **KF6Screen required** |
| Apply | three shell lines | two-phase config apply (add modes, reload, set mode+scale) |
| Tests | fake doctor binary | libkscreen **Fake** backend where possible + math unit tests |
| First agent step | `/speckit.specify` R1 | **Discovery brainstorm + research**, then specify R1 |
| Doctor CLI | runtime | debug / manual fallback only |

Unchanged: name, license, 200% scale, formula, four presets, Fixture A, tray later, no virtual screens / EDID / X11.

---

## Product (unchanged)

KScaling detects the native resolution of a Plasma Wayland output, derives 2× HiDPI “looks like” modes from UI-size percentages, applies the chosen mode through **libkscreen**, and restores the last profile at login.

Formula:

```text
looks_like = snap8(native / ui_scale)   # keep panel aspect
mode       = looks_like × 2
scale      = 2.00
```

| Id | ui_scale | 2560×1440 looks like | 2560×1440 mode |
|---|---|---|---|
| `perfect` | 1.25 | 2048×1152 | 4096×2304 |
| `max-space` | 1.00 | 2560×1440 | 5120×2880 |
| `comfort` | 1.60 | 1600×900 | 3200×1800 |
| `large` | 1.778 | 1440×810 | 2880×1620 |

Hz heuristic: if `mode_w ≥ 2 × native_w` → 60 Hz, else cap at 120 Hz. Always `ModeInfo::Flag::Custom` without ReducedBlanking (equivalent to doctor `.full`).

Identify outputs by **connector name** (`DP-3`), never by numeric KScreen id.

---

## Project constitution (v2)

### I. Looks-like first
Users pick a logical size or UI-scale %. The app derives the 2× mode.

### II. Connector identity, not numeric id
Persist and match outputs by `Output::name()` (`DP-3`, `eDP-1`). Ids are backend-local and move.

### III. libkscreen is the display API
v1 talks to KWin through **KF6Screen**:

- `KScreen::GetConfigOperation`
- mutate `Output` (`setCustomModes`, `setScale`, `setCurrentModeId`)
- `KScreen::SetConfigOperation`
- read `hasError()` / `errorString()`
- refuse to run if `Output::Capability::CustomModes` is missing

`kscreen-doctor` is allowed as:

- a human debug tool
- a source of fixture JSON captured on the author’s machine
- an optional later fallback backend behind the same `DisplayBackend` interface

It is **not** the default implementation.

### IV. Two-phase apply
Adding a custom mode and switching to it are not assumed to work in one `SetConfigOperation` without a reload. See [Apply protocol](#apply-protocol).

### V. Revert is a feature
Every apply path can restore the preferred EDID mode. Driver rejection must not leave a dead session.

### VI. Custom modes are ephemeral
KWin custom modes do not survive reboot (and often not sleep). `--apply-saved` / autostart is core.

### VII. Sharp by default
v1 targets scale **2.00** only.

### VIII. Discover before specifying
Unknown KWin/libkscreen behavior is researched and written down **before** `/speckit.specify`. Agents must not invent `setCustomModes` semantics.

### IX. Small surface and GPL-3.0-or-later
Tray + compact window + app CLI. SPDX on every file. No telemetry.

---

## Apply protocol

This is the v2 contract. Discovery may refine it; it must not skip a phase without evidence.

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

Do not call `setCurrentModeId` with a guessed id from before phase A. Wayland mode ids are backend-generated and do not match KWin’s internal ids.

`ConfigOperation::exec()` exists (nested event loop) and is what the Display KCM uses. GUI code should prefer the async `finished` signal so the tray does not freeze. R1 CLI may use `exec()`.

---

## Architecture (v2)

```text
app CLI / later tray
        │
ProfileEngine          native + ui_scale → looks + mode + hz
        │
DisplayBackend         interface
        │
LibKScreenBackend      default          DoctorBackend (optional, tests/fallback)
        │
KF6Screen ── DBus ── kscreen backend launcher ── KWin Wayland
```

Suggested tree:

```text
kscaling/
├── CMakeLists.txt          # find_package(Qt6 Core Widgets) find_package(KF6Screen)
├── LICENSE
├── KSCALING.md             # v1, historical
├── KSCALING-v2.md          # this file
├── src/
│   ├── core/               # ResolutionMath, Profile, Settings
│   ├── backend/
│   │   ├── DisplayBackend.h
│   │   └── LibKScreenBackend.cpp
│   └── ui/                 # R2+
└── tests/
    └── ResolutionMathTest.cpp
```

Link: `KF6::Screen`. Plasma 6.6+ is a hard runtime requirement. Check `CustomModes` and show a clear error on older stacks.

---

## Can Superpowers brainstorm discover things you never knew?

**Yes, but only in a narrow way.** Treat it as a structured interviewer plus a research intern, not as an oracle.

| It can discover | It cannot reliably discover |
|---|---|
| Design forks you have not named (one-phase vs two-phase apply, persist last mode vs recompute, clean modes or not) | Exact KWin rejection reasons on *your* GPU/DP link |
| Missing states (sleep/resume, dock undock, scale already 1.5) | Whether `setCustomModes` + `setCurrentModeId` in one SetConfig works on 6.6.x vs 6.7 |
| Test seams (Fake backend vs live compositor) | Bandwidth limits of your cable |
| API shapes once it **reads** `output.h`, `doctor.cpp`, kwin!8534 | Stable mode ids across reboot (they are not) |
| Questions worth asking you | “Best” Qt Quick vs Widgets taste without your constraint |

Unbounded brainstorm will happily invent a `KScreen::addMode()` that does not exist.

### Rule for this project

Superpowers brainstorm is **mandatory once**, and it is a **discovery session**, not a greenfield ideation session.

Constraints the agent must accept up front:

- Product and formula are already decided (`KSCALING-v2.md`)
- Backend is libkscreen, not doctor
- v1 scale is 2.00
- No virtual displays / EDID / X11

The agent **must read or cite** before proposing apply code:

1. `libkscreen/src/output.h` — `ModeInfo`, `customModes`, `Capability::CustomModes`
2. `libkscreen/src/doctor/doctor.cpp` — how doctor mutates the same objects
3. `libkscreen` MR !266 and kwin!8534 notes
4. A real `kscreen-doctor -j` (or `-o`) dump from **your** DP-3 session

Output of discovery is a short `docs/discovery-r1.md` (or Superpowers spec) that lists:

- Confirmed API calls
- Still-unknown behaviors
- Experiments to run on your machine (one command each)
- Decisions (use v2 defaults if you do not want to answer)

Only then run Spec Kit specify.

---

## Open questions for the discovery session

Ask these one at a time. Defaults apply if you skip.

1. After `setCustomModes` + `SetConfig`, does the same `ConfigPtr` already contain the new `Mode` with an id, or is a fresh `GetConfigOperation` required? **Default:** always reload (two-phase).
2. Can phase A and phase B be merged on current Plasma without KWin ignoring `currentModeId`? **Default:** do not merge until an experiment on DP-3 proves it.
3. Preferred identity field: `Output::name()`, `hash()`, or EDID serial? **Default:** `name()`, persist a fallback hash later if docking renames.
4. Revert: preferred mode only, or also restore pre-KScaling scale? **Default:** mode only; leave scale at 2.00.
5. Fake backend: does it implement `CustomModes` in the Plasma version you build against? **Default:** unit-test math without KScreen; integration-test backend behind an interface with a hand-written mock if Fake is incomplete.
6. Sleep/resume restore in v1? **Default:** no. Login only.
7. C++20 + CMake + Qt 6.6 + KF6Screen — any distro constraint (Kubuntu vs Fedora vs Arch)? **Ask once; record the answer.**

---

## New steps flow

Do **not** start with `/speckit.specify` or Superpowers `writing-plans`.

```text
0. Repo
1. Discovery (Superpowers brainstorm + research)     ← new
2. Freeze answers into docs/discovery-r1.md
3. Spec Kit constitution from v2
4. Spec Kit specify R1 only
5. Spec Kit clarify (should be short; discovery already did the forks)
6. Spec Kit plan + tasks + analyze
7. Superpowers writing-plans + TDD execute R1
8. Live verify on DP-3
9. Next slice (R2 tray) — same loop, skip a full product brainstorm
```

Pick **one** implementer for R1: Superpowers execute (recommended — math + async backend). Do not also run `/speckit.implement` on the same slice.

### Step 0 — repo

```bash
mkdir kscaling && cd kscaling
git init
# copy KSCALING.md, KSCALING-v2.md, LICENSE (GPL-3.0-or-later)
```

Optional: `specify init . --here`

Capture a fixture while the session still works:

```bash
kscreen-doctor -j > fixtures/dp3-before.json
kscreen-doctor -o > fixtures/dp3-before.txt
```

### Step 1 — discovery brainstorm (paste this)

```text
Use the Superpowers brainstorming skill as a DISCOVERY session, not a
greenfield product session.

Read KSCALING-v2.md. Accept as decided: app name KScaling, GPL-3.0-or-later,
200% scale, the four presets and formula, libkscreen as the only default
backend, no doctor runtime, no tray in R1, no virtual screens.

Your job is to find things I may not know about applying custom modes
through libkscreen on Plasma Wayland.

Hard rules:
- Do not write application code.
- Do not invent APIs. If you need a symbol, read output.h / doctor.cpp
  / GetConfigOperation / SetConfigOperation, or tell me to paste
  kscreen-doctor -j from this machine.
- Ask one question at a time from KSCALING-v2.md “Open questions”.
- Propose 2–3 apply-sequence options with trade-offs, then recommend
  the two-phase protocol unless evidence says one-phase works.
- End by writing docs/discovery-r1.md: confirmed API, unknowns,
  experiments, decisions.
```

If the agent cannot fetch KDE sources, paste `kscreen-doctor -j` and the apply protocol from this file and tell it to work only from that.

### Step 3 — constitution

```text
/speckit.constitution
Read KSCALING-v2.md section “Project constitution (v2)”.
KScaling is a GPL-3.0-or-later Plasma 6.6+ Wayland app.
Display API is libkscreen. kscreen-doctor is debug-only.
Two-phase apply. Connector name identity. Scale 2.00.
Discovery before specifying. Revert required.
```

### Step 4 — specify R1

```text
/speckit.specify
Parent roadmap: KSCALING-v2.md → R1.
Also read docs/discovery-r1.md.

R1 only: ResolutionMath + LibKScreenBackend + CLI
(--list, --apply <preset>, --revert, --apply-saved).
Two-phase apply protocol. Fixture A golden modes.
No tray, no slider, no doctor as runtime.
```

### Step 7 — implement R1

```text
Use writing-plans on the R1 spec + KSCALING-v2.md apply protocol.
TDD ResolutionMath first (Fixture A/B).
Then a mock DisplayBackend.
Then LibKScreenBackend against Get/SetConfigOperation.
CLI last. No Widgets.
```

Then `executing-plans` or subagent-driven-development.

### Step 8 — live gate (you, not the agent)

On DP-3:

```text
kscaling --list
kscaling --apply perfect
# expect 4096×2304, scale 2, looks like 2048×1152
kscaling --revert
kscaling --apply-saved
```

If phase B is rejected, paste `errorString()` and `kscreen-doctor -o` back into discovery — do not let the agent “try 120 Hz” in a loop.

---

## Roadmap (same slices, R1 backend changed)

| Id | Name | v2 intent |
|---|---|---|
| **R1** | Engine + libkscreen + CLI | Math, two-phase apply, profiles, `--apply-saved` |
| **R2** | Tray + compact window | Presets, revert, apply-on-login checkbox |
| **R3** | Autostart packaging | XDG `.desktop` → `kscaling --apply-saved` |
| **R4** | Multi-output profiles | Per connector; ignore disconnected |
| **R5** | Looks-like slider | `ui_scale` 1.00–2.00 |

R1 success: `kscaling --apply perfect` on the 2560×1440 fixture uses libkscreen, not a shell-out, and matches Fixture A.

---

## Agent rules (print on the repo README later)

- Implement from **KSCALING-v2.md**, not v1.
- If a task says `QProcess` + `kscreen-doctor`, reject it.
- If a task guesses a mode id string without a reload, reject it.
- If brainstorm tries to reopen the product name, formula, or BetterDisplay extras, steer back to discovery questions.

---

## License

Copyright 2026 KScaling contributors. GPL-3.0-or-later.
