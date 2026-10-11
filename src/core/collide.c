/* src/core/collide.c — smb.asm's bounding box and collision layer. */
#include "collide.h"

#include "offscreen.h"

/* smb.asm: BoundBoxCtrlData — left/top/right/bottom offsets per
 * bound box control value */
static const uint8_t SMBC_BOUND_BOX_CTRL_DATA[48] = {
    0x02u, 0x08u, 0x0eu, 0x20u, 0x03u, 0x14u, 0x0du, 0x20u,
    0x02u, 0x14u, 0x0eu, 0x20u, 0x02u, 0x09u, 0x0eu, 0x15u,
    0x00u, 0x00u, 0x18u, 0x06u, 0x00u, 0x00u, 0x20u, 0x0du,
    0x00u, 0x00u, 0x30u, 0x0du, 0x00u, 0x00u, 0x08u, 0x08u,
    0x06u, 0x04u, 0x0au, 0x08u, 0x03u, 0x0eu, 0x0du, 0x14u,
    0x00u, 0x02u, 0x10u, 0x15u, 0x04u, 0x04u, 0x0cu, 0x1cu,
};

/* smb.asm: MoveBoundBoxOffscreen — all four coordinates offscreen */
static void move_bound_box_offscreen(CoreState *st, uint8_t x) {
  uint16_t base = SMBC_ENEMY_BBOX_COORD + (uint16_t)(x * 4u);
  st->ram[base] = 0xffu;
  st->ram[base + 1u] = 0xffu;
  st->ram[base + 2u] = 0xffu;
  st->ram[base + 3u] = 0xffu;
}

/* smb.asm: GetMaskedOffScrBits — the offscreen bits masked by which
 * side of the screen's left edge the object is on: beyond or
 * exactly at it keeps the caller's second mask, anywhere to the
 * right uses the first */
static void get_masked_off_scr_bits(CoreState *st, uint8_t x,
                                    uint8_t right_of_edge,
                                    uint8_t at_or_beyond) {
  uint8_t low = (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + x] -
                          st->ram[SMBC_SCREEN_LEFT_X_POS]);
  int16_t high = (int16_t)(st->ram[SMBC_ENEMY_PAGE_LOC + x] -
                           st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] -
                           (low > st->ram[SMBC_ENEMY_X_POSITION + x] ? 1 : 0));
  uint8_t mask = at_or_beyond;
  if (high >= 0 && (high | low) != 0x00) mask = right_of_edge;
  st->ram[SMBC_ENEMY_OFFSCR_MASKED + x] =
      st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & mask;
}

/* smb.asm: BoundingBoxCore — the four corners from the relative
 * coordinates plus the control data row. x arrives as the caller's
 * slot + 1 (the player's box leads the array) and y indexes the
 * relative coordinate arrays */
static void bounding_box_core(CoreState *st, uint8_t x, uint8_t y) {
  uint8_t rel_y = st->ram[SMBC_SPROBJ_REL_YPOS + y];
  uint8_t rel_x = st->ram[SMBC_SPROBJ_REL_XPOS + y];
  uint8_t ctrl = st->ram[SMBC_SPROBJ_BOUND_BOX_CTRL + x];
  const uint8_t *data = &SMBC_BOUND_BOX_CTRL_DATA[ctrl * 4u];
  uint16_t box = SMBC_BOUNDING_BOX_UL_CORNER + (uint16_t)(x * 4u);
  st->ram[box] = (uint8_t)(rel_x + data[0]);
  st->ram[box + 2u] = (uint8_t)(rel_x + data[2]);
  st->ram[box + 1u] = (uint8_t)(rel_y + data[1]);
  st->ram[box + 3u] = (uint8_t)(rel_y + data[3]);
}

/* smb.asm: CheckRightScreenBBox / CheckLeftScreenBBox — against the
 * screen's middle: on the right side a wrapped (sign-set) edge goes
 * to $ff, on the left a wrapped edge past $a0 goes to $00 */
static void check_right_screen_bbox(CoreState *st, uint8_t x, uint8_t y) {
  uint8_t mid_x = (uint8_t)(st->ram[SMBC_SCREEN_LEFT_X_POS] + 0x80u);
  uint8_t carry = mid_x < st->ram[SMBC_SCREEN_LEFT_X_POS] ? 1u : 0u;
  uint8_t mid_page =
      (uint8_t)(st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] + carry);
  uint16_t obj = ((uint16_t)st->ram[SMBC_SPROBJ_PAGE_LOC + x] << 8) |
                 st->ram[SMBC_SPROBJ_X_POSITION + x];
  uint16_t mid = ((uint16_t)mid_page << 8) | mid_x;
  uint16_t ul = SMBC_BOUNDING_BOX_UL_XPOS + y;
  uint16_t dr = SMBC_BOUNDING_BOX_DR_XPOS + y;
  if (obj < mid) {
    /* left side */
    if ((st->ram[ul] & 0x80u) == 0x00u) return;
    if (st->ram[ul] < 0xa0u) return;
    if ((st->ram[dr] & 0x80u) != 0x00u) st->ram[dr] = 0x00u;
    st->ram[ul] = 0x00u;
    return;
  }
  if ((st->ram[dr] & 0x80u) == 0x00u) return;
  if ((st->ram[ul] & 0x80u) != 0x00u) st->ram[ul] = 0xffu;
  st->ram[dr] = 0xffu;
}

void get_enemy_bound_box(CoreState *st, uint8_t x) {
  get_masked_off_scr_bits(st, x, 0x48u, 0x44u);
  if (st->ram[SMBC_ENEMY_OFFSCR_MASKED + x] != 0x00u) {
    move_bound_box_offscreen(st, x);
    return;
  }
  /* smb.asm: SetupEOffsetFBBox */
  bounding_box_core(st, (uint8_t)(x + 1u), 0x01u);
  check_right_screen_bbox(st, (uint8_t)(x + 1u), (uint8_t)((x + 1u) * 4u));
}

uint8_t get_enemy_bound_box_ofs(CoreState *st, uint8_t slot,
                                uint8_t *all_set) {
  uint8_t masked = (uint8_t)(st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0fu);
  *all_set = masked == 0x0fu ? 0x01u : 0x00u;
  return (uint8_t)(slot * 4u + 4u);
}
