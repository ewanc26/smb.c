/* core/frame.c — see core/frame.h for the contract. */
#include "frame.h"

/* smb.asm: NonMaskableInterrupt begins by strobing the joypads and reading
 * eight bits from each into SavedJoypad1Bits / SavedJoypad2Bits. */
static uint8_t poll_joypad(Joypad *pad) {
  uint8_t bits = 0;
  joypad_write(pad, 0x01u);
  joypad_write(pad, 0x00u);
  for (int i = 0; i < 8; i++) {
    bits |= (uint8_t)(joypad_read(pad) << i);
  }
  return bits;
}

void smbc_init(Smbc *smbc) {
  state_init(&smbc->state);
  joypad_init(&smbc->joypad1);
  joypad_init(&smbc->joypad2);
  ppu_init(&smbc->ppu);
}

void smbc_frame(Smbc *smbc) {
  ppu_set_vblank(&smbc->ppu, true);

  smbc->state.ram[SMBC_SAVED_JOYPAD1_BITS] = poll_joypad(&smbc->joypad1);
  smbc->state.ram[SMBC_SAVED_JOYPAD2_BITS] = poll_joypad(&smbc->joypad2);

  /* translated game routines slot in here, in disassembly order */

  smbc->state.ram[SMBC_FRAME_COUNTER]++; /* smb.asm: FrameCounter */
  ppu_set_vblank(&smbc->ppu, false);
}
