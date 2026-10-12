/* tests/core/physics.c — exercises src/core/physics.c: the player
 * physics tables, the movement subs, the head-bump block routines and
 * the background collision tree. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/move.h"
#include "../../src/core/enemies.h"
#include "../../src/core/physics.h"
#include "../../src/core/sound.h"

/* park a metatile in block buffer 1 (page 0) */
static void put_mt(Smbc *smbc, uint8_t row, uint8_t col, uint8_t mt) {
  smbc->state.ram[SMBC_BLOCK_BUFFER_1 + row + col] = mt;
}

/* the basic in-play setup: subroutine 8, onscreen, big player */
static void in_play(Smbc *smbc) {
  smbc_init(smbc);
  smbc->state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc->state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc->state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc->state.ram[SMBC_PLAYER_X_POSITION] = 0x20u;
  smbc->state.ram[SMBC_PLAYER_Y_POSITION] = 0x60u;
}

static void climb_physics_speeds(void) {
  /* climbing up: negative speed, the animation timer halves */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x03u;
  smbc.state.ram[SMBC_UP_DOWN_BUTTONS] = 0x08u;
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0x08u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xffu);
  assert(smbc.state.ram[SMBC_PLAYER_Y_MOVE_FORCE] == 0x20u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_TIMER_SET] == 0x04u);

  /* climbing down: positive speed, full animation timer */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x03u;
  smbc.state.ram[SMBC_UP_DOWN_BUTTONS] = 0x04u;
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0x04u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_MOVE_FORCE] == 0xffu);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_TIMER_SET] == 0x08u);

  /* no climbing input: nullified, and no X_Physics run */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x03u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_TIMER_SET] == 0x08u);
  assert(smbc.state.ram[SMBC_FRICTION_ADDER_LOW] == 0x00u);
}

static void fresh_jump_brackets(void) {
  /* grounded small player, new A press, top speed bracket */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_A_BUTTON;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_XSPEED_ABSOLUTE] = 0x1cu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x80u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x01u);
  assert(smbc.state.ram[SMBC_JUMP_SWIM_TIMER] == 0x20u);
  assert(smbc.state.ram[SMBC_JUMP_ORIGIN_Y_POSITION] == 0x80u);
  assert(smbc.state.ram[SMBC_VERTICAL_FORCE] == 0x28u);
  assert(smbc.state.ram[SMBC_VERTICAL_FORCE_DOWN] == 0x90u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfbu);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == SMBC_SFX_SMALL_JUMP);

  /* the lowest bracket takes the first table entries */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_A_BUTTON;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_VERTICAL_FORCE] == 0x20u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfcu);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == SMBC_SFX_BIG_JUMP);

  /* a held A press does not re-jump */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_A_BUTTON;
  smbc.state.ram[SMBC_PREVIOUS_A_B_BUTTONS] = SMBC_A_BUTTON;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x00u);
  assert(smbc.state.ram[SMBC_JUMP_SWIM_TIMER] == 0x00u);
}

static void swim_jump_entries(void) {
  /* swimming: the swim bracket and the stomp sound */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x02u;
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_A_BUTTON;
  smbc.state.ram[SMBC_SWIMMING_FLAG] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfeu; /* still rising */
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x01u);
  assert(smbc.state.ram[SMBC_VERTICAL_FORCE] == 0x0du);
  assert(smbc.state.ram[SMBC_VERTICAL_FORCE_DOWN] == 0x0au);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfeu);
  assert(smbc.state.ram[SMBC_PLAYER_Y_MOVE_FORCE] == 0x80u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == SMBC_SFX_ENEMY_STOMP);
}

static void x_physics_picks(void) {
  /* grounded, not water, no direction match: the slow brackets */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_MAXIMUM_LEFT_SPEED] == 0xf0u);
  assert(smbc.state.ram[SMBC_MAXIMUM_RIGHT_SPEED] == 0x10u);
  assert(smbc.state.ram[SMBC_FRICTION_ADDER_LOW] == 0x98u);

  /* B held in a non-water area with direction match: the run timer
   * is set and the fastest brackets apply */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_B_BUTTON;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_RUNNING_TIMER] == 0x0au);
  assert(smbc.state.ram[SMBC_MAXIMUM_LEFT_SPEED] == 0xd8u);
  assert(smbc.state.ram[SMBC_MAXIMUM_RIGHT_SPEED] == 0x28u);
  assert(smbc.state.ram[SMBC_FRICTION_ADDER_LOW] == 0xe4u);

  /* a skidding player gets the doubled friction adder */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x02u;
  player_physics_sub(&smbc.state);
  assert(smbc.state.ram[SMBC_FRICTION_ADDER_LOW] == 0x30u);
  assert(smbc.state.ram[SMBC_FRICTION_ADDER_HIGH] == 0x01u);
}

