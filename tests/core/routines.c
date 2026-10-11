/* tests/core/routines.c — exercises src/core/routines.c: the thirteen
 * GameRoutines entries, ScrollHandler and the shared area-change
 * helpers. Table expectations are the smb.asm values at the entrance
 * tables, HalfwayPageNybbles and Hidden1UpCoinAmts. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/routines.h"
#include "../../src/core/state.h"
#include "../../src/data/music.h"

static void entrance_setup_basics(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x03u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x02u; /* not water */

  entrance_game_timer_setup(&smbc);

  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x03u);
  assert(smbc.state.ram[SMBC_VERTICAL_FORCE_DOWN] == 0x28u);
  assert(smbc.state.ram[SMBC_PLAYER_FACING_DIR] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] == 0xffu);
  assert(smbc.state.ram[SMBC_SWIMMING_FLAG] == 0x00u);
  /* alt entrance control 0, entrance ctrl 0: first table entries */
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x28u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_POSITION] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x07u);
}

static void entrance_setup_swimming_flag(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u; /* water */

  entrance_game_timer_setup(&smbc);

  assert(smbc.state.ram[SMBC_SWIMMING_FLAG] == 0x01u);
  /* ChkSwimE: a bubble spawns ahead of the facing direction, $07 = 0
   * on a fresh start gives the $40 timer */
  assert(smbc.state.ram[SMBC_BUBBLE_X_POSITION] == 0x30u);
  assert(smbc.state.ram[SMBC_AIR_BUBBLE_TIMER] == 0x40u);
}

static void entrance_setup_alt_entrance(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x02u;

  entrance_game_timer_setup(&smbc);

  /* x = AltYPosOffset[0] = 8: Y $f0, and the bg-priority overflow
   * byte ($20, GameTimerData's dummy) */
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x38u);
  assert(smbc.state.ram[SMBC_PLAYER_Y_POSITION] == 0xf0u);
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0x20u);
}

static void entrance_setup_game_timer(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_GAME_TIMER_SETTING] = 0x02u;
  smbc.state.ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] = 0x01u;
  smbc.state.ram[SMBC_STAR_INVINCIBLE_TIMER] = 0x12u;

  entrance_game_timer_setup(&smbc);

  /* GameTimerData[2] = 3, hundreds digit one, tens zero */
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY] == 0x03u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY + 1u] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_DISPLAY + 2u] == 0x01u);
  assert(smbc.state.ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_STAR_INVINCIBLE_TIMER] == 0x00u);
}

static void entrance_setup_vine(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_JOYPAD_OVERRIDE] = 0x08u;

  entrance_game_timer_setup(&smbc);

  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x03u);
  /* InitBlock_XY_Pos runs off the seeded position: ($28 + 8) & $f0 =
   * $30, page 0 */
  assert(smbc.state.ram[SMBC_BLOCK_X_POSITION] == 0x30u);
  assert(smbc.state.ram[SMBC_BLOCK_PAGE_LOC] == 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_PAGE_LOC2] == 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_Y_HIGH_POS] == 0x01u);
  assert(smbc.state.ram[SMBC_BLOCK_Y_POSITION] == 0xf0u);
  /* setup_vine put the vine object in slot 5 */
  assert(smbc.state.ram[SMBC_ENEMY_ID + 5u] == 0x2fu);
}

static void lose_life_keeps_playing(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  smbc.state.ram[SMBC_NUMBEROF_LIVES] = 0x03u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x05u;

  player_lose_life(&smbc);

  assert(smbc.state.ram[SMBC_NUMBEROF_LIVES] == 0x02u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  /* world 0 level 1: x = 0, LSB set → low nybble of $56 = 6; past
   * the screen-left page ($05) → start at the beginning */
  assert(smbc.state.ram[SMBC_HALFWAY_PAGE] == 0x00u);
  /* ContinueGame: small player, game mode, task zero */
  assert(smbc.state.ram[SMBC_PLAYER_SIZE] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_STATUS] == 0x00u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == 0x01u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x00u);
}

