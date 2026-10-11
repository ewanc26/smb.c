# SMBC

A C port of the entire Super Mario Bros. (NES) game, built from the
disassembly, that runs on any system.

---

## Status

The reset and NMI spine is translated and tested: boot, the interrupt
handler, joypad reading, pause, timers, the pseudorandom register, sprite
housekeeping, the VRAM buffer writer and name table initialisation. The
game-setup tasks (`SecondaryGameSetup`, `GetAreaMusic`), the status-bar
writer and digit arithmetic, and the ROM-backed VRAM update sets (palettes
and messages) are in, with the CHR pattern tables embedded
(`src/data/chr.c`). The PPU model decodes the donor cartridge's name-table
mirroring and models palette-region `$2007` reads.

The GameMode tree is in progress. `GameEngine` runs its full object pass
each frame: fireballs and bubbles, the enemy and floatey-number loop,
player graphics (pose selection, grow/shrink, swim kick, offscreen
parking), block objects, misc objects (hammers, jumping coins, floatey
numbers), cannons, whirlpools, the flagpole, the game timer, colour
rotation and the palette cycling, ending in the SaveAB tail and
`UpdScrollVar`. The thirteen `GameRoutines` player routines and
`InitializeArea` are the remaining pieces of the tree. The sound engine,
enemy data driving and the title/victory trees are tracked in the open
issues and translated routine by routine.

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
