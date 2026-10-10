/* core/frame.h — the frame loop.
 *
 * Owns: one frame of emulated time, derived from the disassembly's
 * NonMaskableInterrupt routine (smb.asm: NonMaskableInterrupt) and the
 * reset entry (smb.asm: Start). The skeleton polls the joypad into the
 * saved-bits RAM locations, advances the frame counter and toggles vblank;
 * translated game routines slot into the marked gap as they are ported.
 *
 * Collaborators: hw/joypad, hw/ppu, core/state. Deterministic — no
 * wall-clock reads.
 */
#ifndef SMBC_CORE_FRAME_H
#define SMBC_CORE_FRAME_H

#include "../hw/joypad.h"
#include "../hw/ppu.h"
#include "state.h"

typedef struct {
  CoreState state;
  Joypad joypad1;
  Joypad joypad2;
  Ppu ppu;
} Smbc;

void smbc_init(Smbc *smbc);

/* Run one frame: latch and read both joypads into SavedJoypad1Bits /
 * SavedJoypad2Bits (smb.asm: the NMI's joypad poll), then advance
 * FrameCounter and vblank. */
void smbc_frame(Smbc *smbc);

#endif /* SMBC_CORE_FRAME_H */