static void lose_life_halfway_page(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBEROF_LIVES] = 0x02u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x08u;

  player_lose_life(&smbc);

  /* low nybble 6, within the screen-left page: kept */
  assert(smbc.state.ram[SMBC_HALFWAY_PAGE] == 0x06u);
}

static void lose_life_game_over(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_NUMBEROF_LIVES] = 0x00u;
  smbc.state.ram[SMBC_OPER_MODE] = 0x01u;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x04u;

  player_lose_life(&smbc);

  assert(smbc.state.ram[SMBC_NUMBEROF_LIVES] == 0xffu);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_GAME_OVER);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
}

static void entrance_routine_walk_in(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x50u;
  smbc.state.ram[SMBC_PLAYER_ENTRANCE_CTRL] = 0x00u;
  smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x00u;
  smbc.state.ram[SMBC_JOYPAD_OVERRIDE] = 0x08u;

  player_entrance(&smbc);

  /* PlayerRdy: control routine next, flags cleared */
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u);
  assert(smbc.state.ram[SMBC_PLAYER_FACING_DIR] == 0x01u);
  assert(smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x00u);
  assert(smbc.state.ram[SMBC_JOYPAD_OVERRIDE] == 0x00u);
  assert(smbc.state.ram[SMBC_DISABLE_COLLISION_DET] == 0x00u);
}

static void entrance_routine_drops_in(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x10u; /* above $30 */
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x07u;

  player_entrance(&smbc);

  /* AutoControlPlayer(0): no buttons, still routine 7 */
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x07u);
  /* the controller split ran with the zeroed bits */
  assert(smbc.state.ram[SMBC_A_B_BUTTONS] == 0x00u);
  assert(smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] == 0x00u);
}

static void entrance_routine_pipe_rise(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x02u;
  smbc.state.ram[SMBC_JOYPAD_OVERRIDE] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x93u;

  player_entrance(&smbc);

  /* rises one pixel per frame until below $91 */
  assert(smbc.state.ram[SMBC_PLAYER_Y_POSITION] == 0x92u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x00u);

  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x92u;
  player_entrance(&smbc);
  assert(smbc.state.ram[SMBC_PLAYER_Y_POSITION] == 0x91u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x00u);

  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x91u;
  player_entrance(&smbc);
  assert(smbc.state.ram[SMBC_PLAYER_Y_POSITION] == 0x90u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u);
}

static void entrance_routine_off_vine(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x02u;
  smbc.state.ram[SMBC_JOYPAD_OVERRIDE] = 0x08u;
  smbc.state.ram[SMBC_VINE_HEIGHT] = 0x60u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x90u; /* above $99 */
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x60u;
  /* a coherent screen: left edge $0000, right edge $01ff */
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0xffu;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;

  player_entrance(&smbc);

  /* off the vine: collision detection back on, walks right */
  assert(smbc.state.ram[SMBC_DISABLE_COLLISION_DET] == 0x00u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x01u);
  /* past $48 already: PlayerRdy */
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u);
}

static void entrance_routine_climbs_vine(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x02u;
  smbc.state.ram[SMBC_JOYPAD_OVERRIDE] = 0x08u;
  smbc.state.ram[SMBC_VINE_HEIGHT] = 0x60u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xa0u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x20u;

  player_entrance(&smbc);

  /* forced up the vine: climbing, collision off, hole covered */
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x03u);
  assert(smbc.state.ram[SMBC_DISABLE_COLLISION_DET] == 0x01u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x08u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0xb4u] == 0x08u);
  /* not yet at $48: still routine 7 */
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x00u);
}

