# Feature Specification: R1 Engine and CLI

**Feature Branch**: `001-r1-engine-cli`

**Created**: 2026-09-05

**Status**: Draft

**Input**: User description: "Parent roadmap: KSCALING-v2.md → R1. Also read docs/discovery-r1.md. R1 only: ResolutionMath + LibKScreenBackend + CLI. Two-phase apply. Fixture A. No tray, no doctor runtime."

## Clarifications

### Session 2026-09-05

- Q: After a second preset apply without reverting, which scale should `--revert` restore? → A: Save original scale only on the first successful apply; later applies do not overwrite it.
- Q: Should `--revert` keep the last successful preset so `--apply-saved` can restore it later? → A: Revert restores preferred mode now but keeps the last successful preset for `--apply-saved`.
- Q: When more than one display is connected, how must the user name the target output for `--apply` and `--revert`? → A: `--output DP-3` when multiple outputs are connected; omit when there is only one.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Apply a looks-like preset (Priority: P1)

A Plasma Wayland user runs the KScaling command-line tool with a named preset
(`perfect`, `max-space`, `comfort`, or `large`). The tool reads the panel’s
native (preferred) size, computes a logical “looks like” size from the preset’s
UI-scale percentage, derives a 2× canvas, registers that custom size with the
compositor if needed, then switches the chosen output to that canvas at 200%
scale. The desktop looks sharper and sized as the preset promises.

**Why this priority**: This is the product. Without apply, nothing else matters.

**Independent Test**: On Fixture A (2560×1440 native), run `kscaling --apply
perfect` and confirm the session looks like 2048×1152 at 200% (canvas
4096×2304).

**Acceptance Scenarios**:

1. **Given** Fixture A native 2560×1440 and custom sizes are supported, **When**
   the user runs `kscaling --apply perfect`, **Then** the chosen output uses
   canvas 4096×2304 at scale 2.00 and looks like 2048×1152.
2. **Given** Fixture A, **When** the user applies `max-space`, `comfort`, or
   `large`, **Then** looks-like and canvas match the Fixture A golden table.
3. **Given** the target custom size is not yet known to the compositor, **When**
   apply runs, **Then** the tool registers the size, re-reads compositor sizes,
   then switches — it does not switch using a size id invented before
   registration.
4. **Given** a matching custom size already exists (same pixel size and rounded
   refresh), **When** apply runs, **Then** the tool reuses it and does not
   duplicate the entry.
5. **Given** the session is already on a 2× custom canvas, **When** the user
   applies a preset, **Then** native size is taken from the preferred panel
   mode, not from the current canvas.

---

### User Story 2 - List outputs and computed modes (Priority: P2)

The user runs `kscaling --list` to see connected outputs by connector name,
whether custom sizes are supported, native size, current size and scale, and
the four presets’ looks-like and canvas numbers.

**Why this priority**: Needed to choose the right connector and to verify math
before changing the session.

**Independent Test**: Run `kscaling --list` on a running Plasma Wayland session
and match connector names and native size against a known dump of that
session.

**Acceptance Scenarios**:

1. **Given** a connected output named `DP-3` with native 2560×1440, **When**
   the user lists, **Then** the tool prints connector `DP-3` (not a numeric
   backend id as identity) and the four Fixture A golden mode pairs.
2. **Given** an output that cannot accept custom sizes, **When** the user
   lists, **Then** that output is shown as unsupported, not as silently
   applicable.

---

### User Story 3 - Revert to the preferred panel mode (Priority: P3)

The user runs `kscaling --revert` to leave a custom canvas and return to the
output’s preferred (EDID) mode. Scale is restored to the original scale
captured before the first successful KScaling apply on that connector (later
preset hops do not overwrite that value). The last successful preset, canvas,
and refresh stay saved so `--apply-saved` can restore them later.

**Why this priority**: Recovery is part of the product; a bad custom mode must
not trap the session.

**Independent Test**: Apply `perfect`, then `--revert`, and confirm preferred
mode is current and scale matches the original pre-first-apply value.

**Acceptance Scenarios**:

1. **Given** a successful apply on `DP-3`, **When** the user reverts, **Then**
   that output returns to its preferred mode and to the original scale captured
   before the first successful apply on that connector.
2. **Given** the user applied `perfect` then `comfort` without reverting,
   **When** the user reverts, **Then** scale is the original pre-first-apply
   value, not 2.00.
