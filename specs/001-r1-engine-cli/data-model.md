# Data Model: R1 Engine and CLI

## Preset

Named ui_scale. Closed set in R1.

| Field | Type | Rules |
|-------|------|-------|
| id | string | `perfect` \| `max-space` \| `comfort` \| `large` |
| ui_scale | float | 1.25, 1.00, 1.60, 1.778 |

## ModePlan

Computed from native preferred size + preset. Not persisted unless apply
succeeds (then copied into SavedProfile).

| Field | Type | Rules |
|-------|------|-------|
| looks_like | size (w×h) | `snap8` as in research.md; aspect preserved |
| canvas | size (w×h) | `looks_like × 2` |
| scale | float | always 2.00 |
| hz | float | 60 if canvas_w ≥ 2×native_w, else min(120, …) request 120 |

Fixture A (native 2560×1440) golden:

| id | looks_like | canvas |
|----|------------|--------|
| perfect | 2048×1152 | 4096×2304 |
| max-space | 2560×1440 | 5120×2880 |
| comfort | 1600×900 | 3200×1800 |
| large | 1440×810 | 2880×1620 |

Fixture B (native 1920×1080): perfect → 1536×864 → 3072×1728.

## Output (runtime, not persisted as a row)

| Field | Type | Rules |
|-------|------|-------|
| name | string | Connector `Output::name()`; identity key |
| connected | bool | Disconnected → skip on `--apply-saved`; error on `--apply`/`--revert` |
| custom_modes_capable | bool | Apply aborts if false |
| native | size + hz | From `preferredMode()` |
| current | size + hz + scale | From current mode |
| custom_modes | list of {size, hz} | Match by size + `qRound(hz)` |

Numeric `id()` MUST NOT be persisted or used as identity.

## SavedProfile

Per-connector record in `profiles.json`.

| Field | Type | Rules |
|-------|------|-------|
| preset | string | Last successful preset id |
| mode | [w, h] | Last successful canvas |
| hz | number | Last successful requested/applied refresh |
| originalScale | number | Scale before first successful apply; write-once until missing |

State:

```text
empty --successful apply--> saved
saved --successful apply--> saved (update preset/mode/hz; keep originalScale)
saved --revert--> saved (display back to preferred; file unchanged)
saved --apply-saved--> saved (re-register + switch)
saved --disconnected--> skipped (file unchanged)
failed apply --> file unchanged
```

`--apply-saved` uses `mode`+`hz` first; recomputes from `preset` + current
native only if mode/hz missing.

## profiles.json

```json
{
  "version": 1,
  "outputs": {
    "DP-3": {
      "preset": "perfect",
      "mode": [4096, 2304],
      "hz": 60,
      "originalScale": 1.0
    }
  }
}
```

Validation: unknown version → error. Missing `outputs` → treat as empty.
Connector keys MUST look like connector names, not integers.

## ApplyAttempt (ephemeral)

| Field | Rules |
|-------|-------|
| target name | From `--output` or the sole connected output |
| prior scale | Scale immediately before this attempt (failed switch restores this) |
| originalScale | From file, or prior scale if first success |

Failed phase A: stop, no switch, no persist.
Failed phase B: preferred mode + prior scale; no persist.
