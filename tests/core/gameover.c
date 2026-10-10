/* tests/core/gameover.c — exercises src/core/gameover.c and area.c: the
 * GameOverMode task tree, TransposePlayers and the area pointer lookup.
 * Table expectations are the smb.asm values at data/area. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/area.h"
#include "../../src/core/frame.h"
#include "../../src/core/gameover.h"
#include "../../src/core/mode.h"
#include "../../src/core/state.h"
#include "../../src/data/music.h"

static void area_pointer_lookup(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* world 1 area 1: pointer $25, type %01 (underground) */
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x00u;
  assert(find_area_pointer(&smbc.state) == 0x25u);
  load_area_pointer(&smbc);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x25u);
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x01u);

  /* world 1 area 3 (index 2): the $c0 bonus pointer, type %10 */
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x02u;
  assert(find_area_pointer(&smbc.state) == 0xc0u);
  load_area_pointer(&smbc);
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x02u);

  /* world 2 area 3 (index 2): water pointer $01, type %00 */
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x01u;
  assert(find_area_pointer(&smbc.state) == 0x01u);
  load_area_pointer(&smbc);
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x00u);

  /* world 8 last entry (index 3): castle pointer $65, type %11 */
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x07u;
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x03u;
  assert(find_area_pointer(&smbc.state) == 0x65u);
  load_area_pointer(&smbc);
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x03u);
}

static void setup_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x55u;
  smbc.state.ram[SMBC_SPRITE0_HIT_FLAG] = 0x01u;
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x07u;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;

  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE0_HIT_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_EVENT_MUSIC_GAME_OVER);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x08u); /* inc */
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x01u);
}

static void run_task_waits_while_timer_counts(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  smbc.state.ram[SMBC_SCREEN_TIMER] = 0x10u;
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x01u;

  /* no start pressed, timer still counting: GameIsOn, nothing changes */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_GAME_OVER);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
}

static void run_task_terminates_one_player(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  smbc.state.ram[SMBC_SCREEN_TIMER] = 0x10u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_START;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x04u;

  /* start pressed in a 1-player game: TerminateGame ends at the title */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  assert(smbc.state.ram[SMBC_CONTINUE_WORLD] == 0x04u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_TITLE);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
  assert(smbc.state.ram[SMBC_SCREEN_TIMER] == 0x00u);
}

static void run_task_terminates_when_timer_expires(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  /* no start, timer at zero: same terminate path */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_TITLE);
}

static void run_task_continues_two_player(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_START;
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x30u;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x09u;
  smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] = 0x01u;
  /* WorldNumber and AreaNumber are bytes 5 and 6 of OnscreenPlayerInfo, so
   * the transpose moves them; both players continue at world 3 area 2 */
  for (int x = 0; x < 5; x++) {
    smbc.state.ram[SMBC_ONSCREEN_PLAYER_INFO + x] = (uint8_t)(0x10 + x);
    smbc.state.ram[SMBC_OFFSCREEN_PLAYER_INFO + x] = (uint8_t)(0x20 + x);
  }
  smbc.state.ram[SMBC_ONSCREEN_PLAYER_INFO + 5] = 0x02u;
  smbc.state.ram[SMBC_ONSCREEN_PLAYER_INFO + 6] = 0x01u;
  smbc.state.ram[SMBC_OFFSCREEN_PLAYER_INFO + 5] = 0x02u;
  smbc.state.ram[SMBC_OFFSCREEN_PLAYER_INFO + 6] = 0x01u;

  /* offscreen player has lives ($20 > 0): players transpose, game resumes */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  assert(smbc.state.ram[SMBC_CURRENT_PLAYER] == 0x01u);
  for (int x = 0; x < 5; x++) {
    assert(smbc.state.ram[SMBC_ONSCREEN_PLAYER_INFO + x] == 0x20 + x);
    assert(smbc.state.ram[SMBC_OFFSCREEN_PLAYER_INFO + x] == 0x10 + x);
  }
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x35u); /* world 3, index 1 */
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_SIZE] == 0x01u);
  assert(smbc.state.ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_GAME);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
}

static void run_task_terminates_when_other_player_is_out(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_START;
  smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] = 0x01u;
  smbc.state.ram[SMBC_OFFSCR_NUMBEROF_LIVES] = 0xffu; /* bmi — out of lives */
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x06u;

  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_CURRENT_PLAYER] == 0x00u); /* no transpose */
  assert(smbc.state.ram[SMBC_CONTINUE_WORLD] == 0x06u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_TITLE);
}

int main(void) {
  area_pointer_lookup();
  setup_task();
  run_task_waits_while_timer_counts();
  run_task_terminates_one_player();
  run_task_terminates_when_timer_expires();
  run_task_continues_two_player();
  run_task_terminates_when_other_player_is_out();
  printf("tests/core/gameover: ok\n");
  return 0;
}
