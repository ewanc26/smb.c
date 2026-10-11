/* tests/core/runners.c — exercises src/core/runners.c: the offscreen
 * bounds check. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/enemies.h"
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


static void bullet_bill_moves(void) {
  /* undefeated: a fixed $e8 speed; defeated: jumping gravity */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  move_bullet_bill(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xe8u);
  /* $e8: high nybble $e sign-extends, so 2 pixels left per frame */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x0eu);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x80u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x20u;
  move_bullet_bill(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] != 0x00u);
}

static void cheep_cheep_drifts_left(void) {
  /* the horizontal force drains by the identifier's preset; each
   * underflow costs a pixel and eventually a page */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x0au; /* drain $40 */
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] = 0x40u;
  move_swimming_cheep_cheep(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x10u); /* no borrow */
  move_swimming_cheep_cheep(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0xc0u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x0fu); /* borrowed */

  /* slots zero and one never oscillate */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x0bu;
  move_swimming_cheep_cheep(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 0] == 0x00u);
}

static void cheep_cheep_oscillates(void) {
  /* slot two: with the flag below $10 the dummy drains and the
   * position ticks up on each underflow; fifteen pixels from the
   * origin flips the flag to downwards */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 2] = 0x0bu;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] = 0x60u;
  smbc.state.ram[SMBC_CHEEP_CHEEP_ORIG_Y_POS + 2] = 0x60u;
  move_swimming_cheep_cheep(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 2] == 0xe0u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] == 0x5fu); /* borrowed */
  assert(smbc.state.ram[SMBC_CHEEP_CHEEP_MOVE_M_FLAG + 2] == 0x00u);

  /* at fifteen pixels below the origin the flag flips downwards */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] = 0x51u;
  smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 2] = 0x20u; /* no borrow this time */
  move_swimming_cheep_cheep(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] == 0x51u);
  assert(smbc.state.ram[SMBC_CHEEP_CHEEP_MOVE_M_FLAG + 2] == 0x10u);

  /* the flip call drained the dummy to zero on its upward leg;
   * downwards now accumulates from there */
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 2] == 0x00u);
  move_swimming_cheep_cheep(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 2] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] == 0x51u);
  smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 2] = 0xe0u;
  move_swimming_cheep_cheep(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] == 0x52u); /* carried */
}

static void defeated_cheep_cheep_sinks(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 2] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 2] = 0x80u;
  move_swimming_cheep_cheep(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 2] == 0x80u); /* untouched */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 2] != 0x00u);
}


static void podoboo_reinit_and_rise(void) {
  /* timer expired: reinitialise below the screen with the LSFR
   * force and timer, an upward speed, then gravity every frame */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x25u;
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x00u;
  move_podoboo(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] == 0x07u);
  /* the gravity frame already ran against the $f9 speed: the $a5
   * force took $1c more and the podoboo rose seven pixels */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0xc1u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0xfbu);
  /* the high byte takes the sign extension of the still-$f9 speed */
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x01u);

  /* timer running: gravity only, no reinitialisation */
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x10u;
  uint8_t box = smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0];
  move_podoboo(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] == 0x07u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == box);
}

static void hammer_bro_jump_code(void) {
  /* jump timer expired and not already jumping: speed by screen
   * position, d0 of the state set, length timer by hard mode, jump
   * timer rearmed with d7-d6 set */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x50u; /* top half, above $70 */
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xfdu);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x01u) == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 0] == 0x20u); /* easy */
  assert((smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] & 0xc0u) == 0xc0u);

  /* bottom half: the deep $fa jump */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x90u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xfau);

  /* hard mode with the LSFR bit set: the long jump length */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x01u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 2] = 0x01u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 0] == 0x37u);

  /* already jumping: no new jump */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x01u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x00u);
}

static void hammer_bro_throws(void) {
  /* jump timer running and the throwing timer expired: the timer
   * resets by hard mode and a hammer spawns into a free misc slot,
   * marking d3 of the state */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x05u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x03u; /* misc slot 3 */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x50u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] == 0x04u);
  assert(smbc.state.ram[SMBC_HAMMER_THROWING_TIMER + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_MISC_STATE + 3] == 0x90u);
  assert(smbc.state.ram[SMBC_MISC_BOUND_BOX_CTRL + 3] == 0x07u);
  assert(smbc.state.ram[SMBC_HAMMER_ENEMY_OFFSET + 3] == 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x08u) == 0x08u);

  /* throwing timer running: decrements instead, and no d3 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x05u;
  smbc.state.ram[SMBC_HAMMER_THROWING_TIMER + 0] = 0x10u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_HAMMER_THROWING_TIMER + 0] == 0x0fu);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x08u) == 0x00u);

  /* misc slot taken: no spawn, no d3 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x05u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x03u;
  smbc.state.ram[SMBC_MISC_STATE + 3] = 0x01u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x08u) == 0x00u);

  /* a little offscreen: no throwing at all */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x05u;
  smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] = 0x04u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_HAMMER_THROWING_TIMER + 0] == 0x00u);
}