static void vine_climb_and_exit(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xa0u; /* inside the band */

  vine_auto_climb(&smbc);

  assert(smbc.state.ram[SMBC_JOYPAD_OVERRIDE] == 0x08u);
  assert(smbc.state.ram[SMBC_PLAYER_STATE] == 0x03u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x08u);

  /* above the status bar: the mode change to entry 2 */
  Smbc smbc2;
  smbc_init(&smbc2);
  smbc2.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  smbc2.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x00u;
  smbc2.state.ram[SMBC_PLAYER_Y_POSITION] = 0xd0u;

  vine_auto_climb(&smbc2);

  assert(smbc2.state.ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x02u);
  assert(smbc2.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
  assert(smbc2.state.ram[SMBC_OPER_MODE_TASK] == 0x00u);
}

static void vertical_pipe_sinks_and_changes_area(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xa0u;
  smbc.state.ram[SMBC_WARP_ZONE_CONTROL] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u; /* water → entry 1 */
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  smbc.state.ram[SMBC_CHANGE_AREA_TIMER] = 0x01u;

  vertical_pipe_entry(&smbc);

  assert(smbc.state.ram[SMBC_PLAYER_Y_POSITION] == 0xa1u);
  assert(smbc.state.ram[SMBC_CHANGE_AREA_TIMER] == 0x00u);
  assert(smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x01u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
}

static void vertical_pipe_warp_zone(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_WARP_ZONE_CONTROL] = 0x01u;
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  smbc.state.ram[SMBC_CHANGE_AREA_TIMER] = 0x01u;

  vertical_pipe_entry(&smbc);

  /* warp zones always enter at the top */
  assert(smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x00u);
}

static void side_pipe_walks_out(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  /* a coherent screen: left edge $0000, right edge $01ff */
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0xffu;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x53u;
  smbc.state.ram[SMBC_CHANGE_AREA_TIMER] = 0x01u;

  side_exit_pipe_entry(&smbc);

  /* mid-tile: keep sliding right at speed 8 */
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x08u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x01u);
  assert(smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x02u);
}

static void side_pipe_aligned_stops(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  /* a coherent screen: left edge $0000, right edge $01ff */
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0xffu;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x50u; /* low nybble zero */
  smbc.state.ram[SMBC_CHANGE_AREA_TIMER] = 0x02u;

  side_exit_pipe_entry(&smbc);

  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
  /* timer not expired: no mode change yet */
  assert(smbc.state.ram[SMBC_CHANGE_AREA_TIMER] == 0x01u);
  assert(smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x00u);
}

static void change_size_sequence(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] = 0x00u;
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0xf8u;

  player_change_size(&smbc);

  /* InitChangeSize: flag set, small flips to big */
  assert(smbc.state.ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_SIZE] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_CTRL] == 0x00u);

  smbc.state.ram[SMBC_TIMER_CONTROL] = 0xc4u;
  player_change_size(&smbc);

  /* DonePlayerTask */
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u);

  /* a size change already running is not reinitialised */
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u;
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0xf8u;
  player_change_size(&smbc);
  assert(smbc.state.ram[SMBC_PLAYER_SIZE] == 0x01u);
}

static void injury_blink_timer_paths(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x00u;
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0xf0u;

  player_injury_blink(&smbc);

  /* exactly $f0 falls through into InitChangeSize: big shrinks */
  assert(smbc.state.ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_SIZE] == 0x01u);

  Smbc smbc2;
  smbc_init(&smbc2);
  smbc2.state.ram[SMBC_TIMER_CONTROL] = 0xf1u;
  player_injury_blink(&smbc2);
  /* past $f0: leaves without doing anything */
  assert(smbc2.state.ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] == 0x00u);

  Smbc smbc3;
  smbc_init(&smbc3);
  smbc3.state.ram[SMBC_TIMER_CONTROL] = 0xc8u;
  player_injury_blink(&smbc3);
  assert(smbc3.state.ram[SMBC_TIMER_CONTROL] == 0x00u);
  assert(smbc3.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u);
}

