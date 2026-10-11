/* tests/core/gamemode.c — exercises src/core/gamemode.c: the GameMode
 * task tree, GameCoreRoutine's master joypad copy and task gate, the
 * GameEngine's music check and SaveAB tail, and UpdScrollVar. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/gamemode.h"
#include "../../src/core/mode.h"
#include "../../src/core/state.h"

static void master_joypad_copy(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_CURRENT_PLAYER] = 0x01u;
  smbc.state.ram[SMBC_SAVED_JOYPAD2_BITS] = 0x83u;

  /* luigi's controller bits become the master controller bits */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x83u);
}

static void setup_task_does_not_run_core(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  smbc.state.ram[SMBC_SAVED_JOYPAD2_BITS] = 0x83u;
  smbc.state.ram[SMBC_VRAM_BUFFER1 + 0x10] = 0xaa; /* ClearVRLoop target */

  oper_mode_execution_tree(&smbc);
  /* SecondaryGameSetup ran: buffer cleared, task advanced */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 0x10] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x03u);
  /* GameCoreRoutine did not: no joypad copy this frame */
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
}

static void engine_save_ab_tail(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x10u; /* onscreen: bpl taken */
  smbc.state.ram[SMBC_A_B_BUTTONS] = 0xc0u;
  smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x03u;
  smbc.state.ram[SMBC_STAR_INVINCIBLE_TIMER] = 0x20u;

  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_PREV_A_B_BUTTONS] == 0xc0u);
  assert(smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] == 0x00u);
  /* onscreen player: no music re-attain */
  assert(smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] == 0x00u);
}

static void engine_reattains_music_when_star_fades(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x00u; /* below screen */
  smbc.state.ram[SMBC_STAR_INVINCIBLE_TIMER] = 0x04u;
  smbc.state.ram[SMBC_INTERVAL_TIMER_CONTROL] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* underground music */

  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] != 0x00u);

  /* the interval timer gates the re-attain */
  Smbc gated;
  smbc_init(&gated);
  gated.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  gated.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  gated.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x00u;
  gated.state.ram[SMBC_STAR_INVINCIBLE_TIMER] = 0x04u;
  gated.state.ram[SMBC_INTERVAL_TIMER_CONTROL] = 0x07u;
  oper_mode_execution_tree(&gated);
  assert(gated.state.ram[SMBC_AREA_MUSIC_QUEUE] == 0x00u);

  /* no star at all: palette cleared, palette cycle skipped, no music */
  Smbc nostar;
  smbc_init(&nostar);
  nostar.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  nostar.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  nostar.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x00u;
  nostar.state.ram[SMBC_A_B_BUTTONS] = 0x40u;
  oper_mode_execution_tree(&nostar);
  assert(nostar.state.ram[SMBC_AREA_MUSIC_QUEUE] == 0x00u);
  assert(nostar.state.ram[SMBC_PREV_A_B_BUTTONS] == 0x40u);
}

static void scroll_var_update(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* vram address controller at 6: leave everything alone */
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x06u;
  smbc.state.ram[SMBC_SCROLL_THIRTY_TWO] = 0x21u;
  upd_scroll_var(&smbc);
  assert(smbc.state.ram[SMBC_SCROLL_THIRTY_TWO] == 0x21u);

  /* scroll past the 32-column range with no parser tasks: wrap and
   * reset the level graphics buffer offset */
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x00u;
  smbc.state.ram[SMBC_VRAM_BUFFER2_OFFSET] = 0x12u;
  upd_scroll_var(&smbc);
  assert(smbc.state.ram[SMBC_SCROLL_THIRTY_TWO] == 0x01u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2_OFFSET] == 0x00u);

  /* inside the range: untouched — the parser ran one task with the
   * last call, so reset its counter to reach the scroll-range path */
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x00u;
  smbc.state.ram[SMBC_SCROLL_THIRTY_TWO] = 0x1fu;
  smbc.state.ram[SMBC_VRAM_BUFFER2_OFFSET] = 0x12u;
  upd_scroll_var(&smbc);
  assert(smbc.state.ram[SMBC_SCROLL_THIRTY_TWO] == 0x1fu);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2_OFFSET] == 0x12u);
}

int main(void) {
  master_joypad_copy();
  setup_task_does_not_run_core();
  engine_save_ab_tail();
  engine_reattains_music_when_star_fades();
  scroll_var_update();
  printf("tests/core/gamemode: ok\n");
  return 0;
}
