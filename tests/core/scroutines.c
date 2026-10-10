/* tests/core/scroutines.c — exercises the ScreenRoutines task tree in
 * src/core/screen.c: the palette and colour subroutines, the status line
 * writers, the intermediate displays and the parser/title tasks, plus the
 * mode wiring through OperModeExecutionTree. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/mode.h"
#include "../../src/core/screen.h"
#include "../../src/core/state.h"
#include "../../src/data/chr.h"
#include "../../src/data/title.h"

#define BUF SMBC_VRAM_BUFFER1

static void player_colors_mario(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x02u; /* bg color from the area type */
  get_player_colors(&smbc);
  assert(smbc.state.ram[BUF] == 0x3fu);
  assert(smbc.state.ram[BUF + 1] == 0x10u);
  assert(smbc.state.ram[BUF + 2] == 0x04u);
  /* background color overwrites the first fetched player color */
  assert(smbc.state.ram[BUF + 3] == 0x0fu);
  assert(smbc.state.ram[BUF + 4] == 0x16u);
  assert(smbc.state.ram[BUF + 5] == 0x27u);
  assert(smbc.state.ram[BUF + 6] == 0x18u);
  assert(smbc.state.ram[BUF + 7] == 0x00u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x07u);
}

static void player_colors_fiery_luigi(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_CURRENT_PLAYER] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u; /* fiery wins over luigi */
  smbc.state.ram[SMBC_BACKGROUND_COLOR_CTRL] = 0x05u;
  smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] = 0x20u;
  get_player_colors(&smbc);
  assert(smbc.state.ram[BUF + 0x23] == 0x22u); /* bg from color ctrl 5 */
  assert(smbc.state.ram[BUF + 0x24] == 0x37u); /* fiery set */
  assert(smbc.state.ram[BUF + 0x25] == 0x27u);
  assert(smbc.state.ram[BUF + 0x26] == 0x16u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x27u);
}

static void init_screen_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  /* title mode: no vram address control load */
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x00u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x01u);
  assert(smbc.state.ram[SMBC_SPRITE_DATA] == 0xf8u); /* sprites offscreen */

  /* any other mode: buffer address control 3 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x03u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x01u);
}

static void setup_intermediate_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u;
  smbc.state.ram[SMBC_BACKGROUND_COLOR_CTRL] = 0x05u;
  screen_routines(&smbc);
  /* written with status 0 and color ctrl 2, then both restored */
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x02u);
  assert(smbc.state.ram[SMBC_BACKGROUND_COLOR_CTRL] == 0x05u);
  assert(smbc.state.ram[BUF + 3] == 0x0fu);  /* BackgroundColors[2] */
  assert(smbc.state.ram[BUF + 4] == 0x16u);  /* mario, not fiery */
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x02u);
}

static void top_status_line_task_advances(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x02u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x03u);
}

static void bottom_status_line_task(void) {
  /* the world-area block lands right after GetSBNybbles' writes: compute
   * that offset on a fresh instance rather than hardcoding the writer */
  Smbc probe;
  smbc_init(&probe);
  get_sb_nybbles(&probe.state);
  uint8_t o = probe.state.ram[SMBC_VRAM_BUFFER1_OFFSET];
  /* all-zero score: highest digit suppressed to the space tile */
  assert(probe.state.ram[BUF - 6 + o] == 0x24u);

  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x03u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x02u;
  screen_routines(&smbc);
  assert(smbc.state.ram[BUF + o] == 0x20u);
  assert(smbc.state.ram[BUF + o + 1] == 0x73u);
  assert(smbc.state.ram[BUF + o + 2] == 0x03u);
  assert(smbc.state.ram[BUF + o + 3] == 0x01u); /* world 1-3, one-based */
  assert(smbc.state.ram[BUF + o + 4] == 0x28u); /* the dash */
  assert(smbc.state.ram[BUF + o + 5] == 0x03u);
  assert(smbc.state.ram[BUF + o + 6] == 0x00u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == o + 6u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x04u);
}

static void time_up_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x04u;
  /* timer not expired: two tasks forward */
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x06u);

  /* expired: flag cleared, screen timer reset, screen reenabled */
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x04u;
  smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] = 0x01u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_SCREEN_TIMER] == 0x07u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x05u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x00u);
}