static void skid_snaps_direction(void) {
  /* pressing against the facing direction below the skid threshold
   * snaps the moving direction and nullifies the speed */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x01u;
  smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_XSPEED_ABSOLUTE] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0xfbu;
  player_movement_subs(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_MOVING_DIR] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_XSPEED_ABSOLUTE] == 0x00u);
}

static void run_speed_bracket(void) {
  /* fast enough: the running speed stays and the tightest animation
   * timing applies */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_XSPEED_ABSOLUTE] = 0x20u;
  player_movement_subs(&smbc.state);
  assert(smbc.state.ram[SMBC_RUNNING_SPEED] == 0x20u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_TIMER_SET] == 0x02u);
}

static void friction_clamps(void) {
  /* pressing right into the maximum speed clamps at the cap */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0xffu;
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x01u;
  smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_XSPEED_ABSOLUTE] = 0x10u;
  player_movement_subs(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x10u);

  /* pressing left against rightward speed decelerates */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0xffu;
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x02u;
  smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_XSPEED_ABSOLUTE] = 0x10u;
  player_movement_subs(&smbc.state);
  assert((int8_t)smbc.state.ram[SMBC_PLAYER_X_SPEED] < 0x10);
}

static void crouch_flag_rules(void) {
  /* a small player never crouches */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_UP_DOWN_BUTTONS] = 0x04u;
  player_movement_subs(&smbc.state);
  assert(smbc.state.ram[SMBC_CROUCHING_FLAG] == 0x00u);

  /* a big grounded player crouches on down */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_UP_DOWN_BUTTONS] = 0x04u;
  player_movement_subs(&smbc.state);
  assert(smbc.state.ram[SMBC_CROUCHING_FLAG] == 0x04u);
}

static void collision_gates(void) {
  /* the disable flag short-circuits everything */
  Smbc smbc;
  in_play(&smbc);
  smbc.state.ram[SMBC_DISABLE_COLLISION_DET] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0x00u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] == 0x00u);

  /* early subroutines skip the check */
  in_play(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x03u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] == 0x00u);

  /* offscreen vertically: no state changes */
  in_play(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x02u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x02u);
}

static void default_states(void) {
  /* a normal player defaults to falling, a swimming one to jumping */
  Smbc smbc;
  in_play(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xd0u; /* too low: exit */
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x02u);
  assert(smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] == 0xffu);

  in_play(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xd0u;
  smbc.state.ram[SMBC_SWIMMING_FLAG] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x01u);
}

static void landing_on_solid(void) {
  /* big player over a solid metatile lands, snaps and resets */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x40u, 0x02u, 0xc4u);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x02u; /* falling */
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_Y_MOVE_FORCE] = 0x55u;
  smbc.state.ram[SMBC_STOMP_CHAIN_COUNTER] = 0x03u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x42u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_POSITION] == 0x40u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_MOVE_FORCE] == 0x00u);
  assert(smbc.state.ram[SMBC_STOMP_CHAIN_COUNTER] == 0x00u);
}

static void foot_impede(void) {
  /* a partial landing (low nybble 5+) impedes the movement instead */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x60u, 0x02u, 0xc4u);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x67u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_SIDE_COLLISION_TIMER] == 0x10u);
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x1fu);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x00u); /* 0xff + carry */
  assert(smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] == 0xfeu);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x02u);
}

static void head_bump_solid(void) {
  /* rising into a solid metatile: bump sound, vertical speed
   * nullified to one */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x40u, 0x02u, 0x24u);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x64u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == SMBC_SFX_BUMP);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0x01u);
}

static void head_bump_hidden_block(void) {
  /* rising into a hidden coin block: unbreakable block object, the
   * coin dispensed, the buffer cell blanked */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x40u, 0x02u, 0x5fu);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x64u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x11u);
  assert(smbc.state.ram[SMBC_BLOCK_METATILE + 0] == 0xc4u);
  assert(smbc.state.ram[SMBC_BLOCK_BOUNCE_TIMER] == 0x10u);
  assert(smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] == 0x60u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0x42u] == 0x23u);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x01u);
  assert(smbc.state.ram[SMBC_MISC_Y_SPEED + 8] == 0xfbu);
  assert(smbc.state.ram[SMBC_COIN_TALLY] == 0x01u);
  assert(smbc.state.ram[SMBC_COIN_TALLY_FOR_1UPS] == 0x01u);
  assert(smbc.state.ram[SMBC_SPDATA_OFFSET_CTRL] == 0x01u);
}

static void head_bump_breaks_brick(void) {
  /* a big player breaking a brick: shatter state, chunk speeds, the
   * player's bounce */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x40u, 0x02u, 0x51u);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x64u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x12u);
  assert(smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] == SMBC_SFX_BRICK_SHATTER);
  assert(smbc.state.ram[SMBC_NOISE_SOUND_QUEUE] == SMBC_SFX_BRICK_SHATTER);
  assert(smbc.state.ram[SMBC_BLOCK_X_SPEED + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_BLOCK_X_SPEED + 2] == 0xf0u);
  assert(smbc.state.ram[SMBC_BLOCK_Y_SPEED + 2] == 0xfcu);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0xfeu);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0x42u] == 0x23u);
}

