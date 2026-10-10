/* hw/joypad.h — NES controller serial interface.
 *
 * Owns: the strobe/latch/shift behaviour of a standard controller behind
 * $4016/$4017. The host feeds a button bitmap in; translated game code reads
 * bits out serially, exactly as the original does.
 *
 * Bit order out of the port: A, B, Select, Start, Up, Down, Left, Right,
 * then 1s forever.
 */
#ifndef SMBC_HW_JOYPAD_H
#define SMBC_HW_JOYPAD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SMBC_BUTTON_A      = 0x01,
  SMBC_BUTTON_B      = 0x02,
  SMBC_BUTTON_SELECT = 0x04,
  SMBC_BUTTON_START  = 0x08,
  SMBC_BUTTON_UP     = 0x10,
  SMBC_BUTTON_DOWN   = 0x20,
  SMBC_BUTTON_LEFT   = 0x40,
  SMBC_BUTTON_RIGHT  = 0x80,
} SmbcButton;

typedef struct {
  uint8_t buttons; /* host-supplied bitmap of currently held buttons */
  uint8_t shift;   /* latched copy being shifted out */
  bool strobe;     /* last written strobe level */
} Joypad;

void joypad_init(Joypad *pad);
void joypad_set_buttons(Joypad *pad, uint8_t bitmap);

/* $4016/$4017 write: only bit 0 (strobe) matters. */
void joypad_write(Joypad *pad, uint8_t value);

/* Serial read: bit 0 of the latched state, then a 1 shifts in. While the
 * strobe is high, reads report the live A button. */
uint8_t joypad_read(Joypad *pad);

#endif /* SMBC_HW_JOYPAD_H */
