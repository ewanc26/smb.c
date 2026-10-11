/* core/colour.c — see core/colour.h for the contract. */
#include "colour.h"

#include "../data/palettes.h"

/* smb.asm: ColorRotation */
void color_rotation(CoreState *st) {
  /* bne ExitColorRot — only the frames whose low three bits are zero */
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x07u) != 0x00u) return;
  uint8_t x = st->ram[SMBC_VRAM_BUFFER1_OFFSET];
  /* cpx #$31 / bcs — leave once 49 bytes are already queued */
  if (x >= 0x31u) return;
  /* GetBlankPal — the blank palette-3 set, copied whole (Y starts at
   * zero because the frame counter's low bits are zero here) */
  for (uint8_t y = 0; y < 8u; y++) {
    st->ram[SMBC_VRAM_BUFFER1 + x + y] = SMBC_BLANK_PALETTE[y];
  }
  x = st->ram[SMBC_VRAM_BUFFER1_OFFSET];
  /* GetAreaPal — AreaType * 4 selects the four palette bytes, patched
   * into the third through sixth buffer slots */
  uint8_t at = (uint8_t)(st->ram[SMBC_AREA_TYPE] << 2);
  for (uint8_t i = 0; i < 4u; i++) {
    st->ram[SMBC_VRAM_BUFFER1 + 3u + x + i] = SMBC_PALETTE_3_DATA[at + i];
  }
  x = st->ram[SMBC_VRAM_BUFFER1_OFFSET];
  /* the cycling colour overwrites the second palette byte */
  uint8_t cycle = st->ram[SMBC_COLOR_ROTATE_OFFSET];
  st->ram[SMBC_VRAM_BUFFER1 + 4u + x] = SMBC_COLOR_ROTATE_PALETTE[cycle];
  st->ram[SMBC_VRAM_BUFFER1_OFFSET] = (uint8_t)(x + 0x07u);
  cycle = (uint8_t)(cycle + 1u);
  if (cycle >= 0x06u) cycle = 0x00u;
  st->ram[SMBC_COLOR_ROTATE_OFFSET] = cycle;
}