static void injury_blink_runs_control(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x80u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0xc3u;

  player_injury_blink(&smbc);

  /* PlayerCtrlRoutine ran: the controller split */
  assert(smbc.state.ram[SMBC_A_B_BUTTONS] == 0xc0u);
  assert(smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] == 0x03u);
  assert(smbc.state.ram[SMBC_UP_DOWN_BUTTONS] == 0x00u);
}

static void death_timer_paths(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0xf0u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x80u;

  player_death(&smbc);

  /* $f0 and up: the control routine does not run */
  assert(smbc.state.ram[SMBC_A_B_BUTTONS] == 0x00u);

  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x40u;
  player_death(&smbc);
  assert(smbc.state.ram[SMBC_A_B_BUTTONS] == 0x80u);
}

static void fire_flower_palette_cycle(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0x80u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x0cu; /* >> 2 = 3 */
  smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] = 0xf8u;

  player_fire_flower(&smbc);

  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0xfbu);
}

static void fire_flower_reset(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_TIMER_CONTROL] = 0xc0u;
  smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] = 0xfbu;
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u; /* fiery */

  player_fire_flower(&smbc);

  /* palette back to the fiery colours, timers running */
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0xf8u);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0x00u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u);
}

static void flagpole_slide_sound_and_hold(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_ENEMY_ID + 5u] = 0x30u;
  smbc.state.ram[SMBC_FLAGPOLE_SOUND_QUEUE] = 0x04u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xa0u; /* at the bottom */

  flagpole_slide(&smbc);

  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x04u);
  assert(smbc.state.ram[SMBC_FLAGPOLE_SOUND_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);

  Smbc smbc2;
  smbc_init(&smbc2);
  smbc2.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc2.state.ram[SMBC_ENEMY_ID + 5u] = 0x30u;
  smbc2.state.ram[SMBC_PLAYER_Y_POSITION] = 0x50u;

  flagpole_slide(&smbc2);

  /* above $9e: down held */
  assert(smbc2.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x04u);
}

static void flagpole_slide_residual(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_ENEMY_ID + 5u] = 0x00u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x04u;

  flagpole_slide(&smbc);

  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x05u);
}

static void end_level_music_once(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xb0u;
  smbc.state.ram[SMBC_SCROLL_LOCK] = 0x01u;
  smbc.state.ram[SMBC_STAR_FLAG_TASK_CONTROL] = 0x00u;

  player_end_level(&smbc);

  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x01u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] ==
         SMBC_EVENT_MUSIC_END_OF_LEVEL);
  assert(smbc.state.ram[SMBC_SCROLL_LOCK] == 0x00u);
  /* collision bits d0 clear: the star flag task advances */
  assert(smbc.state.ram[SMBC_STAR_FLAG_TASK_CONTROL] == 0x01u);
  /* inside the castle: the bg priority bit */
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0x20u);
}

static void end_level_next_area(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x02u;
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x02u;
  smbc.state.ram[SMBC_COIN_TALLY_FOR_1UPS] = 0x20u;
  smbc.state.ram[SMBC_STAR_FLAG_TASK_CONTROL] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_COLLISION_BITS] = 0x01u; /* d0 set */

  player_end_level(&smbc);

  /* third area of the world: 32 coins ≥ 21 → the hidden 1-up */
  assert(smbc.state.ram[SMBC_LEVEL_NUMBER] == 0x03u);
  assert(smbc.state.ram[SMBC_HIDDEN_1UP_FLAG] == 0x01u);
  /* NextArea: area advanced, screen disabled, silence */
  assert(smbc.state.ram[SMBC_AREA_NUMBER] == 0x03u);
  assert(smbc.state.ram[SMBC_AREA_POINTER] != 0x00u);
  assert(smbc.state.ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_HALFWAY_PAGE] == 0x00u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
}

