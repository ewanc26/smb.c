/* core/frame.h — the frame loop and the console state bundle.
 *
 * Owns: the Smbc struct (RAM, both joypads, PPU, APU) and one frame of
 * emulated time: the raster clock enters vblank, the translated NMI
 * handler runs (advancing the clock through its hardware waits), and
 * the frame runs out. Initialisation runs the disassembly's reset
 * sequence. The full timing contract lives in hw/ppu.h.
 *
 * Deterministic — no wall-clock reads anywhere below the frontend.
 */
#ifndef SMBC_CORE_FRAME_H
#define SMBC_CORE_FRAME_H

#include "../hw/apu.h"
#include "../hw/joypad.h"
#include "../hw/ppu.h"
#include "state.h"

typedef struct Smbc {
  CoreState state;
  Joypad joypad1;
  Joypad joypad2;
  Ppu ppu;
  Apu apu;
} Smbc;

/* Zero all state, then run the reset sequence (smb.asm: Start). */
void smbc_init(Smbc *smbc);

/* Run one frame: vblank in, NMI (smb.asm: NonMaskableInterrupt) with its
 * raster-clock waits, frame out. */
void smbc_frame(Smbc *smbc);

#endif /* SMBC_CORE_FRAME_H */
