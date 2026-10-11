/* tests/core/whirlpool.c — exercises src/core/whirlpool.c:
 * ProcessWhirlpools' water and timer gates, the five-slot scan, the
 * left/right extent test, the alternate-frame horizontal pull towards
 * the centre and the vertical gravity handed to the player. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/state.h"
#include "../../src/core/whirlpool.h"

static void set_water(Smbc *smbc) {
  smbc->state.ram[SMBC_AREA_TYPE] = 0x00u; /* water */
  smbc->state.ram[SMBC_TIMER_CONTROL] = 0x00u;
}

static void set_player(Smbc *smbc, uint8_t page, uint8_t x) {
  smbc->state.ram[SMBC_PLAYER_PAGE_LOC] = page;
  smbc->state.ram[SMBC_PLAYER_X_POSITION] = x;
}

static void set_slot(Smbc *smbc, uint8_t y, uint8_t page, uint8_t left,
                     uint8_t length) {
  smbc->state.ram[SMBC_WHIRLPOOL_PAGE_LOC + y] = page;
  smbc->state.ram[SMBC_WHIRLPOOL_LEFT_EXTENT + y] = left;
  smbc->state.ram[SMBC_WHIRLPOOL_LENGTH + y] = length;
}

static void non_water_leaves_flag(void) {
  /* the AreaType check comes before the flag is cleared */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_WHIRLPOOL_FLAG] = 0x5au;
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_FLAG] == 0x5au);
}

static void timer_control_zeroes_flag_then_leaves(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x01u;
  smbc.state.ram[SMBC_WHIRLPOOL_FLAG] = 0x5au;
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_FLAG] == 0x00u);
}

static void empty_slot_skipped(void) {
  /* a slot with a zero page location is not a whirlpool */
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x01u, 0x50u);
  set_slot(&smbc, 0x04u, 0x00u, 0x40u, 0x20u);
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_MOVE_FORCE] == 0x00u);
}

static void player_too_far_left_not_caught(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x01u, 0x10u); /* left of the 0x40 extent */
  set_slot(&smbc, 0x04u, 0x01u, 0x40u, 0x20u);
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_FLAG] == 0x00u);
}

static void player_past_right_not_caught(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x02u, 0x00u); /* past the right extent 0x0160 */
  set_slot(&smbc, 0x04u, 0x01u, 0x40u, 0x20u);
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_FLAG] == 0x00u);
}

static void inside_applies_gravity_no_pull_on_even_frame(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x01u, 0x50u);
  set_slot(&smbc, 0x04u, 0x01u, 0x40u, 0x20u);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u; /* even frame: no pull */
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x50u);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_MOVE_FORCE] == 0x10u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0x00u);
}

static void odd_frame_pulls_right_when_left_of_centre(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x01u, 0x48u); /* centre is 0x50 */
  set_slot(&smbc, 0x04u, 0x01u, 0x40u, 0x20u);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0x01u;
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x49u);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x01u);
}

static void right_pull_needs_collision_bit(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x01u, 0x48u);
  set_slot(&smbc, 0x04u, 0x01u, 0x40u, 0x20u);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0x00u; /* bit 0 clear */
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x48u);
}

static void odd_frame_pulls_left_when_right_of_centre(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x01u, 0x58u); /* centre is 0x50 */
  set_slot(&smbc, 0x04u, 0x01u, 0x40u, 0x20u);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x57u);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x01u);
}

static void highest_matching_slot_wins(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_water(&smbc);
  set_player(&smbc, 0x01u, 0x50u);
  /* slot 4: centre 0x40, player to its right -> pull left to 0x4f */
  set_slot(&smbc, 0x04u, 0x01u, 0x10u, 0x60u);
  /* slot 3: centre 0x60, player to its left -> would pull right */
  set_slot(&smbc, 0x03u, 0x01u, 0x30u, 0x60u);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u;
  process_whirlpools(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x4fu);
}

int main(void) {
  non_water_leaves_flag();
  timer_control_zeroes_flag_then_leaves();
  empty_slot_skipped();
  player_too_far_left_not_caught();
  player_past_right_not_caught();
  inside_applies_gravity_no_pull_on_even_frame();
  odd_frame_pulls_right_when_left_of_centre();
  right_pull_needs_collision_bit();
  odd_frame_pulls_left_when_right_of_centre();
  highest_matching_slot_wins();
  puts("tests/core/whirlpool: ok");
  return 0;
}
