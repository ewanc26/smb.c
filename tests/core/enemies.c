/* tests/core/enemies.c — exercises src/core/enemies.c: the enemy data
 * interpreter, driven through EnemiesAndLoopsCore with crafted enemy
 * object streams and screen boundary states. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/enemies.h"
#include "../../src/core/frame.h"

/* a fresh state with the given stream; the parser task number sits at
 * zero, clear of the seventh task the core leaves on */
static void init_interp(Smbc *smbc, const uint8_t *stream) {
  smbc_init(smbc);
  smbc->state.enemy_data = stream;
}

static void task_seven_leaves(void) {
  /* smb.asm: ChkAreaTsk — on the parser's seventh task the core leaves
   * without touching the stream */
  static const uint8_t stream[] = {0x29u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x07u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x00u);
}

static void active_flag_pending(void) {
  /* smb.asm: an active slot jumps to the enemy runners — pending their
   * own atom, the stream is not walked */
  static const uint8_t stream[] = {0x29u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x00u);
}

static void bowser_pair_flag(void) {
  /* smb.asm: ChkBowserF — a flag with its MSB set pairs with the slot
   * indexed by its low nybble; when the pair is inactive the flag is
   * cleared, when active the core leaves */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_ENEMY_FLAG + 2] = 0x81u;
  enemies_and_loops_core(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2] == 0x00u);

  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_ENEMY_FLAG + 2] = 0x81u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 1] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2] == 0x81u);
}

static void eod_leaves(void) {
  /* smb.asm: the $ff terminator jumps to the frenzy buffer check,
   * which leaves when both it and the vine flag are empty */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x00u);
}

static void frenzy_queue_spawn(void) {
  /* smb.asm: ChkEnemyFrenzy — the queue's object moves into the slot
   * with its flag set and state and queue cleared; the checkpoint adds
   * eight to the vertical coordinate for identifiers below $15 */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_ENEMY_FRENZY_QUEUE] = 0x06u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x08u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCR_BITS_MASKED + 0] == 0x01u);
}

static void spawn_goomba_at_right_edge(void) {
  /* smb.asm: PositionEnemyObj — a goomba between the right boundary
   * and the extended boundary spawns with its column, vertical
   * coordinate times sixteen (plus the checkpoint's eight), and the
   * data offset advanced */
  static const uint8_t stream[] = {0x29u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x98u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_OBJECT_PAGE_SEL] == 0x00u);
}

static void hard_mode_mutates_goomba(void) {
  /* smb.asm: BuzzyBeetleMutate — primary hard mode turns the goomba
   * into a buzzy beetle */
  static const uint8_t stream[] = {0x29u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_PRIMARY_HARD_MODE] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
}

static void hard_mode_object_skipped(void) {
  /* smb.asm: d6 of the second byte marks a hard mode object; without
   * secondary hard mode it is skipped entirely */
  static const uint8_t stream[] = {0x29u, 0x46u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x02u);

  /* with secondary hard mode on, the same object spawns */
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
}

static void object_behind_boundary(void) {
  /* smb.asm: an object left of the right boundary is skipped without
   * spawning (CheckThreeBytes → Inc2B) */
  static const uint8_t stream[] = {0x29u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x40u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x02u);
}

static void object_beyond_extended_waits(void) {
  /* smb.asm: CheckRightExtBounds — an object past the boundary plus
   * 48 pixels is not consumed; the position writes still happen */
  static const uint8_t stream[] = {0x59u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x50u);
}

static void page_control_row_0f(void) {
  /* smb.asm: a row $0f object stores the low six bits of its second
   * byte as the enemy page control; the next object spawns on it */
  static const uint8_t stream[] = {0x0fu, 0x45u, 0x29u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OBJECT_PAGE_LOC] == 0x05u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_OBJECT_PAGE_SEL] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);

  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x05u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x05u);
}

static void page_select_bit(void) {
  /* smb.asm: d7 of the second byte increments the enemy page control
   * once (the page select guards repeats) */
  static const uint8_t stream[] = {0x29u, 0x86u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_OBJECT_PAGE_SEL] == 0x00u);
}

static void sixth_slot_residual(void) {
  /* smb.asm: the residual check keeps ordinary enemies out of the
   * sixth slot, excepting the $2e identifier and row $0e objects */
  static const uint8_t stream[] = {0x29u, 0x06u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  enemies_and_loops_core(&smbc.state, 0x05u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 5] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x00u);

  static const uint8_t powerup[] = {0x29u, 0x2eu, 0xffu};
  init_interp(&smbc, powerup);
  enemies_and_loops_core(&smbc.state, 0x05u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 5] == 0x2eu);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 5] == 0x01u);
}

static void row_0e_area_jump(void) {
  /* smb.asm: ParseRow0e — when the third byte's high bits name the
   * current world, the second byte becomes the area pointer and the
   * low five the entrance page; either way three bytes are consumed */
  static const uint8_t stream[] = {0x0eu, 0x25u, 0x65u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x03u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x25u);
  assert(smbc.state.ram[SMBC_ENTRANCE_PAGE] == 0x05u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x03u);

  /* a different world leaves the pointer alone but still advances */
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x00u);
  assert(smbc.state.ram[SMBC_ENTRANCE_PAGE] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x03u);
}

