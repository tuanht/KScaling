# Quickstart: R1 Engine and CLI

## Prerequisites

- Plasma 6.6+ Wayland, KF6Screen with `CustomModes` (author: 6.6.4)
- `kf6-libkscreen-devel` (or distro equivalent), Qt 6 Core, CMake, C++20
- Fixture dumps: `fixtures/dp3-before.json`, `fixtures/dp3-before.txt`

## Build and unit tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: Fixture A goldens (FR-003) and Fixture B perfect (1536×864 /
3072×1728) pass without a compositor. Mock apply tests pass two-phase
rules (no pre-reload mode id).

## Live gate (author, DP-3)

Session must already be usable. Do not loop Hz if apply fails; paste
`errorString()` and `kscreen-doctor -o` into discovery instead.

```bash
kscaling --list
kscaling --apply perfect
# expect looks like 2048×1152, canvas 4096×2304, scale 2.00

kscaling --revert
# preferred 2560×1440; original scale restored; profiles.json still has perfect

kscaling --apply-saved
# perfect again

kscaling --apply comfort --output DP-3
kscaling --revert --output DP-3
```

`--apply perfect` MUST NOT appear in `ps` as a `kscreen-doctor` child.

## Config

After a successful apply: `~/.config/kscaling/profiles.json` contains
`outputs.DP-3` with `preset`, `mode`, `hz`, `originalScale`.
