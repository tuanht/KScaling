# KScaling R1 discovery — libkscreen custom modes on Plasma Wayland

**Date:** 2026-09-05  
**Machine:** openSUSE (bp161), Plasma / KWin / libkscreen **6.6.4**  
**Sources read:** `libkscreen` Plasma/6.6 `output.h`, `doctor.cpp`, `getconfigoperation.*`, `setconfigoperation.*`, `backends/kwayland/waylandoutputdevice.cpp`, `backends/kwayland/waylandoutputdevicemode.cpp`, `backends/kwayland/waylandconfig.cpp`, `backends/fake/{fake,parser}.cpp`; `kwin` Plasma/6.6 `src/wayland/outputmanagement_v2.cpp`; `kde-output-management-v2.xml`; `fixtures/dp3-before.json` + `fixtures/dp3-before.txt`.  
**Not read as 6.6:** GitHub `master` differs (protocol v22, `add_cvt`, `ModeInfo::cvt`). R1 must follow **6.6.4** symbols.

Product decisions from `KSCALING-v2.md` are unchanged: name KScaling, GPL-3.0-or-later, scale 2.00, four presets + formula, libkscreen only, no doctor runtime, no tray in R1, no virtual screens.

---

## Confirmed API (do not invent past this)

### Types (KF6Screen 6.6)

```cpp
namespace KScreen {
class ModeInfo {
    QSize size;
    float refreshRate = 60.0;
    Flags flags;  // Flag::Custom = 0x1, Flag::ReducedBlanking = 0x2
    bool operator==(const ModeInfo &) const = default;
};

class Output {
    enum class Capability { /* ... */ CustomModes = 1 << 13, /* ... */ };
    QString name() const;                 // "DP-3"
    QString uuid() const;                 // in doctor -o, not in -j
    QString hash() const;                 // deprecated
    QString hashMd5() const;
    int id() const;                       // client-local, starts at 1 per process
    Capabilities capabilities() const;
    QList<ModeInfo> customModes() const;
    void setCustomModes(const QList<ModeInfo> &);
    QString currentModeId() const;
    void setCurrentModeId(const QString &);
    ModePtr currentMode() const;
    QString preferredModeId() const;      // highest res+Hz among preferredModes
    ModePtr preferredMode() const;
    void setScale(qreal);
    qreal scale() const;
};

class GetConfigOperation : public ConfigOperation { /* config() is a clone */ };
class SetConfigOperation : public ConfigOperation {
    explicit SetConfigOperation(const ConfigPtr &, QObject *parent = nullptr);
    // config() returns the SAME pointer you passed in, not a compositor refresh
};
class ConfigOperation {
    bool hasError() const;
    QString errorString() const;
    bool exec();                          // nested event loop; Display KCM uses this
    void finished(ConfigOperation *);
};
}
```

There is **no** `KScreen::addMode()`, no `Output::addCustomMode()`, no compositor-stable mode id.

### How doctor mutates the same objects

`addCustomMode` (doctor.cpp):

- `flags = ModeInfo::Flag::Custom`; `.full` leaves ReducedBlanking unset; `.reduced` ORs it in.
- `refreshRate = milliHz / 1000.0f` (CLI takes e.g. `60000`).
- **Appends** to `output->customModes()` then `setCustomModes(modes)`.
- Does **not** call `setCurrentModeId` in the same parse.
- One `SetConfigOperation(m_config)->exec()` after all CLI ops.

`mode`:

- Resolves by `Mode::id()` **or** `"WxH@" + qRound(refreshRate)`.
- Then `setCurrentModeId(mode->id())`.
- Looks at the **current** `modes()` list — a custom mode added in the same doctor invocation is not there yet.

### Wayland apply path (why one-phase cannot work)

`WaylandOutputDevice::setWlConfig` (Plasma/6.6):

1. If `currentMode()->id() !=` the live Wayland current id, calls  
   `wlConfig->mode(device, deviceModeFromId(id)->object())`.  
   That requires an existing `kde_output_device_mode_v2`. **Null deref** if the id is unknown.
