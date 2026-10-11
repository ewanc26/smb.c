/* tests/core/playergfx.c — exercises src/core/playergfx.c: pose
 * selection, animation, the grow/shrink interleave, the fireball
 * overlay, the swim kick, offscreen parking and the intermediate
 * display. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/playergfx.h"
#include "../../src/core/frame.h"
#include "../../src/core/initialize.h"
#include "../../src/core/state.h"

static void standing_big(Smbc *smbc) {
  smbc_init(smbc);
  smbc->state.ram[SMBC_PLAYER_SPR_DATA_OFFSET] = 0x04u;
  smbc->state.ram[SMBC_PLAYER_REL_XPOS] = 0x60u;
  smbc->state.ram[SMBC_PLAYER_REL_YPOS] = 0x80u;
  smbc->state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u; /* right */
  smbc->state.ram[SMBC_PLAYER_SIZE] = 0x00u;       /* big */
}

static void standing_pose_draws(void) {
  /* smb.asm: ProcOnGroundActs — no speed or buttons: the big standing
   * offset, frame control reset */
  Smbc smbc;
  standing_big(&smbc);
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0xc8u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_CTRL] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_POS_FORSCROLL] == 0x60u);
  /* big standing row: {00,01,4c,4d,4a,4a,4b,4b} */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x04u] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x08u] == 0x01u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x0cu] == 0x4cu);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x10u] == 0x4du);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x04u] == 0x80u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x14u] == 0x90u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x04u] == 0x60u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x08u] == 0x68u);
  /* smb.asm: ChkForPlayerAttrib — the standing offset fixes row four:
   * first sprite's flips cleared, second horizontally flipped */
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x1cu] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x20u] == 0x40u);
}

static void walking_animates(void) {
  /* smb.asm: ActionWalkRun — moving at low speed: the walk cycle,
   * frame control advancing as the timer expires */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_PLAYER_SIZE] = 0x01u; /* small */
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_XSPEED_ABS] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_ANIM_TIMER_SET] = 0x08u;
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0x60u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_TIMER] == 0x08u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_CTRL] == 0x01u);
  /* small walking frame 1: {fc,fc,fc,fc,32,33,34,35} */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x04u] == 0xfcu);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x14u] == 0x32u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x18u] == 0x33u);
  /* second frame once the timer runs down */
  smbc.state.ram[SMBC_PLAYER_ANIM_TIMER] = 0x00u;
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x14u] == 0x36u);
}

static void skidding_pose(void) {
  /* smb.asm: fast enough, moving against the facing: the skid frame */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_PLAYER_X_SPEED] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_XSPEED_ABS] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_MOVING_DIR] = 0x01u; /* moving right */
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x02u; /* facing left */
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0x18u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_CTRL] == 0x00u);
}

static void jumping_pose(void) {
  /* smb.asm: jumping, not crouching: the jump frame */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0x20u);
}

static void injury_blink_skips_odd_frames(void) {
  /* smb.asm: an injured player is only drawn every other frame */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_INJURY_TIMER] = 0x08u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u; /* odd: skip */
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x04u] == 0x00u);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x02u; /* even: draw */
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x04u] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x08u] == 0x01u);
}

static void killed_pose_fixes_two_rows(void) {
  /* smb.asm: PlayerKilled — the killed pose, with the flip bits
   * fixed on rows three and four */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x0bu;
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0xb0u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x14u] == 0x9eu);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x1cu] == 0x9fu);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x14u] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x18u] == 0x40u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x1cu] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x20u] == 0x40u);
}

static void fireball_throw_overlays(void) {
  /* smb.asm: while the throw timer runs the throwing pose overlays,
   * the timer left at the animation timer's value */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_FIREBALL_THROWING_TIMER] = 0x10u;
  smbc.state.ram[SMBC_PLAYER_ANIM_TIMER] = 0x08u;
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0x58u);
  assert(smbc.state.ram[SMBC_FIREBALL_THROWING_TIMER] == 0x08u);
  /* big throwing row: {08,09,28,29,2a,2b,0e,0f} */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x04u] == 0x08u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x1cu] == 0x0eu);
}

static void swim_kick_patches_tile(void) {
  /* smb.asm: the swimming kick — sprite 7/8's tile patched every
   * eighth frame while swimming */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_SWIMMING_FLAG] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x01u;
  smbc.state.ram[SMBC_JUMP_SWIM_TIMER] = 0x20u;
  player_gfx_handler(&smbc.state);
  /* big swimming frame 1 first, then the kick tile */
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0x28u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x1cu] == 0x31u);
  /* frame counter d2 set: no patch */
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x04u;
  smbc.state.ram[SMBC_PLAYER_ANIM_TIMER] = 0x01u;
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x1cu] == 0x2cu);
}

static void grow_shrink_cycles(void) {
  /* smb.asm: HandleChangeSize — growing walks the adder every fourth
   * frame; at ten frames the flag clears */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] = 0x01u;
  player_gfx_handler(&smbc.state); /* frame 0: ctrl 0 → 1 */
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_CTRL] == 0x01u);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0xc0u);
  /* the intermediate grow row: {fc,fc,00,01,4c,4d,4e,4e} */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x04u] == 0xfcu);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x0cu] == 0x00u);
  smbc.state.ram[SMBC_PLAYER_ANIM_CTRL] = 0x09u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x08u;
  player_gfx_handler(&smbc.state); /* ctrl 9 → 10: done */
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_CTRL] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_GFX_OFFSET] == 0xb8u);
}

static void offscreen_rows_park(void) {
  /* smb.asm: PlayerOffscreenChk — the vertical offscreen bits park
   * whole rows, bottom first */
  Smbc smbc;
  standing_big(&smbc);
  smbc.state.ram[SMBC_PLAYER_OFFSCREEN_BITS] = 0x50u; /* rows 1+3 */
  player_gfx_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x1cu] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x14u] == 0x90u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x0cu] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x04u] == 0x80u);
}

static void intermediate_display(void) {
  /* smb.asm: DrawPlayer_Intermediate — small standing at a fixed
   * spot for the lives display */
  Smbc smbc;
  smbc_init(&smbc);
  draw_player_intermediate(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x14u] == 0x3au);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x18u] == 0x37u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x04u] == 0x58u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x04u] == 0x60u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x08u] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 32u] == 0x40u);
}

static void palette_cycle_and_reset(void) {
  /* smb.asm: CyclePlayerPalette / ResetPalStar — the palette bits
   * cycle while the other attribute bits survive */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] = 0x41u;
  cycle_player_palette(&smbc.state, 0x0eu);
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0x42u);
  reset_pal_star(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_SPR_ATTRIB] == 0x40u);
}

int main(void) {
  standing_pose_draws();
  walking_animates();
  skidding_pose();
  jumping_pose();
  injury_blink_skips_odd_frames();
  killed_pose_fixes_two_rows();
  fireball_throw_overlays();
  swim_kick_patches_tile();
  grow_shrink_cycles();
  offscreen_rows_park();
  intermediate_display();
  palette_cycle_and_reset();
  printf("tests/core/playergfx: ok\n");
  return 0;
}