3. **Given** no prior KScaling apply is recorded, **When** the user reverts,
   **Then** the tool still restores the preferred mode and reports that no
   saved prior scale existed (scale left unchanged).
4. **Given** a saved successful preset, **When** the user reverts, **Then**
   that preset remains available to `--apply-saved`.

---

### User Story 4 - Restore the last successful profile (Priority: P4)

After login (or any time custom modes have vanished), the user runs
`kscaling --apply-saved`. For each persisted connector that is currently
connected, the tool re-registers and re-activates the last successful
preset/canvas/refresh.

**Why this priority**: Custom modes do not survive reboot. Restore is core, not
polish. Autostart packaging itself is out of scope (R3).

**Independent Test**: Apply `perfect`, run `--revert`, then `--apply-saved`,
and confirm `perfect` is current again.

**Acceptance Scenarios**:

1. **Given** a saved successful profile for `DP-3` and that output is
   connected, **When** the user runs `--apply-saved`, **Then** that profile is
   applied with the same two-step register-then-switch behavior as `--apply`.
2. **Given** a saved profile for a connector that is disconnected, **When**
   `--apply-saved` runs, **Then** that connector is skipped without failing
   other connectors.
3. **Given** no saved profiles, **When** `--apply-saved` runs, **Then** the
   tool exits successfully and reports that nothing was applied.

---

### Edge Cases

- Apply on a compositor or output that cannot accept custom sizes: abort with
  a clear error; do not change mode or scale.
- Apply when the named connector is missing or disconnected: abort with a
  clear error.
- Register step fails: report the compositor error, stop, do not attempt
  switch.
- Switch step fails: restore preferred mode and the scale in effect immediately
  before this attempted apply, report the compositor error, do not retry other
  refresh rates.
- Successive preset applies without revert: original scale is captured only on
  the first successful apply; later applies MUST NOT overwrite it.
- Revert MUST NOT clear last successful preset, canvas, or refresh.
- Native size while already on a custom 2× canvas: always use preferred mode
  as native.
- Duplicate custom sizes: match by pixel size plus rounded refresh, not by
  exact floating refresh.
- Several connected outputs: `--apply` and `--revert` target the single
  connected output if there is only one; if there are several, the user MUST
  pass `--output <connector>` (e.g. `--output DP-3`). If several are
  connected and `--output` is omitted, abort with a clear error.
- Sleep/resume: not handled in R1.
- X11, virtual screens, EDID rewriting, tray, slider: out of scope.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Users MUST be able to apply one of four named presets
  (`perfect` ui_scale 1.25, `max-space` 1.00, `comfort` 1.60, `large` 1.778)
  from the CLI with no graphical UI.
- **FR-002**: The system MUST compute looks-like and canvas from native
  preferred size and preset `ui_scale` as:
  `looks_like_w = round_nearest_multiple_8(native_w / ui_scale)`;
  `looks_like_h = round_nearest_multiple_8(looks_like_w * native_h / native_w)`
  (aspect preserved); `canvas = looks_like × 2`; `scale = 2.00`. Native MUST
  be the output’s preferred size. This is the definition of `snap8`.
- **FR-003**: For Fixture A (native 2560×1440) the system MUST produce exactly:

  | preset | looks like | canvas |
  |---|---|---|
  | perfect | 2048×1152 | 4096×2304 |
  | max-space | 2560×1440 | 5120×2880 |
  | comfort | 1600×900 | 3200×1800 |
  | large | 1440×810 | 2880×1620 |

- **FR-004**: Applied scale MUST be 2.00. v1 MUST NOT use fractional scale to
  change looks-like size.
- **FR-005**: Refresh MUST follow: if canvas width ≥ 2 × native width → 60 Hz,
  else cap at 120 Hz. Custom sizes MUST be full-blanking (not reduced
  blanking). The system MUST NOT loop other Hz values if the compositor
  rejects the mode.
- **FR-006**: Users MUST be able to list connected outputs, connector names,
  custom-size support, native/current size, current scale, and computed
  presets (`kscaling --list`).
- **FR-007**: Users MUST be able to revert the target output to its preferred
  mode (`kscaling --revert`), restoring the original scale captured before the
  first successful apply on that connector when that value exists. Later
  preset applies MUST NOT overwrite that original scale. Revert MUST NOT
  clear the last successful preset, canvas, or refresh.
