/* core/sprites.c — see core/sprites.h for the contract. */
#include "sprites.h"

/* smb.asm: the shared SprInitLoop — write $F8 to every fourth byte (the Y
 * coordinate) from the entry offset until the index wraps. */
static void move_sprites_from(CoreState *state, uint8_t offset) {
  uint8_t y = offset;
  do {
    state->ram[SMBC_SPRITE_Y_POSITION + y] = 0xf8u;
    y = (uint8_t)(y + 4u);
  } while (y != 0);
}

void move_all_sprites_offscreen(CoreState *state) {
  /* smb.asm: MoveAllSpritesOffscreen (falls into the loop with Y=$00) */
  move_sprites_from(state, 0x00u);
}

void move_sprites_offscreen(CoreState *state) {
  /* smb.asm: MoveSpritesOffscreen (Y=$04) */
  move_sprites_from(state, 0x04u);
}

void sprite_shuffler(CoreState *state) {
  /* smb.asm: SpriteShuffler */
  const uint8_t preset = 0x28u;
  for (int x = 0x0e; x >= 0; x--) {
    uint8_t off = state->ram[SMBC_SPR_DATA_OFFSET + x];
    if (off < preset) {
      continue; /* smb.asm: NextSprOffset */
    }
    uint8_t amt = state->ram[SMBC_SPR_SHUFFLE_AMT +
                             state->ram[SMBC_SPR_SHUFFLE_AMT_OFFSET]];
    uint16_t sum = (uint16_t)(off + amt);
    uint8_t result = (uint8_t)sum;
    if (sum > 0xffu) {
      result = (uint8_t)(result + preset); /* smb.asm: bcc skip, else adc $00 */
    }
    state->ram[SMBC_SPR_DATA_OFFSET + x] = result;
  }
  uint8_t n = (uint8_t)(state->ram[SMBC_SPR_SHUFFLE_AMT_OFFSET] + 1u);
  state->ram[SMBC_SPR_SHUFFLE_AMT_OFFSET] = n == 3u ? 0u : n;

  /* smb.asm: SetMiscOffset — derive the three misc offsets from the block
   * offsets, each $08 apart; the adds do not propagate carry (clc each). */
  int x = 0x08;
  for (int y = 0x02; y >= 0; y--) {
    uint8_t v = state->ram[SMBC_SPR_DATA_OFFSET + 5 + y];
    state->ram[SMBC_MISC_SPR_DATA_OFFSET - 2 + x] = v;
    state->ram[SMBC_MISC_SPR_DATA_OFFSET - 1 + x] = (uint8_t)(v + 0x08u);
    state->ram[SMBC_MISC_SPR_DATA_OFFSET + x] =
        (uint8_t)((uint8_t)(v + 0x08u) + 0x08u);
    x -= 3;
  }
}
