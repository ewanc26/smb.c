/* tests/core/gametext.c — exercises WriteGameText in src/core/screen.c:
 * the message copies, the lives/world/level patches, the player name
 * swap and the warp zone numbers, plus the un-stubbed task paths. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/screen.h"
#include "../../src/core/state.h"

#define BUF SMBC_VRAM_BUFFER1

static void top_status_bar(void) {
  Smbc smbc;
  smbc_init(&smbc);
  write_game_text(&smbc.state, 0x00u);
  assert(smbc.state.ram[BUF] == 0x20u);
  assert(smbc.state.ram[BUF + 1] == 0x43u);
  assert(smbc.state.ram[BUF + 3] == 0x16u); /* M of MARIO */
  assert(smbc.state.ram[BUF + 37] == 0xeau); /* last attribute byte */
  assert(smbc.state.ram[BUF + 38] == 0x00u); /* terminator */
  /* the one-player path leaves the buffer offset alone */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x00u);
}

static void lives_display(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBEROF_LIVES] = 0x02u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x03u;
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x01u;
  write_game_text(&smbc.state, 0x01u);
  assert(smbc.state.ram[BUF] == 0x21u);
  assert(smbc.state.ram[BUF + 1] == 0xcdu);
  assert(smbc.state.ram[BUF + 8] == 0x03u);  /* lives, one-based */
  assert(smbc.state.ram[BUF + 19] == 0x04u); /* world 4-2 */
  assert(smbc.state.ram[BUF + 21] == 0x02u);

  /* more than nine lives: a crown tile next to the difference */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBEROF_LIVES] = 0x0cu; /* 13 displayed */
  write_game_text(&smbc.state, 0x01u);
  assert(smbc.state.ram[BUF + 7] == 0x9fu); /* the crown */
  assert(smbc.state.ram[BUF + 8] == 0x03u);
}

static void time_up_names(void) {
  /* one player: no name row at all */
  Smbc smbc;
  smbc_init(&smbc);
  write_game_text(&smbc.state, 0x02u);
  assert(smbc.state.ram[BUF] == 0x22u); /* straight to TIME UP */
  assert(smbc.state.ram[BUF + 3] == 0x1du); /* T */

  /* two players, mario up: the name row names the OTHER player — luigi */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] = 0x01u;
  smbc.state.ram[SMBC_CURRENT_PLAYER] = 0x00u;
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  write_game_text(&smbc.state, 0x02u);
  assert(smbc.state.ram[BUF] == 0x21u);
  assert(smbc.state.ram[BUF + 3] == 0x15u); /* L */
  assert(smbc.state.ram[BUF + 4] == 0x1eu); /* U */
  assert(smbc.state.ram[BUF + 7] == 0x12u); /* I */

  /* two players, luigi up: mario keeps his name */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] = 0x01u;
  smbc.state.ram[SMBC_CURRENT_PLAYER] = 0x01u;
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  write_game_text(&smbc.state, 0x02u);
  assert(smbc.state.ram[BUF + 3] == 0x16u); /* M */
}

static void game_over_names(void) {
  /* two players, luigi's game over: the CURRENT player is named */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] = 0x01u;
  smbc.state.ram[SMBC_CURRENT_PLAYER] = 0x01u;
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  write_game_text(&smbc.state, 0x03u);
  assert(smbc.state.ram[BUF] == 0x21u);
  assert(smbc.state.ram[BUF + 3] == 0x15u); /* L of LUIGI */
  assert(smbc.state.ram[BUF + 11] == 0x10u); /* G of GAME OVER */

  /* mario's game over: the name stays */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] = 0x01u;
  smbc.state.ram[SMBC_CURRENT_PLAYER] = 0x00u;
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  write_game_text(&smbc.state, 0x03u);
  assert(smbc.state.ram[BUF + 3] == 0x16u); /* M */
}

static void warp_zone(void) {
  Smbc smbc;
  smbc_init(&smbc);
  write_game_text(&smbc.state, 0x04u);
  assert(smbc.state.ram[BUF] == 0x25u);
  assert(smbc.state.ram[BUF + 3] == 0x20u);  /* W of WELCOME */
  assert(smbc.state.ram[BUF + 27] == 0x04u); /* the 4-3-2 zone */
  assert(smbc.state.ram[BUF + 31] == 0x03u);
  assert(smbc.state.ram[BUF + 35] == 0x02u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x2cu);

  /* the middle zone: spaces around the 5, partly responsible for the
   * minus world */
  smbc_init(&smbc);
  write_game_text(&smbc.state, 0x05u);
  assert(smbc.state.ram[BUF + 27] == 0x24u);
  assert(smbc.state.ram[BUF + 31] == 0x05u);
  assert(smbc.state.ram[BUF + 35] == 0x24u);
}

static void task_paths(void) {
  /* task 2 writes the top status bar */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x02u;
  screen_routines(&smbc);
  assert(smbc.state.ram[BUF] == 0x20u);
  assert(smbc.state.ram[BUF + 3] == 0x16u);

  /* task 4 with the timer expired writes the time-up screen */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x04u;
  smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] = 0x01u;
  screen_routines(&smbc);
  assert(smbc.state.ram[BUF] == 0x22u);

  /* task 6 in game over mode writes the game over screen */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x06u;
  screen_routines(&smbc);
  assert(smbc.state.ram[BUF] == 0x22u);
  assert(smbc.state.ram[BUF + 3] == 0x10u); /* G of GAME OVER */

  /* task 6 in game mode writes the lives display */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x06u;
  smbc.state.ram[SMBC_NUMBEROF_LIVES] = 0x02u;
  screen_routines(&smbc);
  assert(smbc.state.ram[BUF] == 0x21u);
  assert(smbc.state.ram[BUF + 8] == 0x03u);
}

int main(void) {
  top_status_bar();
  lives_display();
  time_up_names();
  game_over_names();
  warp_zone();
  task_paths();
  printf("tests/core/gametext: ok\n");
  return 0;
}
