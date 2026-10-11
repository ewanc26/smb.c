/* core/offscreen.c — see core/offscreen.h for the contract. */
#include "offscreen.h"

#include "../data/objects.h"

/* smb.asm: GetObjRelativePosition — the vertical coordinate verbatim,
 * the horizontal coordinate minus the screen's left edge */
void relative_enemy_position(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_REL_YPOS] = st->ram[SMBC_ENEMY_Y_POSITION + x];
  st->ram[SMBC_ENEMY_REL_XPOS] =
      (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + x] -
                st->ram[SMBC_SCREEN_LEFT_X_POS]);
}

/* smb.asm: RelativePlayerPosition — the player's coordinates relative
 * to the screen: vertical verbatim, horizontal minus the left edge */
void relative_player_position(CoreState *st) {
  st->ram[SMBC_PLAYER_REL_YPOS] = st->ram[SMBC_PLAYER_Y_POSITION];
  st->ram[SMBC_PLAYER_REL_XPOS] =
      (uint8_t)(st->ram[SMBC_PLAYER_X_POSITION] -
                st->ram[SMBC_SCREEN_LEFT_X_POS]);
}

/* smb.asm: DividePDiff — while the object is within the preset
 * distance of an edge, the table offset is the pixel difference
 * divided by eight; the left and bottom checks add the adder to reach
 * their half of the data table. Anything at or beyond the preset
 * keeps the fallback offset */
static uint8_t divide_p_diff(uint8_t diff, uint8_t preset, uint8_t adder,
                             uint8_t side, uint8_t fallback) {
  if (diff >= preset) return fallback;
  uint8_t ofs = (uint8_t)((diff >> 3) & 0x07u);
  if (side == 0x00u) ofs = (uint8_t)(ofs + adder);
  return ofs;
}

/* smb.asm: GetXOffscreenBits — the right edge is checked first, then
 * the left; a nonzero mask ends the loop; i is the spr-object buffer
 * index (player $00, enemies $01+) */
uint8_t get_x_offscreen_bits(CoreState *st, uint8_t i) {
  uint8_t bits = 0x00u;
  for (int y = 1; y >= 0; y--) {
    uint16_t edge = ((uint16_t)st->ram[SMBC_SCREEN_LEFT_PAGE_LOC + y] << 8) |
                    st->ram[SMBC_SCREEN_LEFT_X_POS + y];
    uint16_t obj = ((uint16_t)st->ram[SMBC_PLAYER_PAGE_LOC + i] << 8) |
                   st->ram[SMBC_PLAYER_X_POSITION + i];
    int16_t d = (int16_t)(edge - obj);
    int8_t high = (int8_t)(d >> 8);
    uint8_t ofs;
    if (high < 0) {
      ofs = SMBC_DEFAULT_X_ONSCREEN_OFS[y];
    } else if (high >= 1) {
      ofs = SMBC_DEFAULT_X_ONSCREEN_OFS[y + 1];
    } else {
      ofs = divide_p_diff((uint8_t)d, 0x38u, 0x08u, (uint8_t)y,
                          SMBC_DEFAULT_X_ONSCREEN_OFS[y + 1]);
    }
    bits = SMBC_X_OFFSCREEN_BITS_DATA[ofs];
    if (bits != 0x00u) break;
  }
  return bits;
}

/* smb.asm: GetYOffscreenBits — the top of the screen first (edge one
 * vertical unit up plus HighPosUnitData), then the bottom */