static void group_enemies(void) {
  /* smb.asm: HandleGroupEnemies — id $37 is two goombas at the right
   * edge, 24 pixels apart, vertical $b0 (plus the checkpoint's 8) */
  static const uint8_t stream[] = {0x29u, 0x37u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x02u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_OBJECT_PAGE_LOC] = 0x02u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 1] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 1] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 1] == 0x38u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0xb8u);
  assert(smbc.state.ram[SMBC_NUMBER_OF_GROUP_ENEMIES] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x02u);
}

static void group_enemies_three_high(void) {
  /* smb.asm: id $3a (d0 and d1 of the relative id set) is three
   * enemies at the higher vertical coordinate */
  static const uint8_t stream[] = {0x29u, 0x3au, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 1] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 3] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x78u);
}

static void group_enemies_single_id(void) {
  /* smb.asm: ids $3b-$3e spawn green koopa troopas, not goombas, and
   * hard mode does not mutate them */
  static const uint8_t stream[] = {0x29u, 0x3bu, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_PRIMARY_HARD_MODE] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
}

static void group_enemies_no_slots(void) {
  /* smb.asm: GSltLp's NextED — the group fills the only free slot and
   * abandons the rest, but the data offset still advances */
  static const uint8_t stream[] = {0x29u, 0x37u, 0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  for (uint8_t s = 0; s < 0x04u; s++) {
    smbc.state.ram[SMBC_ENEMY_FLAG + s] = 0x01u;
  }
  enemies_and_loops_core(&smbc.state, 0x04u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 4] == 0x06u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 4] == 0x01u);
  assert(smbc.state.ram[SMBC_NUMBER_OF_GROUP_ENEMIES] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_DATA_OFFSET] == 0x02u);
}

static void vine_from_frenzy_check(void) {
  /* smb.asm: CheckFrenzyBuffer — a vine flag offset of one puts the
   * vine in the identifier without setting the enemy flag */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_VINE_FLAG_OFFSET] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x2fu);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x00u);
}

static void loop_command_pass(void) {
  /* smb.asm: at the loop's page, on solid ground at the right height,
   * the command clears without a loopback */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_LOOP_COMMAND] = 0x01u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x03u;
  smbc.state.ram[SMBC_CURRENT_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x08u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_LOOP_COMMAND] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x08u);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_PASS_CNTR] == 0x00u);
}

static void loop_command_wrong_column(void) {
  /* smb.asm: the loop only processes on column zero */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_LOOP_COMMAND] = 0x01u;
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_LOOP_COMMAND] == 0x01u);
}

static void loop_command_fail_loopback(void) {
  /* smb.asm: WrongChk/DoLpBack — failing the loop sends the player,
   * the screen and both parsers back four pages, rewinds the level
   * data offset by the table, and kills every enemy */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_LOOP_COMMAND] = 0x01u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x03u;
  smbc.state.ram[SMBC_CURRENT_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x50u; /* not the loop's $40 */
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x08u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_AREA_OBJECT_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_AREA_DATA_OFFSET] = 0x30u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 2] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 2] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 2] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x12u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x04u);
  assert(smbc.state.ram[SMBC_CURRENT_PAGE_LOC] == 0x01u);
  assert(smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] == 0x01u);
  assert(smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] == 0x01u);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_PAGE_LOC] == 0x01u);
  assert(smbc.state.ram[SMBC_AREA_DATA_OFFSET] == 0x12u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] == 0x00u);
  assert(smbc.state.ram[SMBC_LOOP_COMMAND] == 0x00u);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_PASS_CNTR] == 0x00u);
}

static void loop_command_world7_multi(void) {
  /* smb.asm: world 7's three-part loop counts passes and correct
   * positions separately; three of each clears the counters, while a
   * wrong third pass loops back */
  static const uint8_t stream[] = {0xffu};
  Smbc smbc;
  init_interp(&smbc, stream);
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x06u;
  smbc.state.ram[SMBC_CURRENT_PAGE_LOC] = 0x04u; /* table index two */
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xb0u;

  smbc.state.ram[SMBC_LOOP_COMMAND] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_PASS_CNTR] == 0x01u);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_CORRECT_CNTR] == 0x01u);

  smbc.state.ram[SMBC_LOOP_COMMAND] = 0x01u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_PASS_CNTR] == 0x02u);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_CORRECT_CNTR] == 0x02u);

  /* a wrong third pass: the counters complete mismatched, so the game
   * loops back and clears them */
  smbc.state.ram[SMBC_LOOP_COMMAND] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x08u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x05u;
  smbc.state.ram[SMBC_AREA_OBJECT_PAGE_LOC] = 0x05u;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x04u);
  assert(smbc.state.ram[SMBC_AREA_DATA_OFFSET] == 0x0eu);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_PASS_CNTR] == 0x00u);
  assert(smbc.state.ram[SMBC_MULTI_LOOP_CORRECT_CNTR] == 0x00u);
}

int main(void) {
  task_seven_leaves();
  active_flag_pending();
  bowser_pair_flag();
  eod_leaves();
  frenzy_queue_spawn();
  spawn_goomba_at_right_edge();
  hard_mode_mutates_goomba();
  hard_mode_object_skipped();
  object_behind_boundary();
  object_beyond_extended_waits();
  page_control_row_0f();
  page_select_bit();
  sixth_slot_residual();
  row_0e_area_jump();
  group_enemies();
  group_enemies_three_high();
  group_enemies_single_id();
  group_enemies_no_slots();
  vine_from_frenzy_check();
  loop_command_pass();
  loop_command_wrong_column();
  loop_command_fail_loopback();
  loop_command_world7_multi();
  printf("tests/core/enemies: ok\n");
  return 0;
}
