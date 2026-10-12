# SMB.C

A native C port of the original NES *Super Mario Bros.*, translated routine by routine from its disassembly. The goal is to run the game as a normal native program on supported host systems—not to emulate a NES console or require a ROM at runtime.

The port aims to preserve the original game's graphics, audio, timing, physics, collision behaviour, glitches, and moment-to-moment gameplay. The original assembly disassembly, `smb.asm`, is the reference source; translated routines cite their source labels. Hardware-specific behaviour needed by the game is represented in C so the original game logic can run natively.

This is a work in progress. Fidelity takes priority over redesign: the default experience should look and play like the original NES release.

## Project goals

- Translate the original game logic from the disassembly into C.
- Run natively on the host platform rather than emulate a complete NES system.
- Reproduce original NES rendering, sound, frame timing, controller behaviour, physics, and known edge cases as closely as possible.
- Keep translated routines traceable to their original assembly labels.
- Provide repeatable tests and reference comparisons to catch behavioural regressions.

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

A playable SDL2 frontend is in (`src/host/`): the window presents the
PPU renderer's 256×240 frame at integer scale with nearest-neighbour
sampling, keyboard and SDL_GameController input map onto the NES
joypad, and the loop paces the deterministic core at the NTSC frame
rate with fixed steps. The audio device is opened with the pull
contract the APU synthesis (#35) will feed; it runs silent until
then. `--headless N` runs the core without any device.

The GameMode tree is in progress. `GameEngine` runs its full object pass
each frame: fireballs and bubbles, the enemy and floatey-number loop,
player graphics (pose selection, grow/shrink, swim kick, offscreen
parking), block objects, misc objects (hammers, jumping coins, floatey
numbers), cannons, whirlpools, the flagpole, the game timer, colour
rotation and the palette cycling, ending in the SaveAB tail and
`UpdScrollVar`. The `GameRoutines` tree is in: the area entrance setup,
the pipe and vine entries, the flagpole slide and end-of-level walk,
the lose-life, death, injury-blink, size-change and fire-flower
sequences, `PlayerCtrlRoutine`'s controller split, bounding box and
hole handling, and `ScrollHandler` with its screen-edge clamps.
The player physics core is in (`src/core/physics.c`): the movement
subroutines (ground, jumping, falling, climbing), `PlayerPhysicsSub`
with its speed brackets and friction, the head-bump block routines
(brick shatter, question blocks, hidden blocks, vines, coins),
`PlayerBGCollision`'s full head/feet/side tree and the pipe-entry
handling, and the title screen's task tree runs end to end:
`InitializeGame`, `ScreenRoutines`, `PrimaryGameSetup` and the menu
routine — the menu's demo tail now runs `GameCoreRoutine` for the
frame, and the demo-over and lose-life paths reset the title screen
the way the original does. The victory mode tree runs too:
`BridgeCollapse` (the axe, chain and bridge metatiles blanking out as
bowser drops), `SetupVictoryMode`, `PlayerVictoryWalk` (the fixed one
and a half pixel scroll toward the castle gate), `PrintVictoryMessages`,
`PlayerEndWorld`, the wrapper's `EnemiesAndLoopsCore` call and the
`AutoPlayer` tail — the whole tree runs end to end, and there are no
port TODOs left anywhere in `src/`. What remains open is hardware
fidelity (APU synthesis, issues #31–#39) and the modding roadmap
(#11 onward).

The source of truth is `smb.asm` — doppelganger's comprehensive disassembly
(x816 syntax), kept unmodified in the repository root. Every translated
routine cites the assembly label it came from.

---

## Building

```bash
make
```

Requires a C23 compiler (`cc`, clang or gcc) and SDL2 (the frontend;
`brew install sdl2` on macOS). The build is warnings-as-errors.

Run `./build/smbc` to play — the window opens straight into the game.
Arrows move, X = A, Z = B, Return = Start, Right Shift = Select,
Escape quits; a connected SDL_GameController mirrors the same buttons.
Flags: `--scale N` (integer window scale, 1-8, default 3),
`--headless N` (run N frames with no window, for CI and debugging).

`make test` builds and runs the suite with no SDL dependency — the
core stays headless; all OS I/O lives in `src/host/`.

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
  hw/     NES hardware model — PPU, APU, joypad registers, renderer
  core/   RAM map, game state, frame loop
  data/   tables translated verbatim from the disassembly
  host/   SDL2 frontend — video, input, audio seam, entry point
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
