/* tests/core/timer.c — exercises src/core/timer.c: RunGameTimer's gate
 * conditions, the once-per-control-period countdown, the 100-unit
 * "time running out" music trigger and the time-up death path. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/state.h"
#include "../../src/core/timer.h"
#include "../../src/data/music.h"

/* the state a live countdown needs: game mode, a running engine
 * subroutine, the player onscreen, and an expired control timer */
static void armed(Smbc *smbc) {
  smbc_init(smbc);
  smbc->state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc->state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc->state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc->state.ram[SMBC_GAME_TIMER_CTRL_TIMER] = 0x00u;
}

static void set_timer(Smbc *smbc, uint8_t hundreds, uint8_t tens,
                      uint8_t ones) {
  smbc->state.ram[SMBC_GAME_TIMER_DISPLAY] = hundreds;
  smbc->state.ram[SMBC_GAME_TIMER_DISPLAY + 1u] = tens;
  smbc->state.ram[SMBC_GAME_TIMER_DISPLAY + 2u] = ones;
}

static void title_screen_leaves(void) {
  Smbc smbc;
  armed(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = 0x00u; /* title screen */
  set_timer(&smbc, 0x01u, 0x00u, 0x00u);
  run_game_timer(&smbc.state);
  assert(smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] == 0x00u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x00u);
}

static void low_engine_routine_leaves(void) {
  Smbc smbc;
  armed(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x07u; /* below eight */
  set_timer(&smbc, 0x01u, 0x00u, 0x00u);
  run_game_timer(&smbc.state);
  assert(smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] == 0x00u);
}

static void death_routine_leaves(void) {
  Smbc smbc;
  armed(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x0bu; /* death */
  set_timer(&smbc, 0x01u, 0x00u, 0x00u);
  run_game_timer(&smbc.state);
  assert(smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] == 0x00u);
}

static void player_below_screen_leaves(void) {
  Smbc smbc;
  armed(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x02u; /* below the screen */
  set_timer(&smbc, 0x01u, 0x00u, 0x00u);
  run_game_timer(&smbc.state);
  assert(smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] == 0x00u);
}

static void ctrl_timer_not_expired_leaves(void) {
  Smbc smbc;
  armed(&smbc);
  smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] = 0x05u;
  set_timer(&smbc, 0x01u, 0x00u, 0x00u);
  run_game_timer(&smbc.state);
  assert(smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] == 0x05u);
}

static void countdown_decrements_and_arms_ctrl(void) {
  Smbc smbc;
  armed(&smbc);
  set_timer(&smbc, 0x03u, 0x00u, 0x00u); /* 300 */
  run_game_timer(&smbc.state);
  /* control reset, timer decremented to 299 through the digit maths */
  assert(smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] == 0x18u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY] == 0x02u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY + 1u] == 0x09u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY + 2u] == 0x09u);
  /* the digit modifier is erased once consumed */
  assert(smbc.state.ram[SMBC_DIGIT_MODIFIER + 5u] == 0x00u);
  /* first digit is not 1: no warning music */
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x00u);
}

static void countdown_plays_music_at_one_hundred(void) {
  Smbc smbc;
  armed(&smbc);
  set_timer(&smbc, 0x01u, 0x00u, 0x00u); /* 100 */
  run_game_timer(&smbc.state);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] ==
         SMBC_EVENT_MUSIC_TIME_RUNNING_OUT);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY + 1u] == 0x09u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY + 2u] == 0x09u);
}

static void no_music_when_not_at_one_hundred(void) {
  Smbc smbc;
  armed(&smbc);
  set_timer(&smbc, 0x01u, 0x06u, 0x00u); /* 160, first digit 1 but not 100 */
  run_game_timer(&smbc.state);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY + 1u] == 0x05u);
}

static void time_up_forces_death(void) {
  Smbc smbc;
  armed(&smbc);
  set_timer(&smbc, 0x00u, 0x00u, 0x00u); /* 000 */
  smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] = 0x00u;
  run_game_timer(&smbc.state);
  /* PlayerStatus was nulled, so ForceInjury takes the KillPlayer path */
  assert(smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x01u); /* death music */
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0bu);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0xffu);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x01u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_CTRL_TIMER] == 0x00u);
}

int main(void) {
  title_screen_leaves();
  low_engine_routine_leaves();
  death_routine_leaves();
  player_below_screen_leaves();
  ctrl_timer_not_expired_leaves();
  countdown_decrements_and_arms_ctrl();
  countdown_plays_music_at_one_hundred();
  no_music_when_not_at_one_hundred();
  time_up_forces_death();
  puts("tests/core/timer: ok");
  return 0;
}
