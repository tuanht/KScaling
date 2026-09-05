# Contract: DisplayBackend

Internal seam. Default implementation: libkscreen. Tests: hand-written mock.
Not a public library ABI.

## Types

```text
ConnectorName: string          # Output::name()
Size: { w: int, h: int }
Mode: { size: Size, hz: float, id: string }
OutputSnapshot:
  name, connected, customModesCapable
  native: Mode                 # preferred
  current: Mode
  scale: float
  customModes: [{ size, hz }]  # no ids required
```

`id` is compositor-local and valid only for the snapshot it came from.

## Operations

### list() → OutputSnapshot[] | Error

Fresh compositor read. No doctor.

### applyCustom(name, canvas, hz, scale=2.00) → Result

1. GetConfig. Find `name`. Error if missing/disconnected.
2. Error if `!customModesCapable`.
3. Native = preferred mode of that snapshot.
4. If no custom mode with same size and `qRound(hz)`: append Custom
   full-blanking `ModeInfo` to the **full** list, SetConfig (phase A).
   On error: return reject (exit 2 path); do not switch.
5. GetConfig again (new snapshot; do not reuse ids from step 1).
6. Pick mode: size match, nearest refreshRate. Error if none.
7. setCurrentModeId(that id); setScale(2.00); SetConfig (phase B).
8. On error: setCurrentModeId(preferredModeId); setScale(scaleBeforeAttempt);
   SetConfig; return reject with compositor error text.
9. Success: caller persists SavedProfile.

### revert(name, originalScale | none) → Result

GetConfig, setCurrentModeId(preferredModeId), if originalScale present
setScale(originalScale), SetConfig. Do not clear SavedProfile.

## Mock requirements

The mock MUST implement CustomModes, assign ids only after a simulated
phase A, and refuse `setCurrentModeId` with an id from a pre-phase-A
snapshot. It MUST NOT call KF6Screen.
