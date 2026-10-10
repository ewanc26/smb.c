/* tests/core/title.c — exercises src/core/title.c: the mushroom icon,
 * DemoEngine, and GameMenuRoutine's start/select/B/demo paths. Table
 * expectations are the smb.asm values at data/title. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/mode.h"
#include "../../src/core/state.h"
#include "../../src/core/title.h"

static void mushroom_icon(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* 1-player: eight template bytes at VRAM_Buffer1-1 ($0300), no edit */
  draw_mushroom_icon(&smbc);
  const uint8_t want[8] = {0x07, 0x22, 0x49, 0x83, 0xce, 0x24, 0x24, 0x00};
  for (int y = 0; y < 8; y++) {
    assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET + y] == want[y]);
  }

  /* 2-player: blank tile at the 1-player position, icon at the 2-player */
  smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] = 0x01u;
  draw_mushroom_icon(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x24u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 5] == 0xceu);
}

static void demo_engine_cycle(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* fresh demo: action advances, timing loads, joypad bits perform */
  assert(!demo_engine(&smbc));
  assert(smbc.state.ram[SMBC_DEMO_ACTION] == 0x01u);
  assert(smbc.state.ram[SMBC_DEMO_ACTION_TIMER] == 0x9au); /* $9b then dec */
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x01u);

  /* while the timer counts the current action repeats */
  smbc.state.ram[SMBC_DEMO_ACTION_TIMER] = 0x05u;
  assert(!demo_engine(&smbc));
  assert(smbc.state.ram[SMBC_DEMO_ACTION] == 0x01u);
  assert(smbc.state.ram[SMBC_DEMO_ACTION_TIMER] == 0x04u);

  /* the zero timing entry ends the demo with carry set */
  smbc.state.ram[SMBC_DEMO_ACTION] = 0x15u; /* action 21 of 21 */
  smbc.state.ram[SMBC_DEMO_ACTION_TIMER] = 0x00u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x55u;
  assert(demo_engine(&smbc));
  assert(smbc.state.ram[SMBC_DEMO_ACTION] == 0x16u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x55u); /* no DoAction */
}

static void menu_start_game(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_START;
  smbc.state.ram[SMBC_DEMO_TIMER] = 0x18u;
  for (int x = 0; x < 0x18; x++) {
    smbc.state.ram[SMBC_SCORE_AND_COIN_DISPLAY + x] = 0xaau;
  }

  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x25u); /* world 1 area 1 */
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x01u);
  assert(smbc.state.ram[SMBC_HIDDEN_1UP_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_OFFSCR_HIDDEN_1UP_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_GAME);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
  assert(smbc.state.ram[SMBC_DEMO_TIMER] == 0x00u);
  assert(smbc.state.ram[SMBC_PRIMARY_HARD_MODE] == 0x00u);
  for (int x = 0; x < 0x18; x++) {
    assert(smbc.state.ram[SMBC_SCORE_AND_COIN_DISPLAY + x] == 0x00u);
  }
  /* the start path never nulls the joypad bits */
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == SMBC_PAD_START);
}

static void menu_continue_with_a_start(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_A | SMBC_PAD_START;
  smbc.state.ram[SMBC_DEMO_TIMER] = 0x18u;
  smbc.state.ram[SMBC_CONTINUE_WORLD] = 0x05u;

  /* A + start continues at the saved world, first area */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_WORLD_NUMBER] == 0x05u);
  assert(smbc.state.ram[SMBC_OFFSCR_WORLD_NUMBER] == 0x05u);
  assert(smbc.state.ram[SMBC_AREA_NUMBER] == 0x00u);
  assert(smbc.state.ram[SMBC_OFFSCR_AREA_NUMBER] == 0x00u);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x2eu); /* world 6 first area */
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_GAME);
}

static void menu_select_toggles_players(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_SELECT;
  smbc.state.ram[SMBC_DEMO_TIMER] = 0x18u;

  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] == 0x01u);
  assert(smbc.state.ram[SMBC_DEMO_TIMER] == 0x18u);
  assert(smbc.state.ram[SMBC_SELECT_TIMER] == 0x10u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u); /* NullJoypad */
  /* the icon moved to the 2-player position */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x24u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 5] == 0xceu);

  /* while the select timer counts, select is ignored */
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_SELECT;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_NUMBER_OF_PLAYERS] == 0x01u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
}

static void menu_b_world_select(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_B;
  smbc.state.ram[SMBC_DEMO_TIMER] = 0x18u;
  smbc.state.ram[SMBC_WORLD_SELECT_ENABLE_FLAG] = 0x01u;
  smbc.state.ram[SMBC_WORLD_SELECT_NUMBER] = 0x07u;

  /* B wraps the world select number and rewrites the VRAM template */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_WORLD_SELECT_NUMBER] == 0x00u);
  assert(smbc.state.ram[SMBC_WORLD_NUMBER] == 0x00u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET + 0] == 0x04u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET + 1] == 0x20u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET + 2] == 0x73u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x01u); /* world number + 1 */
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);

  /* without the enable flag B does nothing but clear the joypad */
  Smbc off;
  smbc_init(&off);
  off.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  off.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  off.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_B;
  off.state.ram[SMBC_DEMO_TIMER] = 0x18u;
  off.state.ram[SMBC_WORLD_SELECT_NUMBER] = 0x03u;
  oper_mode_execution_tree(&off);
  assert(off.state.ram[SMBC_WORLD_SELECT_NUMBER] == 0x03u);
  assert(off.state.ram[SMBC_VRAM_BUFFER1_OFFSET + 0] == 0x00u);
  assert(off.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
}

static void menu_demo_paths(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_DEMO_TIMER] = 0x00u;

  /* timer expired with no input: the demo engine drives the joypad */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_DEMO_ACTION] == 0x01u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x01u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_TITLE); /* no reset */

  /* demo over: modes reset, screen disabled */
  smbc.state.ram[SMBC_DEMO_ACTION] = 0x15u;
  smbc.state.ram[SMBC_DEMO_ACTION_TIMER] = 0x00u;
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  smbc.state.ram[SMBC_SPRITE0_HIT_FLAG] = 0x01u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_SPRITE0_HIT_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);

  /* the lose-life routine during a demo resets the title screen too */
  Smbc lose;
  smbc_init(&lose);
  lose.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  lose.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  lose.state.ram[SMBC_DEMO_TIMER] = 0x18u;
  lose.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x06u;
  lose.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  oper_mode_execution_tree(&lose);
  assert(lose.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
  assert(lose.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
}

int main(void) {
  mushroom_icon();
  demo_engine_cycle();
  menu_start_game();
  menu_continue_with_a_start();
  menu_select_toggles_players();
  menu_b_world_select();
  menu_demo_paths();
  printf("tests/core/title: ok\n");
  return 0;
}
