/* tests/core/cannon.c — exercises src/core/cannon.c: ProcessCannons'
 * firing decisions across the first three enemy slots and
 * BulletBillHandler's movement, too-close check and collision path. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/cannon.h"
#include "../../src/core/enemies.h"
#include "../../src/core/frame.h"
#include "../../src/core/state.h"

static void set_screen(Smbc *smbc) {
  smbc->state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x00u;
  smbc->state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
}

/* mark slots one and zero busy with a non-bullet-bill so the loop's
 * later passes do nothing observable */
static void occupy_low_slots(Smbc *smbc) {
  for (uint8_t x = 0x00u; x < 0x02u; x++) {
    smbc->state.ram[SMBC_ENEMY_FLAG + x] = 0x01u;
    smbc->state.ram[SMBC_ENEMY_ID + x] = 0x06u;
  }
}

static void water_area_leaves(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u; /* water */
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 3u] = 0x02u;
  smbc.state.ram[SMBC_CANNON_PAGE_LOC + 2u] = 0x01u;
  smbc.state.ram[SMBC_CANNON_TIMER + 2u] = 0x00u;
  process_cannons(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x00u);
  assert(smbc.state.ram[SMBC_CANNON_TIMER + 2u] == 0x00u);
}

static void cannon_fires_spawns_bill_and_skips_handler(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  occupy_low_slots(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 3u] = 0x02u; /* masked: 2 */
  smbc.state.ram[SMBC_CANNON_PAGE_LOC + 2u] = 0x01u;
  smbc.state.ram[SMBC_CANNON_X_POSITION + 2u] = 0x50u;
  smbc.state.ram[SMBC_CANNON_Y_POSITION + 2u] = 0x30u;
  smbc.state.ram[SMBC_CANNON_TIMER + 2u] = 0x00u;
  /* the spawned bill lands on the player: if the handler ran on the
   * spawn frame the fresh-bill logic would kill it */
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x50u;
  process_cannons(&smbc.state);

  assert(smbc.state.ram[SMBC_CANNON_TIMER + 2u] == 0x0eu);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 2u] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 2u] == 0x50u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2u] == 0x28u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 2u] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 2u] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 2u] == 0x09u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 2u] == SMBC_BULLET_BILL_CANNON);
}

static void cannon_timer_counts_down(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  occupy_low_slots(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 3u] = 0x02u;
  smbc.state.ram[SMBC_CANNON_PAGE_LOC + 2u] = 0x01u;
  smbc.state.ram[SMBC_CANNON_TIMER + 2u] = 0x05u;
  process_cannons(&smbc.state);
  assert(smbc.state.ram[SMBC_CANNON_TIMER + 2u] == 0x04u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x00u);
}

static void high_lsfr_skips_cannon(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  occupy_low_slots(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 3u] = 0x06u; /* masked: 6 */
  smbc.state.ram[SMBC_CANNON_PAGE_LOC + 2u] = 0x01u;
  smbc.state.ram[SMBC_CANNON_TIMER + 2u] = 0x00u;
  process_cannons(&smbc.state);
  assert(smbc.state.ram[SMBC_CANNON_TIMER + 2u] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x00u);
}

static void page_zero_skips_cannon(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  occupy_low_slots(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 3u] = 0x02u;
  smbc.state.ram[SMBC_CANNON_PAGE_LOC + 2u] = 0x00u;
  smbc.state.ram[SMBC_CANNON_TIMER + 2u] = 0x00u;
  process_cannons(&smbc.state);
  assert(smbc.state.ram[SMBC_CANNON_TIMER + 2u] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x00u);
}

static void timer_control_blocks_fire(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  occupy_low_slots(&smbc);
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x01u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 3u] = 0x02u;
  smbc.state.ram[SMBC_CANNON_PAGE_LOC + 2u] = 0x01u;
  smbc.state.ram[SMBC_CANNON_TIMER + 2u] = 0x00u;
  process_cannons(&smbc.state);
  assert(smbc.state.ram[SMBC_CANNON_TIMER + 2u] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x00u);
}

static void chk_bb_ignores_non_bullet_bill(void) {
  /* a far-offscreen non-bullet-bill is left alone: OffscreenBoundsCheck
   * never runs because the identifier gate rejects it */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  occupy_low_slots(&smbc);
  smbc.state.ram[SMBC_ENEMY_FLAG + 2u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 2u] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 2u] = 0x05u; /* far right */
  process_cannons(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x01u);
}

static void chk_bb_runs_handler_on_bullet_bill(void) {
  /* an onscreen bullet bill right on top of the player is killed by
   * BulletBillHandler, proving Chk_BB hands it off */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  occupy_low_slots(&smbc);
  smbc.state.ram[SMBC_ENEMY_FLAG + 2u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 2u] = SMBC_BULLET_BILL_CANNON;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 2u] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 2u] = 0x30u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x30u;
  process_cannons(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2u] == 0x00u);
}

static void bbh_fresh_offscreen_bits_kills(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_FLAG + 0u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0u] = SMBC_BULLET_BILL_CANNON;
  smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] = 0x0cu;
  bullet_bill_handler(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0u] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0u] == 0x00u);
}

static void bbh_fresh_too_close_kills(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_FLAG + 0u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0u] = SMBC_BULLET_BILL_CANNON;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0u] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0u] = 0x50u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x50u; /* zero difference */
  bullet_bill_handler(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0u] == 0x00u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == 0x00u);
}

static void bbh_moves_left_towards_player(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_FLAG + 0u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0u] = SMBC_BULLET_BILL_CANNON;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0u] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0u] = 0x90u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x10u; /* player to the left */
  bullet_bill_handler(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0u] == 0x02u); /* left */
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0u] == 0xe8u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0u] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 0u] == 0x0au);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == SMBC_SFX_BLAST);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0u] == 0x01u);
}

static void bbh_moves_right_towards_player(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_FLAG + 0u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0u] = SMBC_BULLET_BILL_CANNON;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0u] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0u] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x90u; /* player to the right */
  bullet_bill_handler(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0u] == 0x01u); /* right */
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0u] == 0x18u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0u] == 0x01u);
}

static void bbh_defeated_state_moves_without_bullet_speed(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_FLAG + 0u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0u] = SMBC_BULLET_BILL_CANNON;
  smbc.state.ram[SMBC_ENEMY_STATE + 0u] = 0x20u; /* d5: defeated */
  bullet_bill_handler(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0u] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0u] == 0x00u);
}

static void bbh_timer_control_skips_movement(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 0u] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 0u] = SMBC_BULLET_BILL_CANNON;
  bullet_bill_handler(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0u] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0u] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0u] == 0x00u);
}

int main(void) {
  water_area_leaves();
  cannon_fires_spawns_bill_and_skips_handler();
  cannon_timer_counts_down();
  high_lsfr_skips_cannon();
  page_zero_skips_cannon();
  timer_control_blocks_fire();
  chk_bb_ignores_non_bullet_bill();
  chk_bb_runs_handler_on_bullet_bill();
  bbh_fresh_offscreen_bits_kills();
  bbh_fresh_too_close_kills();
  bbh_moves_left_towards_player();
  bbh_moves_right_towards_player();
  bbh_defeated_state_moves_without_bullet_speed();
  bbh_timer_control_skips_movement();
  puts("tests/core/cannon: ok");
  return 0;
}
