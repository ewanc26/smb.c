/* tests/core/victory.c — exercises src/core/victory.c: the victory mode
 * task tree's translatable parts — SetupVictoryMode,
 * PrintVictoryMessages and PlayerEndWorld. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/enemies.h"
#include "../../src/core/frame.h"
#include "../../src/core/initialize.h"
#include "../../src/core/mode.h"
#include "../../src/core/state.h"
#include "../../src/data/music.h"

/* the victory tasks past bridge collapse run EnemiesAndLoopsCore and
 * the AutoPlayer tail, both of which read the area object data — on
 * hardware the pointers were loaded when the area started, so the
 * fixtures do the same with world 1-1 */
static void load_world_1_1(Smbc *smbc) {
  smbc->state.ram[SMBC_AREA_POINTER] = 0x25u;
  initialize_area(smbc);
}

static void setup_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
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

static void bridge_setm2_without_bowser(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] = 0x01u;

  /* no bowser in the slots: silence the music and move on */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x01u);
  /* past task 0 the wrapper resets the enemy object offset */
  assert(smbc.state.ram[SMBC_OBJECT_OFFSET] == 0x00u);
}

static void bridge_remove_step(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  smbc.state.ram[SMBC_BOWSER_FRONT_OFFSET] = 0x04u;
  smbc.state.ram[SMBC_ENEMY_ID + 4] = SMBC_BOWSER;
  smbc.state.ram[SMBC_ENEMY_STATE + 4] = 0x00u;
  smbc.state.ram[SMBC_BOWSER_FEET_COUNTER] = 0x01u;

  /* the feet counter expiring blanks the axe metatile (the first
   * BridgeCollapseData entry, $221a) and paces the next step */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_BOWSER_FEET_COUNTER] == 0x04u);
  assert(smbc.state.ram[SMBC_BOWSER_BODY_CONTROLS] == 0x01u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 0] == 0x22u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 1] == 0x1au);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 2] == 0x02u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x24u); /* blank quad */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 10] == 0x00u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x0au);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == SMBC_SFX_BLAST);
  assert(smbc.state.ram[SMBC_NOISE_SOUND_QUEUE] == SMBC_SFX_BRICK_SHATTER);
  assert(smbc.state.ram[SMBC_BRIDGE_COLLAPSE_OFFSET] == 0x01u);
  /* the object offset follows bowser; the task stays on the bridge */
  assert(smbc.state.ram[SMBC_OBJECT_OFFSET] == 0x04u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);

  /* the counter still ticking skips the collapse step entirely */
  Smbc wait;
  smbc_init(&wait);
  load_world_1_1(&wait);
  wait.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  wait.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  wait.state.ram[SMBC_BOWSER_FRONT_OFFSET] = 0x04u;
  wait.state.ram[SMBC_ENEMY_ID + 4] = SMBC_BOWSER;
  wait.state.ram[SMBC_ENEMY_STATE + 4] = 0x00u;
  wait.state.ram[SMBC_BOWSER_FEET_COUNTER] = 0x03u;
  oper_mode_execution_tree(&wait);
  assert(wait.state.ram[SMBC_BOWSER_FEET_COUNTER] == 0x02u);
  assert(wait.state.ram[SMBC_BRIDGE_COLLAPSE_OFFSET] == 0x00u);
  assert(wait.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x00u);
}

static void walk_scrolls_toward_castle(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_DESTINATION_PAGE_LOC] = 0x07u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x30u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x04u;
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x00u;
  smbc.state.ram[SMBC_SCROLL_FRACTIONAL] = 0x80u;

  /* off the destination page: walk right, and the fractional scroll
   * ($80 + $80 carries) makes this a two-pixel frame */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x01u);
  assert(smbc.state.ram[SMBC_VICTORY_WALK_CONTROL] == 0x02u);
  assert(smbc.state.ram[SMBC_SCROLL_FRACTIONAL] == 0x00u);
  assert(smbc.state.ram[SMBC_SCROLL_AMOUNT] == 0x02u);
  assert(smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] == 0x02u);
  assert(smbc.state.ram[SMBC_HORIZONTAL_SCROLL] == 0x02u);
  assert(smbc.state.ram[SMBC_SCROLL_INTERVAL_TIMER] == 0x08u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
}