static void ctrl_routine_crouch_nulls_directions(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x07u; /* down + L + R */
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x00u;       /* grounded */

  player_ctrl_routine(&smbc);

  assert(smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] == 0x00u);
  assert(smbc.state.ram[SMBC_UP_DOWN_BUTTONS] == 0x00u);
  assert(smbc.state.ram[SMBC_A_B_BUTTONS] == 0x00u);
}

static void ctrl_routine_bound_box_and_dir(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u; /* small */
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0xf4u; /* left */

  player_ctrl_routine(&smbc);

  assert(smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_MOVING_DIR] == 0x02u);

  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x00u;
  smbc.state.ram[SMBC_CROUCHING_FLAG] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;

  player_ctrl_routine(&smbc);

  assert(smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL] == 0x02u);
  assert(smbc.state.ram[SMBC_PLAYER_MOVING_DIR] == 0x01u);
}

static void ctrl_routine_water_band(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u; /* water */
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xd0u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x80u;

  player_ctrl_routine(&smbc);

  /* at the bottom of the water band the controller is dead */
  assert(smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] == 0x00u);
  assert(smbc.state.ram[SMBC_A_B_BUTTONS] == 0x00u);
}

static void ctrl_routine_bg_priority_clear(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x60u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] = 0xffu;

  player_ctrl_routine(&smbc);

  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0xdfu);

  /* above the 64th pixel the bit survives */
  Smbc smbc2;
  smbc_init(&smbc2);
  smbc2.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc2.state.ram[SMBC_PLAYER_Y_POSITION] = 0x20u;
  smbc2.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc2.state.ram[SMBC_PLAYER_SPR_ATTRIB] = 0xffu;

  player_ctrl_routine(&smbc2);

  assert(smbc2.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0xffu);
}

static void hole_death_waits_for_music(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x06u;
  smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] = 0x00u;
  smbc.state.ram[SMBC_CLOUD_TYPE_OVERRIDE] = 0x00u;
  smbc.state.ram[SMBC_EVENT_MUSIC_BUFFER] = 0x01u;

  player_ctrl_routine(&smbc);

  assert(smbc.state.ram[SMBC_SCROLL_LOCK] == 0x01u);
  /* the death music queued once */
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x01u);
  assert(smbc.state.ram[SMBC_DEATH_MUSIC_LOADED] == 0x01u);
  /* still playing: routine 8 */
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u);

  smbc.state.ram[SMBC_EVENT_MUSIC_BUFFER] = 0x00u;
  player_ctrl_routine(&smbc);

  /* finished: PlayerLoseLife */
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x06u);
}

static void hole_cloud_exit(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x04u;
  smbc.state.ram[SMBC_CLOUD_TYPE_OVERRIDE] = 0x01u;
  smbc.state.ram[SMBC_JOYPAD_OVERRIDE] = 0x08u;

  player_ctrl_routine(&smbc);

  /* no death: the cloud bonus exit, entry mode 3 */
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_JOYPAD_OVERRIDE] == 0x00u);
  assert(smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x03u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
}

static void scroll_handler_moves_screen(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_X_SCROLL] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_POS_FORSCROLL] = 0x60u;
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0xfeu;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x02u;
  smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] = 0x91u;

  scroll_handler(&smbc);

  /* under 112 pixels: one less than the scroll value */
  assert(smbc.state.ram[SMBC_SCROLL_AMOUNT] == 0x04u);
  assert(smbc.state.ram[SMBC_SCROLL_THIRTY_TWO] == 0x04u);
  assert(smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] == 0x02u);
  assert(smbc.state.ram[SMBC_HORIZONTAL_SCROLL] == 0x02u);
  /* the carry crossed a page: $03, mirrored into d0 (was 1 → 1) */
  assert(smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] == 0x03u);
  assert(smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] == 0x91u);
  assert(smbc.state.ram[SMBC_SCROLL_INTERVAL_TIMER] == 0x08u);
}

