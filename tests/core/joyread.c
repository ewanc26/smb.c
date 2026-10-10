/* tests/core/joyread.c — exercises src/core/joyread.c: the game's saved-bit
 * layout and the select/start double-read masking. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/joyread.h"
#include "../../src/core/state.h"
#include "../../src/hw/joypad.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* A and Start on pad 1: A lands in d7, Start in d4 */
  joypad_set_buttons(&smbc.joypad1, SMBC_BUTTON_A | SMBC_BUTTON_START);
  read_joypads(&smbc);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == (SMBC_PAD_A | SMBC_PAD_START));

  /* first read saves the full byte into JoypadBitMask; a second identical
   * read has select/start stripped (both bits seen twice running) */
  assert(smbc.state.ram[SMBC_JOYPAD_BIT_MASK] == (SMBC_PAD_A | SMBC_PAD_START));
  read_joypads(&smbc);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == SMBC_PAD_A);

  /* a read with neither select nor start refreshes the mask instead */
  joypad_set_buttons(&smbc.joypad1, SMBC_BUTTON_B);
  read_joypads(&smbc);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == SMBC_PAD_B);
  assert(smbc.state.ram[SMBC_JOYPAD_BIT_MASK] == SMBC_PAD_B);

  printf("tests/core/joyread: ok\n");
  return 0;
}
