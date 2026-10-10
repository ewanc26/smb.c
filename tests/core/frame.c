/* tests/core/frame.c — exercises src/core/frame.c: joypad bits land in the
 * saved-bits RAM locations and the frame counter advances. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/state.h"
#include "../../src/hw/joypad.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  joypad_set_buttons(&smbc.joypad1, SMBC_BUTTON_B | SMBC_BUTTON_UP);
  joypad_set_buttons(&smbc.joypad2, SMBC_BUTTON_START);

  smbc_frame(&smbc);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] ==
         (SMBC_BUTTON_B | SMBC_BUTTON_UP));
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD2_BITS] == SMBC_BUTTON_START);
  assert(smbc.state.ram[SMBC_FRAME_COUNTER] == 1u);

  smbc_frame(&smbc);
  assert(smbc.state.ram[SMBC_FRAME_COUNTER] == 2u);

  printf("tests/core/frame: ok\n");
  return 0;
}
