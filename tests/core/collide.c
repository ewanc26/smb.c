/* tests/core/collide.c — exercises src/core/collide.c: the enemy
 * bounding box computation and offset helper. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/collide.h"
#include "../../src/core/enemies.h"
#include "../../src/core/frame.h"

static void masked_bits_park_the_box(void) {
  /* offscreen bits colliding with the mask: the box goes offscreen */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x40u; /* right of left edge */
  smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] = 0x48u;
  get_enemy_bound_box(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCR_MASKED + 0] == 0x48u);
  for (int i = 0; i < 4; i++)
    assert(smbc.state.ram[SMBC_ENEMY_BBOX_COORD + i] == 0xffu);

  /* beyond the left edge: the $44 mask applies */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x01u; /* enemy page 0: beyond */
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] = 0x0cu; /* d2-d3 only */
  get_enemy_bound_box(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCR_MASKED + 0] == 0x04u);

  /* at the left edge exactly: also the $44 mask, but nothing set */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x20u;
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] = 0x48u;
  get_enemy_bound_box(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCR_MASKED + 0] == 0x40u);
}

static void corners_from_ctrl_data(void) {
  /* onscreen: corners come from the relative position and the
   * control row; ctrl $01 = {03, 14, 0d, 20} */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS + 1] = 0x30u;
  smbc.state.ram[SMBC_SPROBJ_REL_YPOS + 1] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL + 1] = 0x01u;
  get_enemy_bound_box(&smbc.state, 0x00u);
  uint16_t box = SMBC_BOUNDING_BOX_UL_CORNER + 4u; /* slot 0's box */
  assert(smbc.state.ram[box] == 0x33u);     /* left  = 30+03 */
  assert(smbc.state.ram[box + 1u] == 0x64u); /* top   = 50+14 */
  assert(smbc.state.ram[box + 2u] == 0x3du); /* right = 30+0d */
  assert(smbc.state.ram[box + 3u] == 0x70u); /* bottom= 50+20 */
}

static void right_edge_wraps_offscreen(void) {
  /* right side of the middle with a wrapped right edge: both
   * horizontal edges go to $ff */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0xe0u; /* past middle */
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS + 1] = 0xe0u;
  smbc.state.ram[SMBC_SPROBJ_REL_YPOS + 1] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL + 1] = 0x01u;
  get_enemy_bound_box(&smbc.state, 0x00u);
  uint16_t box = SMBC_BOUNDING_BOX_UL_CORNER + 4u;
  assert(smbc.state.ram[box] == 0xffu);      /* left wrapped too */
  assert(smbc.state.ram[box + 2u] == 0xffu); /* right = e0+0d = ed */
  assert(smbc.state.ram[box + 1u] == 0x64u); /* vertical untouched */

  /* wrapped right edge but onscreen left edge: only the right
   * goes to $ff */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0xe0u;
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS + 1] = 0x78u;
  smbc.state.ram[SMBC_SPROBJ_REL_YPOS + 1] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL + 1] = 0x01u;
  get_enemy_bound_box(&smbc.state, 0x00u);
  assert(smbc.state.ram[box] == 0x7bu);      /* left = 78+03 */
  assert(smbc.state.ram[box + 2u] == 0xffu); /* right = 78+0d = 85 */
}

static void left_edge_wraps_onscreen(void) {
  /* left side of the middle with a wrapped left edge past $a0:
   * both horizontal edges go to $00 */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u; /* left of middle */
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS + 1] = 0xa0u;
  smbc.state.ram[SMBC_SPROBJ_REL_YPOS + 1] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL + 1] = 0x01u;
  get_enemy_bound_box(&smbc.state, 0x00u);
  uint16_t box = SMBC_BOUNDING_BOX_UL_CORNER + 4u;
  assert(smbc.state.ram[box] == 0x00u);      /* left = a0+03 = a3 */
  assert(smbc.state.ram[box + 2u] == 0x00u); /* right = a0+0d = ad */

  /* a wrapped left edge below $a0 stays */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS + 1] = 0x88u;
  smbc.state.ram[SMBC_SPROBJ_REL_YPOS + 1] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL + 1] = 0x01u;
  get_enemy_bound_box(&smbc.state, 0x00u);
  assert(smbc.state.ram[box] == 0x8bu);      /* left = 88+03, kept */
  assert(smbc.state.ram[box + 2u] == 0x95u); /* right = 88+0d */
}

static void bound_box_ofs_helper(void) {
  Smbc smbc;
  smbc_init(&smbc);
  uint8_t all_set = 0xffu;
  smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] = 0x1fu;
  uint8_t ofs = get_enemy_bound_box_ofs(&smbc.state, 0x02u, &all_set);
  assert(ofs == 0x0cu); /* slot 2 * 4 + 4 */
  assert(all_set == 0x01u);
  smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] = 0x10u;
  get_enemy_bound_box_ofs(&smbc.state, 0x00u, &all_set);
  assert(all_set == 0x00u);
}


static void stun_demotes_and_sets_state(void) {
  /* green paratroopa jump ($0e) demotes to green koopa ($00) */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x0eu;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0xa5u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  chk_to_stun_enemies(&smbc.state, 0x00u, 0x0eu);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0xa2u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x3eu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xfdu);
  /* enemy right of player: direction 1, speed $10 */
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u);

  /* id $09 demotes to $01 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  chk_to_stun_enemies(&smbc.state, 0x00u, 0x09u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x01u);

  /* a goomba keeps its id; enemy left of player faces left */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x06u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  chk_to_stun_enemies(&smbc.state, 0x00u, 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf0u);
}

static void stun_speeds_by_area_and_id(void) {
  /* a bloober always gets the $ff water speed */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_BLOOBER;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  chk_to_stun_enemies(&smbc.state, 0x00u, SMBC_BLOOBER);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xffu);

  /* anyone in a water area too */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x06u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  chk_to_stun_enemies(&smbc.state, 0x00u, 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xffu);

  /* a frenzy bullet bill keeps its direction but still takes
   * the horizontal speed */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_BULLET_BILL_FRENZY;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x02u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  chk_to_stun_enemies(&smbc.state, 0x00u, SMBC_BULLET_BILL_FRENZY);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u);
}

int main(void) {
  masked_bits_park_the_box();
  corners_from_ctrl_data();
  right_edge_wraps_offscreen();
  left_edge_wraps_onscreen();
  bound_box_ofs_helper();
  stun_demotes_and_sets_state();
  stun_speeds_by_area_and_id();
  puts("collide: ok");
  return 0;
}