static void small_player_bumps_brick(void) {
  /* a small player only bounces the brick: unbreakable state */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x50u, 0x02u, 0x51u);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x64u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x11u);
  assert(smbc.state.ram[SMBC_BLOCK_METATILE + 0] == 0x51u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_SPEED] == 0x00u);
}

static void question_block_gives_power_up(void) {
  /* a mushroom block puts the power-up object in the last enemy slot
   * behind the scenery */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x40u, 0x02u, 0xc1u);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x64u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 5] == SMBC_POWER_UP_OBJECT);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 5] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 5] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] == 0x58u);
  assert(smbc.state.ram[SMBC_POWER_UP_TYPE] == 0x00u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] ==
         SMBC_SFX_GROW_POWER_UP);
}

static void vine_block_grows_vine(void) {
  /* the vine block hands off to setup_vine with the block offset */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x40u, 0x02u, 0x56u);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x64u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 5] == SMBC_VINE_OBJECT);
}

static void coin_on_top_collected(void) {
  /* a coin sitting on a bumped block is collected with it */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x40u, 0x02u, 0x55u);
  put_mt(&smbc, 0x30u, 0x02u, 0xc2u); /* one row up */
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x64u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0x32u] == 0x00u);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x01u);
  assert(smbc.state.ram[SMBC_COIN_TALLY] == 0x01u);
}

static void side_touch_climbable(void) {
  /* walking into a vine puts the player on it */
  Smbc smbc;
  in_play(&smbc);
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x29u;
  put_mt(&smbc, 0x50u, 0x03u, 0x26u);
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x03u);
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_X_MOVE_FORCE] == 0x00u);
}

static void side_solid_stops_player(void) {
  /* walking into a wall impedes the movement */
  Smbc smbc;
  in_play(&smbc);
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x29u;
  put_mt(&smbc, 0x50u, 0x03u, 0xc4u);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_SIDE_COLLISION_TIMER] == 0x10u);
}

static void sideways_pipe_entry(void) {
  /* grounded, facing right, against a sideways pipe bottom: the entry
   * timer and the priority bit, the routine switch only from 8 */
  Smbc smbc;
  in_play(&smbc);
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x22u;
  put_mt(&smbc, 0x60u, 0x02u, 0xc4u); /* ground */
  put_mt(&smbc, 0x50u, 0x02u, 0x6cu); /* pipe bottom, second side */
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x02u; /* falls to ground... */
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] & 0x20u);
  assert(smbc.state.ram[SMBC_CHANGE_AREA_TIMER] == 0xa0u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x02u);
}

static void vertical_pipe_entry(void) {
  /* handle_pipe_entry: down on a warp pipe sets the area change */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_UP_DOWN_BUTTONS] = 0x04u;
  handle_pipe_entry(&smbc.state, 0x11u, 0x10u);
  assert(smbc.state.ram[SMBC_CHANGE_AREA_TIMER] == 0x30u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x03u);
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0x20u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] ==
         SMBC_SFX_PIPE_DOWN_INJURY);

  /* the wrong metatiles do nothing */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_UP_DOWN_BUTTONS] = 0x04u;
  handle_pipe_entry(&smbc.state, 0x11u, 0x11u);
  assert(smbc.state.ram[SMBC_CHANGE_AREA_TIMER] == 0x00u);
}

static void hundred_coins_extra_life(void) {
  /* the hundredth coin rolls the tally over and earns a life */
  Smbc smbc;
  in_play(&smbc);
  put_mt(&smbc, 0x60u, 0x02u, 0xc2u); /* coin at the feet */
  smbc.state.ram[SMBC_COIN_TALLY] = 99u;
  smbc.state.ram[SMBC_NUMBEROF_LIVES] = 0x03u;
  player_bg_collision(&smbc.state);
  assert(smbc.state.ram[SMBC_COIN_TALLY] == 0x00u);
  assert(smbc.state.ram[SMBC_NUMBEROF_LIVES] == 0x04u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == SMBC_SFX_EXTRA_LIFE);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0x62u] == 0x00u);
}

int main(void) {
  climb_physics_speeds();
  fresh_jump_brackets();
  swim_jump_entries();
  x_physics_picks();
  skid_snaps_direction();
  run_speed_bracket();
  friction_clamps();
  crouch_flag_rules();
  collision_gates();
  default_states();
  landing_on_solid();
  foot_impede();
  head_bump_solid();
  head_bump_hidden_block();
  head_bump_breaks_brick();
  small_player_bumps_brick();
  question_block_gives_power_up();
  vine_block_grows_vine();
  coin_on_top_collected();
  side_touch_climbable();
  side_solid_stops_player();
  sideways_pipe_entry();
  vertical_pipe_entry();
  hundred_coins_extra_life();
  puts("physics: ok");
  return 0;
}