static void scroll_handler_full_speed_past_midway(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_X_SCROLL] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_POS_FORSCROLL] = 0x70u;

  scroll_handler(&smbc);

  assert(smbc.state.ram[SMBC_SCROLL_AMOUNT] == 0x05u);
}

static void scroll_handler_locked(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCROLL_LOCK] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_SCROLL] = 0x08u;
  smbc.state.ram[SMBC_PLAYER_POS_FORSCROLL] = 0x80u;

  scroll_handler(&smbc);

  assert(smbc.state.ram[SMBC_SCROLL_AMOUNT] == 0x00u);
  assert(smbc.state.ram[SMBC_SCROLL_THIRTY_TWO] == 0x00u);
}

static void scroll_handler_clamps_at_edges(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_POS_FORSCROLL] = 0x40u; /* no scroll */
  /* left-edge offscreen: d7 set */
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x20u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x40u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x03u;
  smbc.state.ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x02u; /* holding left */
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0xf0u;

  scroll_handler(&smbc);

  /* pinned to the left edge: $40 - $00, page 3 - 0 */
  assert(smbc.state.ram[SMBC_PLAYER_X_POSITION] == 0x40u);
  assert(smbc.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x03u);
  assert(smbc.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
  assert(smbc.state.ram[SMBC_PLATFORM_X_SCROLL] == 0x00u);

  /* the right-edge clamp needs d5 with d7 clear, which the offscreen
   * table gives when the player is within sixteen pixels left of
   * the right edge (the DividePDiff path, offsets 0 and 1) */
  Smbc smbc2;
  smbc_init(&smbc2);
  smbc2.state.ram[SMBC_PLAYER_POS_FORSCROLL] = 0x40u;
  smbc2.state.ram[SMBC_PLAYER_X_POSITION] = 0xf8u;
  smbc2.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  smbc2.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x00u;
  smbc2.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x01u;
  smbc2.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0xffu;
  smbc2.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc2.state.ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x01u; /* holding right */
  smbc2.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;

  scroll_handler(&smbc2);

  /* pinned to the right edge: $ff - $10 = $ef, page 1 - 0 */
  assert(smbc2.state.ram[SMBC_PLAYER_X_POSITION] == 0xefu);
  assert(smbc2.state.ram[SMBC_PLAYER_PAGE_LOC] == 0x01u);
  assert(smbc2.state.ram[SMBC_PLAYER_X_SPEED] == 0x00u);
}

int main(void) {
  entrance_setup_basics();
  entrance_setup_swimming_flag();
  entrance_setup_alt_entrance();
  entrance_setup_game_timer();
  entrance_setup_vine();
  lose_life_keeps_playing();
  lose_life_halfway_page();
  lose_life_game_over();
  entrance_routine_walk_in();
  entrance_routine_drops_in();
  entrance_routine_pipe_rise();
  entrance_routine_off_vine();
  entrance_routine_climbs_vine();
  vine_climb_and_exit();
  vertical_pipe_sinks_and_changes_area();
  vertical_pipe_warp_zone();
  side_pipe_walks_out();
  side_pipe_aligned_stops();
  change_size_sequence();
  injury_blink_timer_paths();
  injury_blink_runs_control();
  death_timer_paths();
  fire_flower_palette_cycle();
  fire_flower_reset();
  flagpole_slide_sound_and_hold();
  flagpole_slide_residual();
  end_level_music_once();
  end_level_next_area();
  ctrl_routine_crouch_nulls_directions();
  ctrl_routine_bound_box_and_dir();
  ctrl_routine_water_band();
  ctrl_routine_bg_priority_clear();
  hole_death_waits_for_music();
  hole_cloud_exit();
  scroll_handler_moves_screen();
  scroll_handler_full_speed_past_midway();
  scroll_handler_locked();
  scroll_handler_clamps_at_edges();
  printf("tests/core/routines: ok\n");
  return 0;
}
