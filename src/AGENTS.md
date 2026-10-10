# Source module layout

This file refines the repository-wide `AGENTS.md` rules for everything under
`src/`.

## Mandatory scoped/atomic paths

Implementation and private-module files MUST use the form:

```text
./<scope>/<atom>.{c,h}
```

The `./` is relative to the source root (`src/`). A scope is a cohesive
subsystem (`hw`, `core`, `data`); an atom is one focused responsibility
within it.

Examples:

```text
src/hw/ppu.c
src/hw/joypad.c
src/core/state.c
src/core/frame.c
src/data/metatiles.c
```

Do not create flat compound modules such as `hw_ppu.c` or `core_state.c`, and
do not use underscored file names at all. The directory is the scope, the
file is a single-word atom.

Scope-private declarations belong in an `internal.h` inside the scope
directory. Cross-scope interfaces use the atom's own header (`hw/ppu.h` is
the PPU's interface to `core`).

## Atomicity

Each atom owns one responsibility and should remain substantially below 500
lines. Split an atom before unrelated behaviour pushes it toward that size.
Do not create generic `util.*`, `misc.*` or `helpers.*` dumping grounds;
shared code needs a deliberately named scope and a concrete contract.

A file move or split must not silently change behaviour. Pure modularisation
preserves the translated logic, the data tables and the RAM map semantics.
Behavioural changes belong in separate commits with their own tests — and,
for translated code, a citation of the disassembly passage that justifies
them.

Every new module needs a short leading contract comment explaining what it
owns, its important collaborators, ownership expectations where relevant,
and failure semantics.

## Tests

Tests mirror the source layout: `tests/<scope>/<atom>.c` exercises
`src/<scope>/<atom>.c`. Each test file is a standalone binary with its own
`main`; the Makefile discovers and runs them all.