static void hammer_bro_shimmy_and_walk(void) {
  /* the shimmy flips with the frame counter's d6; the bro faces the
   * player and walks left at them when the walking timer expires */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x01u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  proc_hammer_bro(&smbc.state, 0x00u);
  /* enemy at the origin with the player: face left, walk left */
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf8u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x01u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x01u; /* walking timer */
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x04u); /* right shimmy */

  /* player to the right of the bro: face right, keep the shimmy */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xfcu);

  /* defeated bros just fall: the $3d gravity from rest */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x20u;
  proc_hammer_bro(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x3du);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x00u);
}


static void piranha_stays_in_pipe(void) {
  /* any state, an unexpired frame timer, or a nearby player keeps
   * the plant down; the background priority bit is always set */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x01u;
  move_piranha_plant(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] == 0x00u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 0] = 0x10u;
  move_piranha_plant(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] == 0x00u);

  /* player within $21 pixels: no reversal, plant stays put */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] = 0x01u; /* down: gate runs */
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x30u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x20u; /* $10 away */
  move_piranha_plant(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] == 0x01u);
}

static void piranha_emerges_and_moves(void) {
  /* a resting plant with the player far enough away reverses to
   * rise and climbs a pixel every other frame towards its up
   * coordinate, resting with a $40 timer on arrival */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x80u; /* far from player */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_PIRANHA_PLANT_UP_Y_POS + 0] = 0x5fu;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u; /* movement frame */
  move_piranha_plant(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] == 0xffu);
  /* rose one pixel and arrived in the same frame: the flag set by
   * the reversal clears again, and the rest timer sets */
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x5fu);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 0] == 0x40u);

  /* off the movement frame the plant holds still */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] = 0xffu;
  smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_PIRANHA_PLANT_UP_Y_POS + 0] = 0x50u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  move_piranha_plant(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x60u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] == 0x01u);

  /* a negative resting speed reverses without the proximity gate */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] = 0xffu;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  move_piranha_plant(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] == 0x01u);
}


static void lakitu_spawning_state(void) {
  /* any nonzero state resets the direction and frenzy buffer and
   * flies at $10, negated for the reset (leftward) direction */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x99u;
  move_lakitu(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf0u); /* -$10 */
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);

  /* defeated: falls with the $3d gravity, frenzy buffer untouched */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x99u;
  move_lakitu(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x3du);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] == 0x99u);
}

static void lakitu_chases_player(void) {
  /* active and close: the frenzy buffer takes the spiny id and the
   * speed comes from the quartered distance off the adjustment */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_LAKITU;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x40u; /* $40 right of player */
  move_lakitu(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] == SMBC_SPINY);
  /* diff $40 capped to $3c, quartered to $0f; no movement class:
   * $15 - $0f - 1 = $05, leftward */
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xfbu);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);

  /* running player with fast scroll: the $40 class */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_LAKITU;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x20u;
  smbc.state.ram[SMBC_SCROLL_AMOUNT] = 0x02u;
  /* keep the class with a vertical speed — but $a0 is also the
   * move direction, so use $02: nonzero, with d0 clear */
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x02u;
  move_lakitu(&smbc.state, 0x00u);
  /* diff $10, quartered $04: $40 - $04 - 1 = $3b, leftward */
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xc5u);

  /* moving right (direction d0 set): no negation */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_LAKITU;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] = 0x01u;
  move_lakitu(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
  assert((smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] & 0x80u) == 0x00u);
}

static void lakitu_turns_around(void) {
  /* beyond $3c and moving away: a right-moving lakitu (direction
   * d0 set, enemy right of the player) bleeds speed and the
   * decremented value is returned as-is */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_LAKITU;
  smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x80u; /* side 0 */
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x05u;
  move_lakitu(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x04u);
  assert(smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);

  /* bled to zero: the direction flips, and the speed comes from
   * the distance calc for the new (leftward) direction */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_LAKITU;
  smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x01u;
  move_lakitu(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] == 0x00u);
  /* capped $3c quartered to $0f: $15 - $0f - 1 = $05, leftward */
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xfbu);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);

  /* a left-moving lakitu left of the player flips immediately */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_LAKITU;
  smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u; /* side 1 */
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  move_lakitu(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_LAKITU_MOVE_DIRECTION + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x05u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
}


