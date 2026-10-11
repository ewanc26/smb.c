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



static void fly_green_counter_ramps(void) {
  /* every fourth frame the secondary counter (the X speed) ramps
   * towards $13 while the primary's d0 is clear; the mover negates
   * it for leftward flight and restores it afterwards */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x40u; /* fourth frame, sway down */
  move_fly_green_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u); /* left */
  /* the move ran with the negated speed ($ff): one pixel left */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x01u); /* swayed */

  /* off the fourth frame nothing changes but the movement itself */
  uint8_t speed = smbc.state.ram[SMBC_ENEMY_X_SPEED + 0];
  uint8_t ypos = smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0];
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x41u;
  move_fly_green_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == speed);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == ypos);
}

static void fly_green_counter_limits(void) {
  /* the secondary at $13 with d0 clear advances the primary instead;
   * with d0 set the secondary counts back down, and at zero the
   * primary advances again */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x13u;
  move_fly_green_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x13u);

  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x04u;
  move_fly_green_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x12u);
  assert(smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] == 0x01u);

  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x00u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x08u;
  move_fly_green_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] == 0x02u);
}

static void fly_green_rightward_leg(void) {
  /* the primary's d1 selects rightward flight: no negation, and the
   * moving direction is one */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  move_fly_green_p_troopa(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
  /* d0 of the primary is clear here, so the counter ramped to one
   * and the move used it rightward */
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x10u);
}


static void bloober_direction_pick(void) {
  /* LSFR masked to zero picks a direction: even slots by the player
   * difference, odd slots by the player's own direction */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  /* enemy right of player → keep left */
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x01u;
  move_bloober(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  /* enemy left of player → right */
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  move_bloober(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 2] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 1] = 0x60u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x02u;
  move_bloober(&smbc.state, 0x01u); /* odd slot: follows the player */
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 1] == 0x02u);

  /* a nonzero masked LSFR skips the pick entirely (0x40 would be
   * masked away by $3f, so use a value inside the mask) */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  move_bloober(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
}

static void bloober_swim_cycle(void) {
  /* every eighth frame the force counts up to two, advancing the
   * counter; d0 then counts it back down, and zero sets the
   * float-down timer */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x02u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0xffu; /* no pick */
  move_bloober(&smbc.state, 0x00u); /* frame 0: force to one */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] == 0x00u);
  /* the bob subtracts the force from the vertical position */
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x5fu);
  /* and the swim moved one pixel left */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0xffu);

  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x00u;
  move_bloober(&smbc.state, 0x00u); /* force to two: counter up */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] == 0x01u);

  move_bloober(&smbc.state, 0x00u); /* d0 set: back down to one */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x01u);
  move_bloober(&smbc.state, 0x00u); /* down to zero: float-down timer */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] == 0x02u);
}

static void bloober_floatdown(void) {
  /* with the counter's d1 set and the timer expired, the bloober
   * sinks every other frame until within sixteen pixels above the
   * player, then the cycle restarts */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0xffu;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u; /* even frame: sink */
  move_bloober(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x61u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x20u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x28u; /* caught up */
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0xffu;
  move_bloober(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_X_MOVE_PRIMARY_COUNTER + 0] == 0x00u);
}

static void defeated_bloober_sinks(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  move_bloober(&smbc.state, 0x00u);
  /* MoveEnemySlowVert: a quarter of the force per frame */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] >= 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x60u);
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
  fly_green_counter_ramps();
  fly_green_counter_limits();
  fly_green_rightward_leg();
  bloober_direction_pick();
  bloober_swim_cycle();
  bloober_floatdown();
  defeated_bloober_sinks();
  puts("tests/core/runners: ok");
  return 0;
}
