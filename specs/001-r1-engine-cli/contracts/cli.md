# Contract: `kscaling` CLI

Binary: `kscaling`. No GUI in R1. Flags are mutually exclusive except
`--output` which may combine with `--apply` or `--revert`.

## Invocation

```text
kscaling --list
kscaling --apply <preset> [--output <connector>]
kscaling --revert [--output <connector>]
kscaling --apply-saved
kscaling --help
```

`<preset>` ∈ `perfect` | `max-space` | `comfort` | `large`.
`<connector>` is `Output::name()` (e.g. `DP-3`).

## `--output` rules

- 0 connected outputs: exit 3 (any command that needs an output).
- 1 connected output: `--output` optional; if present it MUST match.
- >1 connected: `--apply` and `--revert` require `--output`; omit → exit 4
  with an error that lists connector names.
- `--apply-saved` ignores `--output` (if passed, exit 1 usage error).
- `--list` lists all outputs; `--output` is unused (if passed, exit 1).

## `--list` stdout (human text, UTF-8)

One block per connected output, at least:

```text
DP-3  capable  native 2560x1440  current 4096x2304 @ 59.93  scale 2.00
  perfect    looks 2048x1152  mode 4096x2304 @ 60
  max-space  looks 2560x1440  mode 5120x2880 @ 60
  comfort    looks 1600x900   mode 3200x1800 @ 120
  large      looks 1440x810   mode 2880x1620 @ 120
```

Unsupported outputs print `incapable` instead of `capable` and MAY omit
preset rows. Identity column is the connector name, never a numeric id.

## `--apply` / `--revert` / `--apply-saved` stdout

Brief human confirmation on success (connector, preset or “reverted”,
canvas, scale). Errors on stderr.

## Exit codes

| Code | Meaning |
|------|---------|
| 0 | Success (including `--apply-saved` with nothing to do) |
| 1 | Usage, parse, or backend error (including missing CustomModes) |
| 2 | Compositor rejected register or switch (after revert-on-fail for switch) |
| 3 | No connected outputs |
| 4 | Multiple outputs and `--output` missing or unknown connector |

## Runtime bans

MUST NOT spawn `kscreen-doctor` or any other display-doctor process.
MUST NOT guess a mode id from before the post-register reload.
