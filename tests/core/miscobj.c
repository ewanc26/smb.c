/* tests/core/miscobj.c — exercises src/core/miscobj.c: the jumping
 * coin and floatey number, the hammer's spawn, flight, collision and
 * draw, and the MiscObjectsCore pass. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/miscobj.h"
#include "../../src/core/frame.h"
#include "../../src/core/initialize.h"
#include "../../src/core/state.h"

/* screen spanning $0020-$0110 for the offscreen/relative calls */
static void set_screen(Smbc *smbc) {
  smbc->state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x00u;
  smbc->state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
}

static void empty_slots_do_nothing(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_OBJECT_OFFSET] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER] == 0x00u);
}

static void coin_rises_under_gravity(void) {
  /* smb.asm: JCoinRun — the first gravity frame banks the downward
   * force without moving the coin */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x60u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x80u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x01u);
  assert(smbc.state.ram[SMBC_MISC_Y_POSITION + 8] == 0x60u);
  assert(smbc.state.ram[SMBC_SPROBJ_Y_MOVE_FORCE + 8 + 0x0d] == 0x50u);
  /* the draw ran: coin tiles at the relative coordinate */
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x30u] == 0x60u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x30u] == 0x60u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x34u] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x30u] == 0x02u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x34u] == 0x82u);
}

static void coin_tile_cycles(void) {
  /* smb.asm: JCoinGfxHandler — the tile walks JumpingCoinTiles every
   * other frame */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x60u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x80u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x06u; /* (6>>1)&3 = 3 */
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x30u] == 0x63u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x34u] == 0x63u);
}

static void coin_becomes_floatey(void) {
  /* smb.asm: the coin moving down at speed 5 flips to the floatey
   * number state */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x60u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x80u;
  smbc.state.ram[SMBC_MISC_Y_SPEED + 8] = 0x05u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x02u);
}

static void floatey_drifts_and_expires(void) {
  /* smb.asm: state 2 and up counts as a timer, drifting with the
   * scroll; $30 nullifies the object and skips the subroutines */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x02u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x50u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x80u;
  smbc.state.ram[SMBC_SCROLL_AMOUNT] = 0x02u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x03u);
  assert(smbc.state.ram[SMBC_MISC_X_POSITION + 8] == 0x82u);
  /* DrawFloateyNumber_Coin: "200" tiles; frame 0 is even, so the
   * number rises a pixel before the dump */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x30u] == 0xf7u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x34u] == 0xfbu);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x30u] == 0x4fu);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x34u] == 0x4fu);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x34u] == 0x6au);

  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x2fu;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x50u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x80u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x00u);
  assert(smbc.state.ram[SMBC_MISC_REL_YPOS] == 0x00u); /* subs skipped */
}

static void hammer_leaves_the_bro(void) {
  /* smb.asm: SetHSpd/SetHPos — state $82 gives the hammer the
   * direction's speed and tracks it two pixels right, ten above the
   * bro, dropping to $81 */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x82u;
  smbc.state.ram[SMBC_HAMMER_ENEMY_OFFSET + 8] = 0x03u;
  smbc.state.ram[SMBC_ENEMY_STATE + 3] = 0x08u;
  smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 3] = 0x01u; /* right */
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 3] = 0x80u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 3] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 3] = 0x60u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_Y_SPEED + 8] == 0xfeu);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 3] == 0x00u);
  assert(smbc.state.ram[SMBC_MISC_X_SPEED + 8] == 0x10u);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x81u);
  assert(smbc.state.ram[SMBC_MISC_X_POSITION + 8] == 0x82u);
  assert(smbc.state.ram[SMBC_MISC_Y_POSITION + 8] == 0x56u);
  assert(smbc.state.ram[SMBC_MISC_Y_HIGH_POS + 8] == 0x01u);
  /* DrawHammer, pose 0 at the relative coordinate */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x30u] == 0x80u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x34u] == 0x81u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x30u] == 0x03u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x30u] == 0x66u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x34u] == 0x66u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x30u] == 0x56u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x34u] == 0x5eu);
}

static void hammer_flying_pose_cycles(void) {
  /* smb.asm: RenderH — state $81 picks the pose from the frame
   * counter every four frames */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x81u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x60u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x80u;
  smbc.state.ram[SMBC_MISC_Y_HIGH_POS + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x0cu; /* (12>>2)&3 = 3 */
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x30u] == 0x83u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x34u] == 0x82u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x30u] == 0xc3u);
}

static void hammer_offscreen_dies(void) {
  /* smb.asm: DrawHammer — any offscreen bits outside d1-d0 nullify
   * the object and park its sprites */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x81u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x60u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0xf0u;
  smbc.state.ram[SMBC_MISC_PAGE_LOC + 8] = 0x01u; /* past the edge */
  smbc.state.ram[SMBC_MISC_Y_HIGH_POS + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_STATE + 8] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x30u] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x34u] == 0xf8u);
}

static void hammer_hits_player(void) {
  /* smb.asm: PlayerHammerCollision — an odd frame, no timer, no
   * offscreen bits, overlapping boxes: the collision flag sets, the
   * hammer bounces back, and without star the player is injured */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x81u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x50u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x70u; /* rel x $50 */
  smbc.state.ram[SMBC_MISC_Y_HIGH_POS + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_X_SPEED + 8] = 0x10u;
  smbc.state.ram[SMBC_MISC_BOUND_BOX_CTRL + 8] = 0x01u;
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_REL_YPOS] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL] = 0x01u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x01u; /* odd: check runs */
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_COLLISION_FLAG + 8] == 0x01u);
  assert(smbc.state.ram[SMBC_MISC_X_SPEED + 8] == 0xf0u);
  /* smb.asm: InjurePlayer via ForceInjury — small player, so this is
   * KillPlayer: death music and the death subroutine */
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0xffu);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0bu);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_QUEUE] == 0x01u);
}

static void even_frames_skip_collision(void) {
  /* smb.asm: d0 of the frame counter clear skips the whole check */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_MISC_STATE + 8] = 0x81u;
  smbc.state.ram[SMBC_MISC_Y_POSITION + 8] = 0x50u;
  smbc.state.ram[SMBC_MISC_X_POSITION + 8] = 0x70u;
  smbc.state.ram[SMBC_MISC_Y_HIGH_POS + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_X_SPEED + 8] = 0x10u;
  smbc.state.ram[SMBC_SPROBJ_REL_XPOS] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_REL_YPOS] = 0x50u;
  smbc.state.ram[SMBC_SPROBJ_BOUND_BOX_CTRL] = 0x01u;
  smbc.state.ram[SMBC_MISC_BOUND_BOX_CTRL + 8] = 0x01u;
  smbc.state.ram[SMBC_MISC_SPR_DATA_OFFSET + 8] = 0x30u;
  misc_objects_core(&smbc.state);
  assert(smbc.state.ram[SMBC_MISC_COLLISION_FLAG + 8] == 0x00u);
  assert(smbc.state.ram[SMBC_MISC_X_SPEED + 8] == 0x10u);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0x00u);
}

int main(void) {
  empty_slots_do_nothing();
  coin_rises_under_gravity();
  coin_tile_cycles();
  coin_becomes_floatey();
  floatey_drifts_and_expires();
  hammer_leaves_the_bro();
  hammer_flying_pose_cycles();
  hammer_offscreen_dies();
  hammer_hits_player();
  even_frames_skip_collision();
  printf("tests/core/miscobj: ok\n");
  return 0;
}