static void flying_cheep_cheep_moves(void) {
  /* undefeated: horizontal movement, the $0d/$05 gravity, the
   * wobble, and the residual priority store */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x18u; /* rightward */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0xa0u;
  move_flying_cheep_cheep(&smbc.state, 0x00u);
  /* force $0d after gravity: nybble 0, subtracter $f8; $a0-$f8
   * wraps negative, negated to $58 — no wobble */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x0du);
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 0] == 0x20u);
  /* moved one pixel right ($18 high nybble) */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x41u);

  /* within eight pixels of the subtracter: the force wobbles up */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0xa0u; /* == subtracter[1] */
  smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] = 0x10u;
  move_flying_cheep_cheep(&smbc.state, 0x00u);
  /* gravity: $10+$0d = $1d, nybble 1; $a0-$a0 = 0 < 8: wobble */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x2du);
  /* nybble of $2d is 2: priority $20 */
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 0] == 0x20u);

  /* high force nybble selects the zero priority */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] = 0x30u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x00u; /* $00-$bd wraps: $43 */
  move_flying_cheep_cheep(&smbc.state, 0x00u);
  /* gravity: $3d, nybble 3, subtracter $bd; $00-$bd = $43 >= 8 */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x3du);
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 0] == 0x00u);
}

static void defeated_flying_cheep_cheep(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 0] = 0x60u;
  move_flying_cheep_cheep(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x1cu);
}


static void movement_subs_dispatches(void) {
  /* the dummy ids do nothing at all */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x09u;
  enemy_movement_subs(&smbc.state, 0x00u);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x13u;
  enemy_movement_subs(&smbc.state, 0x00u);
  /* untouched apart from the id byte: compare a fresh baseline */
  Smbc baseline;
  smbc_init(&baseline);
  for (int i = 0; i < 0x800; i++) {
    if (i == SMBC_ENEMY_ID) continue; /* the id byte itself */
    assert(smbc.state.ram[i] == baseline.state.ram[i]);
  }

  /* a goomba ($06) walks: left by default under the master timer */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0xf0u; /* one pixel left */
  enemy_movement_subs(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x3fu);

  /* a hammer bro ($05) onscreen, mid jump timer, with an expired
   * throw timer re-arms and throws */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x05u;
  smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x00u;
  enemy_movement_subs(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_HAMMER_THROWING_TIMER + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_HAMMER_BRO_JUMP_TIMER + 0] == 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_STATE + 0] & 0x08u) != 0x00u);

  /* a podoboo ($0c) with an active interval timer moves with its
   * old speed under the $1c jumping gravity */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x0cu;
  smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0xc1u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0xfbu;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  enemy_movement_subs(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x1cu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] < 0xfbu);

  /* a piranha plant ($0d) in a state byte stays in its pipe */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = 0x0du;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x30u;
  enemy_movement_subs(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 0] == 0x20u);
}


static void run_normal_family(void) {
  /* a walking goomba: the full pipeline draws it and the timer
   * control being clear lets the movement sub run; the sprite
   * attributes start cleared */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0xa0u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0xf0u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x08u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x02u;
  run_normal_enemies(&smbc.state, 0x00u);
  /* the goomba frame was drawn */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 8u] == 0x70u);
  /* the movement sub walked it one pixel left */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x3fu);
  /* the bound box was computed: slot x + 1 */
  assert(smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER + 4] != 0x00u);

  /* with the master timer control set the movement sub is
   * skipped but everything else still runs */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_GOOMBA;
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0xa0u;
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0xf0u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x08u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x02u;
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x01u;
  run_normal_enemies(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 8u] == 0x70u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x40u);

  /* the retainer draws with no movement and no collisions */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 0] = SMBC_RETAINER_OBJECT;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0xa0u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] = 0x01u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x08u;
  run_retainer_obj(&smbc.state, 0x00u);
  /* retainer graphics at offset $a2: $cd $cd $ce $ce $cf $cf */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER] == 0xcdu);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 8u] == 0xceu);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x40u);
  /* the bound box stays untouched */
  assert(smbc.state.ram[SMBC_BOUNDING_BOX_UL_CORNER + 4] == 0x00u);
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
  bullet_bill_moves();
  cheep_cheep_drifts_left();
  cheep_cheep_oscillates();
  defeated_cheep_cheep_sinks();
  podoboo_reinit_and_rise();
  hammer_bro_jump_code();
  hammer_bro_throws();
  hammer_bro_shimmy_and_walk();
  piranha_stays_in_pipe();
  piranha_emerges_and_moves();
  lakitu_spawning_state();
  lakitu_chases_player();
  lakitu_turns_around();
  flying_cheep_cheep_moves();
  defeated_flying_cheep_cheep();
  movement_subs_dispatches();
  run_normal_family();
  puts("tests/core/runners: ok");
  return 0;
}