static uint8_t get_y_offscreen_bits(CoreState *st, uint8_t i) {
  uint8_t bits = 0x00u;
  for (int y = 1; y >= 0; y--) {
    uint16_t edge = (uint16_t)(0x0100u | SMBC_HIGH_POS_UNIT_DATA[y]);
    uint16_t obj = ((uint16_t)st->ram[SMBC_PLAYER_Y_HIGH_POS + i] << 8) |
                   st->ram[SMBC_PLAYER_Y_POSITION + i];
    int16_t d = (int16_t)(edge - obj);
    int8_t high = (int8_t)(d >> 8);
    uint8_t ofs;
    if (high < 0) {
      ofs = SMBC_DEFAULT_Y_ONSCREEN_OFS[y];
    } else if (high >= 1) {
      ofs = SMBC_DEFAULT_Y_ONSCREEN_OFS[y + 1];
    } else {
      ofs = divide_p_diff((uint8_t)d, 0x20u, 0x04u, (uint8_t)y,
                          SMBC_DEFAULT_Y_ONSCREEN_OFS[y + 1]);
    }
    bits = SMBC_Y_OFFSCREEN_BITS_DATA[ofs];
    if (bits != 0x00u) break;
  }
  return bits;
}

/* smb.asm: GetOffScreenBitsSet — the horizontal mask in the low
 * nybble, the vertical mask in the high nybble */
void get_enemy_offscreen_bits(CoreState *st, uint8_t x) {
  uint8_t xbits = get_x_offscreen_bits(st, (uint8_t)(x + 1u));
  uint8_t ybits = get_y_offscreen_bits(st, (uint8_t)(x + 1u));
  st->ram[SMBC_ENEMY_OFFSCREEN_BITS] =
      (uint8_t)((ybits << 4) | (xbits >> 4));
}

/* smb.asm: GetPlayerOffscreenBits — the same pair with the player's
 * buffer index, stored in the player's offscreen bits */
void get_player_offscreen_bits(CoreState *st) {
  uint8_t xbits = get_x_offscreen_bits(st, 0x00u);
  uint8_t ybits = get_y_offscreen_bits(st, 0x00u);
  st->ram[SMBC_PLAYER_OFFSCREEN_BITS] =
      (uint8_t)((ybits << 4) | (xbits >> 4));
}

/* smb.asm: RelativeBlockPosition — the block object's two chunks
 * (spr-object indices 9 + block and 11 + block, i.e. the block arrays
 * at +block and +block+2) written into the block pool's relative
 * coordinate pair */
void relative_block_position(CoreState *st, uint8_t block) {
  st->ram[SMBC_BLOCK_REL_YPOS] = st->ram[SMBC_BLOCK_Y_POSITION + block];
  st->ram[SMBC_BLOCK_REL_XPOS] =
      (uint8_t)(st->ram[SMBC_BLOCK_X_POSITION + block] -
                st->ram[SMBC_SCREEN_LEFT_X_POS]);
  st->ram[SMBC_BLOCK_REL_YPOS + 1u] =
      st->ram[SMBC_BLOCK_Y_POSITION + block + 2u];
  st->ram[SMBC_BLOCK_REL_XPOS + 1u] =
      (uint8_t)(st->ram[SMBC_BLOCK_X_POSITION + block + 2u] -
                st->ram[SMBC_SCREEN_LEFT_X_POS]);
}

/* smb.asm: GetBlockOffscreenBits — via SetOffscrBitsOffset the block
 * object's offset is 9 + block; the combined bits land in
 * Block_OffscreenBits */
void get_block_offscreen_bits(CoreState *st, uint8_t block) {
  uint8_t i = (uint8_t)(9u + block);
  uint8_t xbits = get_x_offscreen_bits(st, i);
  uint8_t ybits = get_y_offscreen_bits(st, i);
  st->ram[SMBC_BLOCK_OFFSCREEN_BITS] =
      (uint8_t)((ybits << 4) | (xbits >> 4));
}

/* smb.asm: PlayerEnemyDiff — the low byte is discarded by callers;
 * the sign of the page difference is what branches on */
int8_t player_enemy_diff(CoreState *st, uint8_t x) {
  int16_t d = (int16_t)((((uint16_t)st->ram[SMBC_ENEMY_PAGE_LOC + x] << 8) |
                         st->ram[SMBC_ENEMY_X_POSITION + x]) -
                        (((uint16_t)st->ram[SMBC_PLAYER_PAGE_LOC] << 8) |
                         st->ram[SMBC_PLAYER_X_POSITION]));
  return (int8_t)(d >> 8);
}
