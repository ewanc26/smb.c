/* tests/core/victory.c — exercises src/core/victory.c: the victory mode
 * task tree's translatable parts — SetupVictoryMode,
 * PrintVictoryMessages and PlayerEndWorld. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/mode.h"
#include "../../src/core/state.h"
#include "../../src/data/music.h"

static void setup_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x12u;
  smbc.state.ram[SMBC_OBJECT_OFFSET] = 0x05u;

  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_DESTINATION_PAGE_LOC] == 0x13u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] ==
         SMBC_EVENT_MUSIC_END_OF_CASTLE);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
  /* past task 0 the wrapper resets the enemy object offset */
  assert(smbc.state.ram[SMBC_OBJECT_OFFSET] == 0x00u);
}

static void bridge_task_skips_enemy_offset_reset(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  smbc.state.ram[SMBC_OBJECT_OFFSET] = 0x05u;

  /* during the bridge collapse the enemy object offset stays put */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_OBJECT_OFFSET] == 0x05u);
}

static void first_messages(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_CURRENT_PLAYER] = 0x00u;

  /* mario's first message */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x0cu);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] == 0x04u);
  assert(smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] == 0x00u);

  /* luigi's first message sits one index up */
  Smbc luigi;
  smbc_init(&luigi);
  luigi.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  luigi.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  luigi.state.ram[SMBC_CURRENT_PLAYER] = 0x01u;
  oper_mode_execution_tree(&luigi);
  assert(luigi.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x0du);
}

static void world_1_through_7_messages(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x02u;

  /* primary 2: "but our princess is in another castle!" */
  smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] = 0x02u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x0eu);

  /* primary 3 counts without printing */
  smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] = 0x03u;
  smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x00u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x00u);
  assert(smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] == 0x04u);

  /* primary 4 sets the world end timer and moves on, skipping counters */
  smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] = 0x04u;
  smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] = 0x00u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_WORLD_END_TIMER] == 0x06u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x04u);
  assert(smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] == 0x00u);
}

static void world_8_messages(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = SMBC_WORLD_8;

  /* primary 3: victory music first, then message $0f */
  smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] = 0x03u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_EVENT_MUSIC_VICTORY);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x0fu);

  /* primaries 4-6 print $10-$12 with no more music */
  for (uint8_t p = 0x04; p <= 0x06; p++) {
    smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] = p;
    smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] = 0x00u;
    smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] = 0x00u;
    oper_mode_execution_tree(&smbc);
    assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == p + 0x0cu);
    assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x00u);
  }
}

static void counter_carry_and_end_at_seven(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;

  /* a nonzero secondary only counts; its carry advances the primary */
  smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] = 0xfcu;
  smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x00u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] == 0x00u);
  assert(smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] == 0x01u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x00u);

  /* primary reaching 7 from the carry sets the world end timer */
  smbc.state.ram[SMBC_SECONDARY_MSG_COUNTER] = 0xfcu;
  smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] = 0x06u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_PRIMARY_MSG_COUNTER] == 0x07u);
  assert(smbc.state.ram[SMBC_WORLD_END_TIMER] == 0x06u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x04u);
}

static void end_world_advances_to_next(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x04u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x03u;
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x02u;

  /* timer still running: nothing happens */
  smbc.state.ram[SMBC_WORLD_END_TIMER] = 0x05u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_WORLD_NUMBER] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_VICTORY);

  /* timer expired: world increments, the next area pointer loads */
  smbc.state.ram[SMBC_WORLD_END_TIMER] = 0x00u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_WORLD_NUMBER] == 0x01u);
  assert(smbc.state.ram[SMBC_AREA_NUMBER] == 0x00u);
  assert(smbc.state.ram[SMBC_LEVEL_NUMBER] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x28u); /* world 2 first area */
  assert(smbc.state.ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_GAME);
}

static void end_world_8_b_button(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x04u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = SMBC_WORLD_8;

  /* no B button: the game waits */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_WORLD_SELECT_ENABLE_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_VICTORY);

  /* B on controller 2: world select unlocks, the game terminates */
  smbc.state.ram[SMBC_SAVED_JOYPAD2_BITS] = SMBC_PAD_B;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_WORLD_SELECT_ENABLE_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_NUMBEROF_LIVES] == 0xffu);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  assert(smbc.state.ram[SMBC_CONTINUE_WORLD] == SMBC_WORLD_8);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_TITLE);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
}

int main(void) {
  setup_task();
  bridge_task_skips_enemy_offset_reset();
  first_messages();
  world_1_through_7_messages();
  world_8_messages();
  counter_carry_and_end_at_seven();
  end_world_advances_to_next();
  end_world_8_b_button();
  printf("tests/core/victory: ok\n");
  return 0;
}
