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


static void shell_defeat_scores_and_stuns(void) {
  /* a goomba: stunned, defeated, 100 points, smack queued */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_REL_XPOS] = 0x77u;
  shell_or_block_defeat(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x22u) == 0x22u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0xc0u) == 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_TIMER + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_Y_POS + 0] == 0x3eu);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_X_POS + 0] == 0x77u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x08u);

  /* a hammer bro scores 1000 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_HAMMER_BRO;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  shell_or_block_defeat(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x06u);

  /* anything else scores the default 200 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x02u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  shell_or_block_defeat(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x02u);

  /* a piranha plant sinks $18 first and keeps its id — the
   * adjusted coordinate fails the demote tests */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_PIRANHA_PLANT;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x20u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  shell_or_block_defeat(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == SMBC_PIRANHA_PLANT);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_Y_POS + 0] == 0x36u);
}


static void collision_core_overlaps(void) {
  /* overlapping boxes collide */
  Smbc smbc;
  smbc_init(&smbc);
  uint16_t a = SMBC_BOUNDING_BOX_UL_CORNER;     /* box a: slots 0 */
  uint16_t b = SMBC_BOUNDING_BOX_UL_CORNER + 8u; /* box b: slot 1's */
  smbc.state.ram[a] = 0x10u;
  smbc.state.ram[a + 1u] = 0x10u;
  smbc.state.ram[a + 2u] = 0x20u;
  smbc.state.ram[a + 3u] = 0x20u;
  smbc.state.ram[b] = 0x18u;
  smbc.state.ram[b + 1u] = 0x18u;
  smbc.state.ram[b + 2u] = 0x28u;
  smbc.state.ram[b + 3u] = 0x28u;
  assert(spr_object_collision_core(&smbc.state, 0x00u, 0x08u) == 0x01u);

  /* touching edges count */
  smbc.state.ram[b] = 0x20u; /* left edge == a's right edge */
  smbc.state.ram[b + 1u] = 0x20u;
  assert(spr_object_collision_core(&smbc.state, 0x00u, 0x08u) == 0x01u);

  /* separated horizontally: no collision */
  smbc.state.ram[b] = 0x30u;
  smbc.state.ram[b + 2u] = 0x40u;
  assert(spr_object_collision_core(&smbc.state, 0x00u, 0x08u) == 0x00u);

  /* the player entry compares against slot 0's box */
  smbc.state.ram[b] = 0x18u;
  smbc.state.ram[b + 2u] = 0x28u;
  assert(player_collision_core(&smbc.state, 0x08u) == 0x01u);
}

static void collision_core_wraps(void) {
  /* a wrapped first box (right above its left) collides via the
   * second box's vertical check */
  Smbc smbc;
  smbc_init(&smbc);
  uint16_t a = SMBC_BOUNDING_BOX_UL_CORNER;
  uint16_t b = SMBC_BOUNDING_BOX_UL_CORNER + 8u;
  smbc.state.ram[a] = 0x30u;      /* left */
  smbc.state.ram[a + 1u] = 0x10u; /* top */
  smbc.state.ram[a + 2u] = 0x10u; /* right < left: wrapped */
  smbc.state.ram[a + 3u] = 0x20u;
  smbc.state.ram[b] = 0x05u;
  smbc.state.ram[b + 1u] = 0x18u; /* vertical overlaps a's */
  smbc.state.ram[b + 2u] = 0x08u;
  smbc.state.ram[b + 3u] = 0x28u;
  assert(spr_object_collision_core(&smbc.state, 0x00u, 0x08u) == 0x01u);

  /* but a wrapped second box on the greater path is NOT a
   * collision — the asm's asymmetry */
  smbc.state.ram[a] = 0x10u;
  smbc.state.ram[a + 2u] = 0x20u;
  smbc.state.ram[b] = 0x30u;
  smbc.state.ram[b + 1u] = 0x30u;
  smbc.state.ram[b + 2u] = 0x08u; /* right < left */
  smbc.state.ram[b + 3u] = 0x08u;
  assert(spr_object_collision_core(&smbc.state, 0x00u, 0x08u) == 0x00u);
}


static void turn_around_rules(void) {
  /* a goomba turns: speed negated, direction inverted */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  enemy_turn_around(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);

  /* a spiny turns despite being >= $07 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_SPINY;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0xf0u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x02u;
  enemy_turn_around(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);

  /* a bloober (>= $07, not spiny/paratroopa) does not */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_BLOOBER;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  enemy_turn_around(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);

  /* nor a hammer bro */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_HAMMER_BRO;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  enemy_turn_around(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u);
}