static void walk_arrived_advances_task(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x07u;
  smbc.state.ram[SMBC_DESTINATION_PAGE_LOC] = 0x07u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u; /* past $60 */
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x07u;

  /* at the gate with the screen caught up: no walk, no scroll, next
   * task */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
  assert(smbc.state.ram[SMBC_VICTORY_WALK_CONTROL] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x03u);

  /* arrived but the screen still catching up: the walk control stays
   * set so the task holds */
  Smbc lag;
  smbc_init(&lag);
  load_world_1_1(&lag);
  lag.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  lag.state.ram[SMBC_OPER_MODE_TASK] = 0x02u;
  lag.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x07u;
  lag.state.ram[SMBC_DESTINATION_PAGE_LOC] = 0x07u;
  lag.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  lag.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x06u;
  oper_mode_execution_tree(&lag);
  assert(lag.state.ram[SMBC_VICTORY_WALK_CONTROL] == 0x01u);
  assert(lag.state.ram[SMBC_OPER_MODE_TASK] == 0x02u);
  /* no carry this frame ($00 + $80): one pixel */
  assert(lag.state.ram[SMBC_SCROLL_AMOUNT] == 0x01u);
}

static void bridge_collapse_complete(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  smbc.state.ram[SMBC_BOWSER_FRONT_OFFSET] = 0x04u;
  smbc.state.ram[SMBC_ENEMY_ID + 4] = SMBC_BOWSER;
  smbc.state.ram[SMBC_ENEMY_STATE + 4] = 0x00u;
  smbc.state.ram[SMBC_BOWSER_FEET_COUNTER] = 0x01u;
  smbc.state.ram[SMBC_BRIDGE_COLLAPSE_OFFSET] = 0x0eu;
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 4] = 0x09u;
  smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 4] = 0x07u;

  /* the last bridge metatile gone: bowser drops, defeated */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_BRIDGE_COLLAPSE_OFFSET] == 0x0fu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 4] == 0x00u); /* InitVStf */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 4] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 4] == 0x40u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == SMBC_SFX_BOWSER_FALL);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
}

static void bridge_defeated_bowser_falls(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  smbc.state.ram[SMBC_BOWSER_FRONT_OFFSET] = 0x04u;
  smbc.state.ram[SMBC_ENEMY_ID + 4] = SMBC_BOWSER;
  smbc.state.ram[SMBC_ENEMY_STATE + 4] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 4] = 0x10u; /* above $e0 */

  /* MoveD_Bowser: gravity starts pulling him down through the gap */
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 4] == 0x0fu);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);

  /* low enough: SetM2 silences the music and moves on */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 4] = 0xe0u;
  smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] = 0x01u;
  oper_mode_execution_tree(&smbc);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x01u);
}

static void first_messages(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
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
  load_world_1_1(&luigi);
  luigi.state.ram[SMBC_OPER_MODE] = SMBC_MODE_VICTORY;
  luigi.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  luigi.state.ram[SMBC_CURRENT_PLAYER] = 0x01u;
  oper_mode_execution_tree(&luigi);
  assert(luigi.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x0du);
}

static void world_1_through_7_messages(void) {
  Smbc smbc;
  smbc_init(&smbc);
  load_world_1_1(&smbc);
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
  load_world_1_1(&smbc);
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
  load_world_1_1(&smbc);
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
  load_world_1_1(&smbc);
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
  load_world_1_1(&smbc);
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
  bridge_setm2_without_bowser();
  bridge_remove_step();
  walk_scrolls_toward_castle();
  walk_arrived_advances_task();
  bridge_collapse_complete();
  bridge_defeated_bowser_falls();
  first_messages();
  world_1_through_7_messages();
  world_8_messages();
  counter_carry_and_end_at_seven();
  end_world_advances_to_next();
  end_world_8_b_button();
  printf("tests/core/victory: ok\n");
  return 0;
}