static void reset_sprites_and_timer_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x05u;
  /* screen timer running: nothing happens */
  smbc.state.ram[SMBC_SCREEN_TIMER] = 0x03u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_TIMER] == 0x03u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x05u);

  /* expired: sprites offscreen, timer reset, task forward */
  smbc.state.ram[SMBC_SCREEN_TIMER] = 0x00u;
  smbc.state.ram[SMBC_SPRITE_DATA] = 0x40u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SPRITE_DATA] == 0xf8u);
  assert(smbc.state.ram[SMBC_SCREEN_TIMER] == 0x07u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x06u);
}

static void intermediate_task(void) {
  /* title mode: skip to the parser control */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x06u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x08u);

  /* game mode with an alternate entrance: skip */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x06u;
  smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x02u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x08u);

  /* game mode, normal entrance: the lives display */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x06u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_TIMER] == 0x07u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x07u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x00u);

  /* game over mode: screen timer $12 and the mode task moves on */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x06u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_TIMER] == 0x12u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
}

static void parser_control_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x08u;
  smbc.state.ram[SMBC_COLUMN_SETS] = 0x01u;
  uint8_t disabled = smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG];
  /* more column sets to render: stay on the task, screen off, output */
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_COLUMN_SETS] == 0x00u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x08u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x06u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == disabled + 1u);

  /* last set rendered: move on */
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x09u);
}

static void palette_tasks(void) {
  Smbc smbc;
  smbc_init(&smbc);
  /* task 9: the area palette by area type */
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x09u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x02u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x03u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x0au);

  /* task 10 without a color control: no addr change, colors written */
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x77u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x77u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x0bu);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x07u);

  /* task 10 with color control 5: addr control 9 */
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x0au;
  smbc.state.ram[SMBC_BACKGROUND_COLOR_CTRL] = 0x05u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x09u);

  /* task 11: the mushroom style palette */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x0bu;
  smbc.state.ram[SMBC_AREA_STYLE] = 0x01u;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x0bu);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x0cu);
}

static void title_tasks(void) {
  /* task 12 outside title mode: the mode task moves on */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x0cu;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x0cu);

  /* task 12 in title mode: the CHR title data lands in the buffer page,
   * buffer control 5, task forward */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x0cu;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x05u);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x0du);
  assert(smbc.state.ram[BUF - 1] == SMBC_CHR_ROM[0x1ec0]);
  assert(smbc.state.ram[BUF - 1 + 0x100] == SMBC_CHR_ROM[0x1fc0]);
  assert(smbc.state.ram[BUF - 1 + 0x139] == SMBC_CHR_ROM[0x1ff9]);

  /* task 13: the buffer pages cleared, icon drawn, task forward */
  smbc.state.ram[BUF - 1] = 0xaau; /* the offset byte itself */
  smbc.state.ram[BUF + 0xff] = 0xbbu; /* $0400: the second page */
  screen_routines(&smbc);
  assert(smbc.state.ram[BUF + 0xff] == 0x00u);
  /* the mushroom icon overwrites the offset byte and the buffer start */
  assert(smbc.state.ram[BUF - 1] == SMBC_MUSHROOM_ICON_DATA[0]);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x0eu);

  /* task 13 outside title mode: the mode task moves on */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x0du;
  screen_routines(&smbc);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
}

static void top_score_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] = 0x0eu;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x01u;
  screen_routines(&smbc);
  /* $fa selects the top score display: $22f0, six digits */
  assert(smbc.state.ram[BUF] == 0x22u);
  assert(smbc.state.ram[BUF + 1] == 0xf0u);
  assert(smbc.state.ram[BUF + 2] == 0x06u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
}

static void mode_wiring(void) {
  /* game mode task 1 runs the screen task tree */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x01u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_ROUTINE_TASK] == 0x01u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x03u);
}

int main(void) {
  player_colors_mario();
  player_colors_fiery_luigi();
  init_screen_task();
  setup_intermediate_task();
  top_status_line_task_advances();
  bottom_status_line_task();
  time_up_task();
  reset_sprites_and_timer_task();
  intermediate_task();
  parser_control_task();
  palette_tasks();
  title_tasks();
  top_score_task();
  mode_wiring();
  printf("tests/core/scroutines: ok\n");
  return 0;
}
