/* hw/joypad.c — see hw/joypad.h for the contract. */
#include "joypad.h"

void joypad_init(Joypad *pad) {
  *pad = (Joypad){0};
}

void joypad_set_buttons(Joypad *pad, uint8_t bitmap) {
  pad->buttons = bitmap;
}

void joypad_write(Joypad *pad, uint8_t value) {
  bool strobe = (value & 0x01u) != 0;
  if (pad->strobe && !strobe) {
    /* falling edge: latch the live buttons into the shift register */
    pad->shift = pad->buttons;
  }
  pad->strobe = strobe;
}

uint8_t joypad_read(Joypad *pad) {
  if (pad->strobe) {
    return pad->buttons & 0x01u;
  }
  uint8_t bit = pad->shift & 0x01u;
  pad->shift = (uint8_t)((pad->shift >> 1) | 0x80u);
  return bit;
}