static void enemy_collision_reactions(void) {
  /* two walkers: both turn around */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 1] = 0xf0u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 1] = 0x02u;
  proc_enemy_collisions(&smbc.state, 0x00u, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 1] == 0x10u);

  /* d5 in either state: nothing happens */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_STATE + 1] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  proc_enemy_collisions(&smbc.state, 0x00u, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u);

  /* the second enemy a shell: the first dies, the chain counter
   * feeds the score and increments */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = 0x00u; /* koopa shell */
  smbc.state.ram[SMBC_ENEMY_STATE + 1] = 0x06u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_SHELL_CHAIN_COUNTER + 1] = 0x02u;
  proc_enemy_collisions(&smbc.state, 0x00u, 0x01u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x20u) != 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_SHELL_CHAIN_COUNTER + 1] == 0x03u);

  /* shell versus moving shell: the asm's 1000-point floatey for
   * the x enemy is immediately overwritten by ShellOrBlockDefeat's
   * own score call, so it keeps the koopa default, and the y
   * enemy dies with the chain score */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x86u; /* d7 + >= 6 */
  smbc.state.ram[SMBC_ENEMY_STATE + 1] = 0x86u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  proc_enemy_collisions(&smbc.state, 0x00u, 0x01u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 1] == 0x04u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x20u) != 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 1] & 0x20u) != 0x00u);
  assert(smbc.state.ram[SMBC_SHELL_CHAIN_COUNTER + 0] == 0x01u);

  /* a hammer bro in an alt state kills nothing */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_HAMMER_BRO;
  smbc.state.ram[SMBC_ENEMY_STATE + 1] = 0x07u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  proc_enemy_collisions(&smbc.state, 0x00u, 0x01u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x20u) == 0x00u);
}


static void enemies_collision_gates(void) {
  /* even frame: nothing happens */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x02u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_GOOMBA;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);

  /* water area: nothing */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_GOOMBA;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);

  /* lakitu slot: nothing */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_LAKITU;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);

  /* masked offscreen bits: nothing */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_OFFSCR_MASKED + 1] = 0x40u;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);
}

static void enemies_collision_pairs(void) {
  /* two overlapping goombas on an odd frame: the bit sets and
   * both turn around */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 1] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 1] = 0xf0u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 1] = 0x02u;
  /* overlapping boxes: slot 0 at $04ac+4, slot 1 at $04ac+8 */
  uint16_t b0 = SMBC_BOUNDING_BOX_UL_CORNER + 4u;
  uint16_t b1 = SMBC_BOUNDING_BOX_UL_CORNER + 8u;
  smbc.state.ram[b0] = 0x10u;
  smbc.state.ram[b0 + 1u] = 0x10u;
  smbc.state.ram[b0 + 2u] = 0x20u;
  smbc.state.ram[b0 + 3u] = 0x20u;
  smbc.state.ram[b1] = 0x18u;
  smbc.state.ram[b1 + 1u] = 0x18u;
  smbc.state.ram[b1 + 2u] = 0x28u;
  smbc.state.ram[b1 + 3u] = 0x28u;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x40u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 1] == 0x10u);

  /* still overlapping on the next odd frame: the bit is already
   * set, so no second reaction */
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x03u;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf0u);

  /* separate them: the bit clears */
  smbc.state.ram[b1] = 0x40u;
  smbc.state.ram[b1 + 2u] = 0x50u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x05u;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);

  /* a dormant slot is skipped */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = SMBC_GOOMBA;
  enemies_collision(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);
}


static void stomp_family(void) {
  /* check_player_vertical gates on offscreen bits, the high
   * byte, and the $d0 row */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  assert(check_player_vertical(&smbc.state) == 0x01u);
  smbc.state.ram[SMBC_PLAYER_OFFSCREEN_BITS] = 0xf0u;
  assert(check_player_vertical(&smbc.state) == 0x00u);
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x02u;
  assert(check_player_vertical(&smbc.state) == 0x00u);
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xd0u;
  assert(check_player_vertical(&smbc.state) == 0x00u);

  /* init_vstf zeroes both and returns zero */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x22u;
  smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] = 0x33u;
  assert(init_vstf(&smbc.state, 0x00u) == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x00u);

  /* enemy_face_player: left of player faces left, offset 1 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  assert(enemy_face_player(&smbc.state, 0x00u) == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);

  /* stomping a bloober: 600 points, direction preserved, d5
   * set, speeds zeroed, player bounces */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_BLOOBER;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x02u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  enemy_stomped(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x04u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfdu);

  /* stomping a goomba is a shell stomp: state $04, chain score,
   * revival timer, $fc bounce */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_STOMP_CHAIN_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_STOMP_TIMER] = 0x02u;
  smbc.state.ram[SMBC_PRIMARY_HARD_MODE] = 0x01u;
  enemy_stomped(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x04u);
  assert(smbc.state.ram[SMBC_STOMP_CHAIN_COUNTER] == 0x02u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x04u);
  assert(smbc.state.ram[SMBC_STOMP_TIMER] == 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] == 0x0bu);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfcu);

  /* stomping a jumping paratroopa demotes it: id LSB, normal
   * state, 400 points, demoted speed facing the player */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GREEN_PARATROOPA_JUMP;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  enemy_stomped(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x08u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfcu);
}


