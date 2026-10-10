/* tests/core/frame.c — exercises src/core/frame.c end to end: boot has run,
 * and each frame stores joypad bits in the game's layout and advances the
 * frame counter. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/state.h"
#include "../../src/hw/joypad.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* boot ran: warm boot validation and the PRNG seed are set */
  assert(smbc.state.ram[SMBC_WARM_BOOT_VALIDATION] == 0xa5u);
  assert(smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG] == 0xa5u);

  joypad_set_buttons(&smbc.joypad1, SMBC_BUTTON_B | SMBC_BUTTON_UP);
  joypad_set_buttons(&smbc.joypad2, SMBC_BUTTON_START);

  smbc_frame(&smbc);
  /* game layout: A=d7 ... Right=d0 (B=0x40, Up=0x08, Start=0x10) */
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x48u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD2_BITS] == 0x10u);
  assert(smbc.state.ram[SMBC_FRAME_COUNTER] == 1u);

  smbc_frame(&smbc);
  assert(smbc.state.ram[SMBC_FRAME_COUNTER] == 2u);

  printf("tests/core/frame: ok\n");
  return 0;
}
