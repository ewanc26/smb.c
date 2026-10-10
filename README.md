# SMBC

A C port of the entire Super Mario Bros. (NES) game, built from the
disassembly, that runs on any system.

---

## Status

The reset and NMI spine is translated and tested: boot, the interrupt
handler, joypad reading, pause, timers, the pseudorandom register, sprite
housekeeping, the VRAM buffer writer and name table initialisation. The
CHR pattern tables are embedded (`src/data/chr.c`). Game logic beyond the
spine — the mode execution tree, the sound engine, levels and enemies —
is tracked in the open issues and translated routine by routine.

The source of truth is `smb.asm` — doppelganger's comprehensive disassembly
(x816 syntax), kept unmodified in the repository root. Every translated
routine cites the assembly label it came from.

---

## Building

```bash
make
```

Requires a C23 compiler (`cc`, clang or gcc). The build is
warnings-as-errors.

---

## Testing

```bash
make test
```

Host-side unit tests live in `tests/`, mirroring the `src/` scope layout.

---

## Layout

```text
src/
  hw/     NES hardware model — PPU, APU, joypad registers
  core/   RAM map, game state, frame loop
  data/   tables translated verbatim from the disassembly
  main.c  entry point
tests/    host unit tests, mirrors src/ scopes
```

---

## Notes

`smb.asm` is doppelganger's disassembly (`SMBDIS.ASM`, assembles with
x816), sourced from [this gist](https://gist.github.com/1wErt3r/4048722) and
provided as-is; it is not original work and is not covered by this project's
licence. The character ROM data and
an iNES header are required to assemble the original — and, of course, the
game itself remains Nintendo's.

---

License: AGPL-3.0-only