2. If `customModes()` differs (exact `ModeInfo::operator==`, including float Hz), sends `set_custom_modes` with the **entire** list (`set_resolution` / `set_refresh_rate` milliHz / `set_reduced_blanking` / `add_mode`).

Protocol `kde_output_configuration_v2.mode` takes a **mode object**, not width/height. Custom modes are only advertised after KWin applies `set_custom_modes`. Protocol note: devices are updated **before** the `applied` event, so a **new** `GetConfigOperation` after `SetConfigOperation` finished should see them. The `SetConfigOperation`’s own `ConfigPtr` does not.

KWin 6.6 `kde_output_configuration_v2_mode` stores a pointer to an existing `OutputMode`. It does not match a not-yet-generated custom modeline.

`set_custom_modes` **replaces** the custom list (“delete ones no longer in the list”). Always send the full intended `customModes()`, never a singleton new mode.

Mode ids are assigned in the **client** (`WaylandOutputDeviceMode`: `static uint id = 1`). They are not KWin’s ids and are not stable across process restart.

### Fake backend

`KSC_Fake.so` is installed. `parser.cpp` sets only `Capability::Disable`, does not parse `customModes`. `Fake::setConfig` clones the pointer; it will not invent mode ids. Unusable as a CustomModes integration backend.

### Fixture A (this machine, already scaled)

`DP-3`, numeric id `1`, uuid `572f64e0-7328-4682-a0cf-af808d51171b`.

| Field | Value |
|---|---|
| Native (preferred `1`) | 2560×1440 @ 164.96 Hz (`!`) |
| Current | mode id `"40"`, 4096×2304 @ 59.93, scale **2** |
| Logical geometry | 2048×1152 (= looks-like `perfect`) |
| `preferredModes` | `"1"`, `"2"` (2560×1440 @ 165 and @ 144) |
| `followPreferredMode` | false |
| Custom list (`-o`) | 8 entries: four 4096×2304 @ ~60, two 5120×2880 @ ~120, two 3200×1800 @ ~120 |

`kscreen-doctor -j` does **not** serialize `customModes`, `uuid`, or `capabilities`. Use `-o` for custom modes.

When already on a custom 2× canvas, `currentMode()` / `size()` are **not** native. Native for the formula is `preferredMode()`.

---

## Apply-sequence options

### Option 1 — Two-phase (recommended)

```text
GetConfig
find name() == connector
abort if !capabilities().testFlag(CustomModes)
native = preferredMode()                         // not currentMode()
if no custom ModeInfo with same size and qRound(Hz):
    append ModeInfo{size, hz, Flag::Custom}      // no ReducedBlanking
    setCustomModes(full list)                    // replace-semantics
    SetConfig                                    // phase A
    if error → report, stop
GetConfig                                        // required; do not reuse ConfigPtr
pick Mode: size match, nearest refreshRate
setCurrentModeId(that id)
setScale(2.0)
SetConfig                                        // phase B
if error → setCurrentModeId(preferredModeId()); setScale(savedPrior); SetConfig; report
persist last successful mode+hz+preset + prior scale per name()
```

**Trade-off:** two round-trips; one extra modeset if the mode is new. Matches doctor’s split (`addCustomMode` then `mode`). Safe against null `deviceModeFromId`.

Skip phase A when a custom mode of that **size** already exists (nearest Hz). Do not require exact `60.0f` equality — KWin CVT advertises 59.93 / 59.92 / …

### Option 2 — One-phase merge

`setCustomModes` + `setCurrentModeId` + `setScale` in one `SetConfigOperation`.

**Rejected.** libkscreen 6.6 cannot address a mode that has no Wayland object yet. KWin 6.6 `mode()` also requires an existing `OutputMode`. Do not experiment by guessing an id (crash path).

### Option 3 — Two-phase, skip A when already registered

Same as option 1, but if `modes()` already has a matching size (custom or not), only phase B. The fixture already has `perfect` / `max-space` / `comfort` leftovers.

