# AGENTS.md

## SMBC

SMBC is a C port of doppelganger's Super Mario Bros. disassembly (`smb.asm`,
x816 syntax), sourced from <https://gist.github.com/1wErt3r/4048722>.

The goal is a C port of the **entire game** — every routine, every table,
the full RAM map — that **runs on any system**. The port is a faithful
translation, not a reimplementation: the disassembly is the specification,
and the C code exists to make it readable, testable and hostable off the
NES. Portability is a hard requirement, not a nice-to-have: the game code
must never grow a dependency on the machine it happens to be running on.

## Invariants

- **`smb.asm` is the source of truth.** It stays unmodified in the repository
  root. If the C and the assembly disagree, the C is wrong.
- **Every translated routine cites its origin.** A routine ported from the
  disassembly carries a comment naming the assembly label it came from
  (`/* smb.asm: LoadAreaPointer */`). Routines with no citation are host-side
  scaffolding and must live in the `hw` layer or `main.c`, never in `core`.
- **Data tables are translated verbatim.** Metatiles, level data, enemy data
  and music data under `src/data/` are byte-for-byte the disassembly's
  tables, with the source label cited. No hand-tuning values.
- **The RAM map is shared, flat memory.** The original uses fixed addresses
  across zero page and `$0700`–`$07FF`. The port keeps a single 2KB RAM image
  in `core/state` with named offsets, rather than inventing structured state
  the original does not have. Structure emerges only where the disassembly
  already imposes it.
- **No game logic in `hw`.** The hardware layer models NES registers and
  their side effects (latch toggles, strobe, serial reads) and nothing else.
- **Determinism.** Frame stepping is deterministic for a fixed input
  sequence. No wall-clock reads inside `core` or `hw`.
- **Portability.** Code under `src/hw/`, `src/core/` and `src/data/` is
  platform-independent C23. Concretely:
  - no OS or I/O calls (no stdio, no file access, no sockets) — platform
    contact happens only in `main.c` and future host frontends;
  - fixed-width integer types (`uint8_t`, `uint16_t`, ...) for all game
    state; no assumptions about `char` signedness, type sizes beyond what
    the fixed-width headers guarantee, or endianness (data tables are byte
    arrays and stay that way);
  - no floating point anywhere in game or hardware code;
  - no dynamic allocation — the NES's state is fixed-size, and so is the
    port's.
- **The frontend is replaceable.** `main.c` (currently a headless frame
  runner) and any future video/audio/input frontend sit outside the game
  code and consume it through the `hw` interfaces: a rendered frame comes
  from PPU state, sound from the APU register image, input goes in as
  joypad bitmaps. A new platform means a new frontend, never a change to
  `core`, `hw` or `data`.
- **British English** in prose, comments and documentation.

## Layout

- `src/hw/` — NES hardware model: PPU, APU and joypad register behaviour,
  plus the renderer that composes PPU state into a 256×240 frame of 2C02
  palette indices.
- `src/core/` — the RAM map, game state and the frame loop. Translated game
  code lives here.
- `src/data/` — tables translated verbatim from the disassembly.
- `tests/` — host unit tests, mirroring the `src/` scope layout.

## Build discipline

Plain Make, C23, warnings-as-errors (`-Wall -Wextra -Werror -Wpedantic`).
`make` builds, `make test` builds and runs the host suite. Both must stay
green on macOS and Linux, clang and gcc.
