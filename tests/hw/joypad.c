/* tests/hw/joypad.c — exercises src/hw/joypad.c: strobe latch, serial bit
 * order, trailing 1s, and live-A reads while strobed. */
#include <assert.h>
#include <stdio.h>

#include "../../src/hw/joypad.h"

int main(void) {
  Joypad pad;
  joypad_init(&pad);
  joypad_set_buttons(&pad, SMBC_BUTTON_A | SMBC_BUTTON_START | SMBC_BUTTON_RIGHT);

  /* latch on the strobe falling edge, then read A, B, Select, Start, ... */
  joypad_write(&pad, 0x01u);
  joypad_write(&pad, 0x00u);
  assert(joypad_read(&pad) == 1u); /* A */
  assert(joypad_read(&pad) == 0u); /* B */
  assert(joypad_read(&pad) == 0u); /* Select */
  assert(joypad_read(&pad) == 1u); /* Start */
  assert(joypad_read(&pad) == 0u); /* Up */
  assert(joypad_read(&pad) == 0u); /* Down */
  assert(joypad_read(&pad) == 0u); /* Left */
  assert(joypad_read(&pad) == 1u); /* Right */
  for (int i = 0; i < 8; i++) {
    assert(joypad_read(&pad) == 1u); /* past the end: 1s */
  }

  /* while strobed, reads report the live A button */
  joypad_set_buttons(&pad, 0);
  joypad_write(&pad, 0x01u);
  assert(joypad_read(&pad) == 0u);
  joypad_set_buttons(&pad, SMBC_BUTTON_A);
  assert(joypad_read(&pad) == 1u);

  /* releasing buttons then re-strobing latches the new state */
  joypad_write(&pad, 0x00u);
  joypad_write(&pad, 0x01u);
  joypad_write(&pad, 0x00u);
  assert(joypad_read(&pad) == 1u);

  printf("tests/hw/joypad: ok\n");
  return 0;
}
