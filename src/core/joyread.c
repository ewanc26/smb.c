/* core/joyread.c — see core/joyread.h for the contract. */
#include "joyread.h"

#include "../hw/joypad.h"
#include "frame.h"
#include "state.h"

/* smb.asm: ReadPortBits — eight serial reads, first bit (A) ending in d7.
 * The port value's d0 and d1 are OR'd together, as the original does for
 * old Famicom systems; our hw model only ever drives d0. */
static void read_port_bits(Smbc *smbc, int port) {
  Joypad *pad = port == 0 ? &smbc->joypad1 : &smbc->joypad2;
  uint8_t bits = 0;
  for (int i = 0; i < 8; i++) {
    uint8_t v = joypad_read(pad);
    bits = (uint8_t)((bits << 1) | ((v | (v >> 1)) & 0x01u));
  }
  CoreState *state = &smbc->state;
  state->ram[SMBC_SAVED_JOYPAD1_BITS + port] = bits;
  if ((bits & 0x30u & state->ram[SMBC_JOYPAD_BIT_MASK + port]) != 0) {
    /* select or start seen twice running: strip them this read */
    state->ram[SMBC_SAVED_JOYPAD1_BITS + port] = bits & 0xcfu;
  } else {
    state->ram[SMBC_JOYPAD_BIT_MASK + port] = bits;
  }
}

void read_joypads(Smbc *smbc) {
  /* smb.asm: ReadJoypads — one strobe pair clocks both ports */
  joypad_write(&smbc->joypad1, 0x01u);
  joypad_write(&smbc->joypad2, 0x01u);
  joypad_write(&smbc->joypad1, 0x00u);
  joypad_write(&smbc->joypad2, 0x00u);
  read_port_bits(smbc, 0);
  read_port_bits(smbc, 1);
}