static void injury_family(void) {
  /* a big player shrinks: small status, $08 injury timer, the
   * pipedown sound, subroutine $0a, halted timers */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x01u;
  force_injury(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x00u);
  assert(smbc.state.ram[SMBC_INJURY_TIMER] == 0x08u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x10u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0au);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x01u);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0xffu);
  assert(smbc.state.ram[SMBC_SCROLL_AMOUNT] == 0x00u);

  /* a small player dies: speed halted, death music, $fc
   * vertical speed, subroutine $0b */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x18u;
  force_injury(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfcu);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0bu);

  /* injure_player respects a running injury timer */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x01u;
  smbc.state.ram[SMBC_INJURY_TIMER] = 0x04u;
  injure_player(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x01u);

  /* falling onto a goomba stomps it */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  assert(chk_for_player_injury(&smbc.state, 0x00u) == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x04u);

  /* moving up into a goomba: injured, and the right-moving
   * enemy with the player on its left turns around first */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_REL_XPOS] = 0x30u;
  assert(chk_for_player_injury(&smbc.state, 0x00u) == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x00u);

  /* a standing player under a bloober, the $0c-adjusted
   * position above the enemy: stomped despite not falling */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_BLOOBER;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  assert(chk_for_player_injury(&smbc.state, 0x00u) == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x20u);

  /* a running stomp timer stomps even a moving-up player */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_STOMP_TIMER] = 0x01u;
  assert(chk_for_player_injury(&smbc.state, 0x00u) == 0x01u);
}


static void power_up_collision(void) {
  /* a star: invincibility timer and star music */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_POWER_UP_TYPE] = 0x04u;
  handle_power_up_collision(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == 0x20u);
  assert(smbc.state.ram[SMBC_STAR_INVINCIBLE_TIMER] == 0x23u);
  assert(smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] == 0x40u);

  /* a 1-up: the floatey becomes $0b */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_POWER_UP_TYPE] = 0x03u;
  handle_power_up_collision(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x0bu);
  assert(smbc.state.ram[SMBC_STAR_INVINCIBLE_TIMER] == 0x00u);

  /* a mushroom on a small player: super, subroutine $09 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_POWER_UP_TYPE] = 0x00u;
  handle_power_up_collision(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x01u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x09u);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x00u);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0xffu);

  /* a flower on a super player: fiery, subroutine $0c */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_POWER_UP_TYPE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x01u;
  handle_power_up_collision(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x02u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0cu);

  /* a fiery player collecting one: nothing changes */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_POWER_UP_TYPE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u;
  handle_power_up_collision(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x02u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x00u);
}

static void pe_collisions_dispatch(void) {
  /* kicking a stunned koopa: d7 sets, it faces the player at
   * the kicked speed, the smack plays, points from the timer */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GREEN_KOOPA;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  handle_pe_collisions(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x82u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x08u);
  assert(smbc.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x06u);

  /* a second contact with the bit set: nothing */
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x30u;
  handle_pe_collisions(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x30u);

  /* a podoboo injures directly */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_PODOBOO;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  handle_pe_collisions(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x00u);
  assert(smbc.state.ram[SMBC_INJURY_TIMER] == 0x08u);

  /* a walking goomba goes through the stomp decision */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0x02u;
  handle_pe_collisions(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x04u);

  /* a defeated goomba in a kicked state leaves alone */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  handle_pe_collisions(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x80u) == 0x00u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x00u);
}

static void player_enemy_collision_gates(void) {
  /* the even-frame gate: nothing */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  player_enemy_collision(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);

  /* a miss clears d0 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] = 0x01u;
  smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER + 4u] = 0x40u;
  smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER + 5u] = 0x40u;
  smbc.state.ram[SMBC_BOUNDING_BOX_LR_CORNER + 4u] = 0x50u;
  smbc.state.ram[SMBC_BOUNDING_BOX_LR_CORNER + 5u] = 0x50u;
  player_enemy_collision(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_COLLISION_BITS + 0] == 0x00u);

  /* a hit on a star-powered player defeats the enemy */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_STAR_INVINCIBLE_TIMER] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  /* overlapping boxes: player slot 0, enemy slot 0 at +4 */
  smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER] = 0x10u;
  smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER + 1u] = 0x10u;
  smbc.state.ram[SMBC_BOUNDING_BOX_LR_CORNER] = 0x20u;
  smbc.state.ram[SMBC_BOUNDING_BOX_LR_CORNER + 1u] = 0x20u;
  smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER + 4u] = 0x18u;
  smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER + 5u] = 0x18u;
  smbc.state.ram[SMBC_BOUNDING_BOX_LR_CORNER + 4u] = 0x28u;
  smbc.state.ram[SMBC_BOUNDING_BOX_LR_CORNER + 5u] = 0x28u;
  player_enemy_collision(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x20u) != 0x00u);
}

int main(void) {
  masked_bits_park_the_box();
  corners_from_ctrl_data();
  right_edge_wraps_offscreen();
  left_edge_wraps_onscreen();
  bound_box_ofs_helper();
  stun_demotes_and_sets_state();
  stun_speeds_by_area_and_id();
  shell_defeat_scores_and_stuns();
  collision_core_overlaps();
  collision_core_wraps();
  turn_around_rules();
  enemy_collision_reactions();
  enemies_collision_gates();
  enemies_collision_pairs();
  stomp_family();
  injury_family();
  power_up_collision();
  pe_collisions_dispatch();
  player_enemy_collision_gates();
  puts("collide: ok");
  return 0;
}
