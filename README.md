# KScaling

KScaling R1 is a Plasma 6.6+ Wayland CLI. It applies HiDPI “looks like”
presets through libkscreen custom modes at scale 2.00.

There is no tray in R1. Apply goes through libkscreen, not `kscreen-doctor`.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

Build and run steps: [specs/001-r1-engine-cli/quickstart.md](specs/001-r1-engine-cli/quickstart.md).

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