- **FR-008**: Users MUST be able to re-apply the last successful profile per
  connected connector (`kscaling --apply-saved`) without a tray or autostart
  installer.
- **FR-009**: The system MUST persist last successful canvas size, refresh,
  preset id, and original pre-first-apply scale, keyed by connector name
  (e.g. `DP-3`). It MUST NOT persist numeric backend ids. Original scale MUST
  be written only when it is not already stored for that connector.
- **FR-010**: Apply MUST be two-step: register the custom size with the
  compositor if it is not already present; re-read compositor state; then
  switch to the compositor-assigned size whose pixels match and whose refresh
  is nearest, and set scale 2.00. The system MUST NOT switch using an id
  guessed before registration. One-step register-and-switch is forbidden.
- **FR-011**: If the switch step fails, the system MUST restore preferred
  mode and the scale that was in effect immediately before this attempted
  apply, and surface the compositor error text.
- **FR-012**: The system MUST refuse to apply when custom sizes are not
  supported, with a clear error (not a silent no-op).
- **FR-013**: Runtime display changes MUST go through Plasma’s own display
  configuration service. The system MUST NOT run an external display-doctor
  command to list, apply, or revert.
- **FR-014**: When matching an existing custom size, the system MUST compare
  pixel size plus rounded refresh, and MUST send the full intended custom-size
  list (replace semantics), not a singleton new size.
- **FR-015**: The CLI MUST be usable with no graphical window. Target
  platform is Plasma 6.6+ Wayland.
- **FR-016**: `--apply` and `--revert` MUST accept `--output <connector>`.
  The flag is required when more than one output is connected and MUST be
  omitted-ok when exactly one is connected. `--apply-saved` MUST NOT require
  `--output`; it applies each saved connected connector.

### Key Entities

- **Output**: A physical connector identified by name (`DP-3`). Attributes:
  native/preferred size, current size, scale, whether custom sizes are
  allowed.
- **Preset**: Named ui_scale (`perfect`, `max-space`, `comfort`, `large`) that
  yields looks-like and canvas sizes.
- **Saved profile**: Per-connector last successful preset, canvas, refresh,
  and original scale from before the first successful apply.
- **Custom size**: Ephemeral compositor mode; gone after reboot; must be
  re-registered on restore.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On Fixture A, `kscaling --apply perfect` makes the desktop look
  like 2048×1152 at 200% (canvas 4096×2304) on the first successful attempt.
- **SC-002**: All four Fixture A presets match the golden looks-like and
  canvas pairs in FR-003 with no manual pixel entry.
- **SC-003**: A user can list, apply one preset, revert, and restore saved
  state using only the four CLI commands, with no graphical window.
- **SC-004**: After a failed switch, the session remains on the preferred
  panel mode (not a blank or stuck custom canvas) in 100% of failed-switch
  trials.
- **SC-005**: After a simulated “modes wiped” state (revert or reboot),
  `--apply-saved` restores the last successful preset on each still-connected
  saved connector without the user re-entering the preset name.
- **SC-006**: Math for Fixture A (and a second 1920×1080 case: perfect →
  looks like 1536×864 → canvas 3072×1728) can be verified without touching a
  live session.

## Assumptions

- Canonical product source is `KSCALING-v2.md`; `KSCALING.md` is historical.
- Discovery decisions in `docs/discovery-r1.md` (2026-09-05) apply: always
  reload after register; never merge register and switch; identity is
  connector name only; revert restores preferred mode plus pre-apply scale;
  no sleep/resume in R1. Custom-size integration is tested against a
  hand-written display backend mock, not the compositor Fake backend.
- Constitution: default display backend is libkscreen; `kscreen-doctor` is
  debug and fixtures only.
- `--apply` / `--revert` default to the only connected output; multiple
  outputs require `--output <connector>`.
- `--apply-saved` runs immediately (no login delay). Autostart `.desktop`
  install is R3.
- `--apply-saved` uses last successfully applied canvas and refresh first,
  and recomputes from preset plus current native only if those values are
  missing.
- Tray, compact window, looks-like slider, multi-output profile UI, virtual
  screens, EDID, and X11 are out of scope.
- Author verification machine is Plasma / KWin 6.6.4; R1 follows 6.6 symbols,
  not later `add_cvt` APIs.
- License GPL-3.0-or-later; no telemetry.