**Trade-off:** faster re-apply; must still GetConfig at start. Dedup by size + rounded Hz or the custom list grows (fixture already has four 4096×2304@~60).

**Recommendation:** Option 1, with option 3 as the “already present” branch inside it. Never option 2.

---

## Unknowns (not inventable from headers)

- Exact driver reject string on this GPU/DP link for a given WxH@Hz. `errorString()` / KWin `failure_reason` (protocol v12+) is the only signal. Do not loop Hz from the agent.
- Whether 4096×2304@120 or 5120×2880@60 is accepted. Fixture shows 4096@~60 and 5120@~120 — **opposite** of the v2 heuristic (`mode_w ≥ 2×native_w → 60 else cap 120`). Heuristic is unproven here.
- Whether custom modes survive sleep on this box (constitution: often not). R1 will not handle resume.
- Whether `GetConfig` immediately after `SetConfig::finished` always includes the new mode, or a `ConfigMonitor` tick is sometimes needed. Protocol says devices update before `applied`; still worth one live check.
- `features: 255` in `-j` is not in current `serializeConfig`; ignore.

---

## Experiments (author, one command each; do not loop)

Run on DP-3. Paste `errorString` / `-o` back if anything fails. Prefer a throwaway size so the session stays on `perfect`.

1. **Phase A does not create a selectable mode in the same doctor process**  
   `kscreen-doctor output.DP-3.addCustomMode.2880.1620.60000.full output.DP-3.mode.2880x1620@60`  
   Expect: mode not found (or matches a leftover), not a one-shot switch.

2. **Phase A then reload then switch (two doctor processes = two-phase)**  
   `kscreen-doctor output.DP-3.addCustomMode.2880.1620.60000.full && kscreen-doctor output.DP-3.mode.2880x1620@60 output.DP-3.scale.2`

3. **Exact-Hz mismatch / duplicates**  
   Repeat experiment 1’s add twice, then `kscreen-doctor -o`. Expect another 2880×1620@~59.9x line, not reuse.

4. **Revert**  
   `kscreen-doctor output.DP-3.mode.2560x1440@165`  
   Confirm preferred `!` mode comes back. Scale is a separate `.scale.` op.

5. **Native while already 2×**  
   Already captured: preferred remains 2560×1440 while current is 4096×2304. No extra command.

Do **not** have the agent iterate 120 Hz vs 60 Hz on failure.

---

## Decisions (discovery session 2026-09-05)

| # | Question | Decision |
|---|---|---|
| 1 | Reload after `setCustomModes`? | **Always** fresh `GetConfigOperation`. |
| 2 | Merge phase A+B? | **No.** |
| 3 | Identity | **`Output::name()` only.** No uuid/hash in R1. Never persist numeric id. |
| 4 | Revert | **Preferred mode + scale saved from before apply** (overrides v2 default “mode only / leave 2.00”). |
| 5 | Fake | **ResolutionMath unit tests + hand-written `DisplayBackend` mock.** No Fake CustomModes. |
| 6 | Sleep/resume | **No in R1.** Login only (`--apply-saved` / autostart = R3). |
| 7 | Distro | **Any Plasma 6.6+** with KF6Screen `CustomModes`. Author box is openSUSE 6.6.4. |

Additional bindings from evidence (not voted, treat as R1 law):

- Match existing custom modes by **size + `qRound(refreshRate)`**, not `ModeInfo::operator==`.
- `setCustomModes` sends the **full** list (protocol replace).
- Native resolution = **`preferredMode()`**, not `currentMode()`.
- Abort if `!CustomModes`.
- R1 CLI may `ConfigOperation::exec()`; GUI later uses `finished`.
- Doctor is debug/fixtures only.
- Need `kf6-libkscreen-devel` (or distro equivalent) to compile; not installed on this box today.

---

## Next

Spec Kit constitution from v2 + this file, then specify R1 only. Do not implement until that spec exists.
