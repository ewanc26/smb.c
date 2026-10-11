/* tests/core/runners.c — exercises src/core/runners.c: the offscreen
 * bounds check. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/runners.h"

/* screen spanning $0020-$0110 */
static void set_screen(Smbc *smbc) {
  smbc->state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x00u;
  smbc->state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
}

static void set_enemy(Smbc *smbc, uint8_t id, uint8_t state, uint8_t page,
                      uint8_t x) {
  smbc->state.ram[SMBC_ENEMY_ID + 0] = id;
  smbc->state.ram[SMBC_ENEMY_STATE + 0] = state;
  smbc->state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc->state.ram[SMBC_ENEMY_PAGE_LOC + 0] = page;
  smbc->state.ram[SMBC_ENEMY_X_POSITION + 0] = x;
}

static uint8_t erased(const Smbc *smbc) {
  return (uint8_t)(smbc->state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
}

static void onscreen_survives(void) {
  /* an object within the bounds is left alone; the left bound wraps
   * below zero here, exercising the page borrow */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x06u, 0x01u, 0x00u, 0x80u); /* green koopa troopa */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(!erased(&smbc));
}

static void too_far_left_erased(void) {
  /* a goomba seventy-three pixels left of the left edge is erased:
   * identifiers below $0d subtract $49 */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x06u, 0x01u, 0x00u, 0x00u);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x80u; /* $80 - $49 = $37 */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(erased(&smbc));
}

static void hammer_bro_limit(void) {
  /* hammer bros add $39 and then subtract $49 (the adc did not
   * overflow): with the left edge at $80 the bound is $70, sixteen
   * pixels back */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x80u;
  set_enemy(&smbc, 0x05u, 0x01u, 0x00u, 0x6fu); /* one past the bound */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(erased(&smbc));

  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x80u;
  set_enemy(&smbc, 0x05u, 0x01u, 0x00u, 0x70u); /* exactly at the bound */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(!erased(&smbc));
}

static void too_far_right_erased(void) {
  /* past the right edge plus the margin (the adc adds $49 when the
   * left subtraction borrowed) an ordinary object is erased */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x06u, 0x01u, 0x01u, 0xa0u); /* right bound: $159 */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(erased(&smbc));
}

static void right_side_exemptions(void) {
  /* the spiny's egg state, piranha plants, flagpole flags, star
   * flags and jumpsprings are never erased on the right */
  static const struct {
    uint8_t id;
    uint8_t state;
  } cases[] = {
      {0x09u, 0x05u}, /* spiny in the egg state */
      {0x0du, 0x01u}, /* piranha plant */
      {0x30u, 0x01u}, /* flagpole flag */
      {0x31u, 0x01u}, /* star flag */
      {0x32u, 0x01u}, /* jumpspring */
  };
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    Smbc smbc;
    smbc_init(&smbc);
    set_screen(&smbc);
    set_enemy(&smbc, cases[i].id, cases[i].state, 0x01u, 0xa0u);
    offscreen_bounds_check(&smbc.state, 0x00u);
    assert(!erased(&smbc));
  }
}

static void flying_cheep_cheep_exempt(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x14u, 0x01u, 0x00u, 0x00u); /* far left */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(!erased(&smbc));
}


/* the following exercises were added with the MoveNormalEnemy atom */

static void normal_state_moves_horizontally(void) {
  /* state $00: plain horizontal movement, speed preserved */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x08u;
  move_normal_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x00u); /* 0.5px */
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x80u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x08u); /* restored */
  move_normal_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x01u);
}

static void d6_state_slows_non_powerup(void) {
  /* d6 set, state $02: gravity runs, then slowed horizontal movement
   * with the $e8 adder for a rightward speed */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x42u; /* d6 plus state $02 */
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x00u;
  move_normal_enemy(&smbc.state, 0x00u);
  /* temp speed $10 + $e8 = $f8: high nybble $f (sign-extended $ff),
   * force $80 → position $00 + $ff + $00 = $ff, page borrow */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u); /* restored */
}

static void d6_powerup_not_slowed(void) {
  /* the same state on a power-up object skips the slow adder */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x2eu;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x41u; /* d6 plus state $01 */
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x00u;
  move_normal_enemy(&smbc.state, 0x00u);
  /* state $01 is not $02, d6 set, power-up: plain steady movement */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x10u);
}

static void defeated_enemy_falls(void) {
  /* d5 set: gravity and horizontal movement, nothing else */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x08u;
  move_normal_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x80u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] != 0x00u);
}

static void stunned_revives_on_timer_expiry(void) {
  /* state $03 with an expired interval timer: state cleared, a
   * pseudorandom direction from the frame counter, revived speed */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x00u; /* green koopa troopa */
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x03u;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x00u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x05u; /* d0 set: direction 2 */
  move_normal_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf8u);

  /* hard mode uses the faster pair */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x03u;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x00u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x04u; /* d0 clear: direction 1 */
  smbc.state.ram[SMBC_PRIMARY_HARD_MODE] = 0x01u;
  move_normal_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x0cu);
}

static void stunned_goomba_killed_at_timer_0e(void) {
  /* an unexpired timer of $0e erases a goomba but no other enemy */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x03u;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x0eu;
  move_normal_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x00u; /* not a goomba */
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x03u;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x0eu;
  move_normal_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  /* and the state is untouched while the timer runs */
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x03u);
}

static void jumping_enemy_moves(void) {
  /* MoveJumpingEnemy: jumping gravity plus horizontal movement */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x08u;
  move_jumping_enemy(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x80u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] != 0x00u);
}

static void red_p_troopa_resting_nudge(void) {
  /* at rest above the origin: one pixel up every eighth frame */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x30u;
  smbc.state.ram[SMBC_RED_PTROOPA_ORIG_X_POS + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 0] = 0x55u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x10u; /* 3 LSB clear */
  proc_move_red_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x31u);
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 0] == 0x00u);

  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x11u; /* 3 LSB set: no nudge */
  proc_move_red_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x31u);
}

static void red_p_troopa_seeks_centre(void) {
  /* with speed or force present, the paratroopa gravitates towards
   * its central coordinate: below it moves up, above it moves down */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_RED_PTROOPA_CENTER_Y_POS + 0] = 0x40u;
  proc_move_red_p_troopa(&smbc.state, 0x00u);
  /* below centre: MoveRedPTroopaUp — the force ($03) underflows the
   * up amount ($06), so the borrow takes the speed down a notch */
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x51u);
}


int main(void) {
  onscreen_survives();
  too_far_left_erased();
  hammer_bro_limit();
  too_far_right_erased();
  right_side_exemptions();
  flying_cheep_cheep_exempt();
  normal_state_moves_horizontally();
  d6_state_slows_non_powerup();
  d6_powerup_not_slowed();
  defeated_enemy_falls();
  stunned_revives_on_timer_expiry();
  stunned_goomba_killed_at_timer_0e();
  jumping_enemy_moves();
  red_p_troopa_resting_nudge();
  red_p_troopa_seeks_centre();
  puts("tests/core/runners: ok");
  return 0;
}
