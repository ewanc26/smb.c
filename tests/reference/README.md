# Reference comparison baselines (#36)

Each scenario is a replay recording (`<name>.smbcr`, see #37) plus a
baseline frame (`<name>.ppm`, P6, 256×240) captured at a fixed frame
number. `tests/reference/compare.c` — run by `make test` — replays
the recording from a fresh reset with the per-frame state hash
checked, renders the checkpoint frame and compares it against the
baseline pixel by pixel.

## Baseline provenance

These baselines are **self-captured from this port**, not from NES
hardware or a reference emulator. They pin current behaviour so
fidelity regressions are measurable; they do not prove correctness
against the original game. When emulator- or hardware-verified
captures exist, replace the PPMs in place and record the source here.

| Scenario | Frames | Checkpoint | Script |
|---|---|---|---|
| `title` | 90 | 89 | idle — title screen |
| `demo` | 700 | 699 | idle — attract demo running |
| `start` | 450 | 449 | Start at 200 — World 1-1 start |
| `walk` | 950 | 949 | Start at 200, hold Right from 700 |
| `jump` | 950 | 949 | Start at 200, Right from 700 with A taps |

## Updating baselines

```sh
./build/tests/reference/compare --update
```

This regenerates recordings and baselines from the compiled-in input
scripts. It is intentionally loud and manual: never re-baseline to
hide a regression. Review the diff outputs
(`build/reference/<name>-{actual,diff}.ppm`) and the git diff of the
baselines before committing, and explain baseline changes in the
commit message.
