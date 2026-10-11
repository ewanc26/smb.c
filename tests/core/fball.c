/* tests/core/fball.c — exercises src/core/fball.c: the fireball and
 * air bubble draw routines. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/fball.h"
#include "../../src/core/frame.h"

static void fireball_sprite(void) {
  /* smb.asm: DrawFireball — position from the relative coordinates;
   * the tile alternates every four frames, the flips every eight */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FBALL_SPR_DATA_OFFSET + 0] = 0x40u;
  smbc.state.ram[SMBC_FIREBALL_REL_XPOS] = 0x60u;
  smbc.state.ram[SMBC_FIREBALL_REL_YPOS] = 0x50u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  draw_fireball(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x40u] == 0x50u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x40u] == 0x60u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x40u] == 0x64u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x40u] == 0x02u);

  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x04u;
  draw_fireball(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x40u] == 0x65u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x40u] == 0x02u);

  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x08u;
  draw_fireball(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x40u] == 0x64u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x40u] == 0xc2u);
}

static void explosion_sprites(void) {
  /* smb.asm: DrawExplosion_Fireball — state $80 starts the run: the
   * state ticks up, four sprites flip outward around the relative
   * position, and the last frame kills the fireball */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ALT_SPR_DATA_OFFSET + 0] = 0x20u;
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x80u;
  smbc.state.ram[SMBC_FIREBALL_REL_XPOS] = 0x30u;
  smbc.state.ram[SMBC_FIREBALL_REL_YPOS] = 0x40u;
  draw_explosion_fireball(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_STATE + 0] == 0x81u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x20u] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x24u] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x28u] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x2cu] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x20u] == 0x3cu);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x28u] == 0x3cu);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x24u] == 0x44u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x2cu] == 0x44u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x20u] == 0x2cu);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x24u] == 0x2cu);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x28u] == 0x34u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x2cu] == 0x34u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x20u] == 0x02u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x24u] == 0x82u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x28u] == 0x42u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x2cu] == 0xc2u);

  /* the tile ages: states $82-$83 use $67, $84-$85 use $66 */
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x82u;
  draw_explosion_fireball(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x20u] == 0x67u);
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x84u;
  draw_explosion_fireball(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x20u] == 0x66u);

  /* smb.asm: KillFireBall — $86 clears the state */
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x86u;
  draw_explosion_fireball(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_STATE + 0] == 0x00u);
}

static void bubble_sprite(void) {
  /* smb.asm: DrawBubble — only while the player is within the screen
   * vertically and the bubble's offscreen d3 is clear */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_BUBBLE_SPR_DATA_OFFSET + 0] = 0x18u;
  smbc.state.ram[SMBC_BUBBLE_REL_XPOS] = 0x66u;
  smbc.state.ram[SMBC_BUBBLE_REL_YPOS] = 0x77u;
  draw_bubble(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x18u] == 0x66u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x18u] == 0x77u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x18u] == 0x74u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x18u] == 0x02u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x02u;
  draw_bubble(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER] == 0x00u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_BUBBLE_OFFSCREEN_BITS] = 0x08u;
  draw_bubble(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER] == 0x00u);
}

/* screen spanning $0020-$0110 for the core's offscreen checks */
static void set_screen(Smbc *smbc) {
  smbc->state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x00u;
  smbc->state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
}

static void fireball_core_init(void) {
  /* smb.asm: FireballObjCore — state $02 initialises from the player:
   * four pixels ahead of the player's position, the facing speed, the
   * small bounding box, then gravity and movement run it */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x60u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u; /* right */
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x00u;
  fireball_obj_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_STATE + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_FIREBALL_X_SPEED + 0] == 0x40u);
  assert(smbc.state.ram[SMBC_FIREBALL_Y_SPEED + 0] == 0x04u);
  assert(smbc.state.ram[SMBC_FIREBALL_BOUND_BOX_CTRL + 0] == 0x07u);
  assert(smbc.state.ram[SMBC_FIREBALL_Y_HIGH_POS + 0] == 0x01u);
  /* position after one frame of movement: 84 + 4 right, 60 + 4 down */
  assert(smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] == 0x88u);
  assert(smbc.state.ram[SMBC_FIREBALL_Y_POSITION + 0] == 0x64u);

  /* facing left: speed $c0 sends it left instead */
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x60u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x02u; /* left */
  fireball_obj_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_X_SPEED + 0] == 0xc0u);
  assert(smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] == 0x80u);
}

static void fireball_core_states(void) {
  /* smb.asm: state $00 leaves everything alone; d7 runs the
   * explosion instead of the object */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] = 0x80u;
  fireball_obj_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] == 0x80u);

  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x80u;
  smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_FIREBALL_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_FIREBALL_X_SPEED + 0] = 0x40u;
  fireball_obj_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_STATE + 0] == 0x81u);
  assert(smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] == 0x80u);
}

static void fireball_core_erases(void) {
  /* smb.asm: EraseFB — offscreen bits under the $cc mask kill the
   * fireball; here past the right edge of the screen */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_FIREBALL_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_FIREBALL_PAGE_LOC + 0] = 0x01u;
  smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] = 0xf0u;
  smbc.state.ram[SMBC_FIREBALL_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_FIREBALL_Y_HIGH_POS + 0] = 0x01u;
  fireball_obj_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_STATE + 0] == 0x00u);
}

static void bubble_creates(void) {
  /* smb.asm: SetupBubble — an offscreen bubble with an expired timer
   * spawns ahead of the facing direction, eight pixels down, the
   * timer from the LSFR bit; MoveBubl then rises it by the borrow */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] = 0xf8u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u; /* right: +8 */
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x40u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x50u;
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x00u;
  bubble_check(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BUBBLE_X_POSITION + 0] == 0x48u);
  assert(smbc.state.ram[SMBC_BUBBLE_PAGE_LOC + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_HIGH_POS + 0] == 0x01u);
  /* LSFR bit 0: timer $40, mforce $ff — dummy 0 borrows, one up */
  assert(smbc.state.ram[SMBC_AIR_BUBBLE_TIMER] == 0x40u);
  assert(smbc.state.ram[SMBC_BUBBLE_YMF_DUMMY + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] == 0x57u);

  /* the timer running leaves an offscreen bubble alone */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] = 0xf8u;
  smbc.state.ram[SMBC_AIR_BUBBLE_TIMER] = 0x10u;
  bubble_check(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] == 0xf8u);
  assert(smbc.state.ram[SMBC_BUBBLE_X_POSITION + 0] == 0x00u);
}

static void bubble_rises(void) {
  /* smb.asm: MoveBubl — no borrow, no movement; above the status bar
   * the bubble goes offscreen */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x01u; /* bit 1 */
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_BUBBLE_YMF_DUMMY + 0] = 0x80u; /* >= $50 */
  bubble_check(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BUBBLE_YMF_DUMMY + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] == 0x60u);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] = 0x20u;
  smbc.state.ram[SMBC_BUBBLE_YMF_DUMMY + 0] = 0x00u; /* borrows */
  bubble_check(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] == 0xf8u);
}

static void proc_spawns_fireball(void) {
  /* smb.asm: ProcFireball_Bubble — fiery mario, a fresh B press and
   * a free slot: the sound, state $02, the throwing timer from the
   * animation setting, and the counter advances */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u;
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_PAD_B;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x60u;
  smbc.state.ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_ANIM_TIMER_SET] = 0x08u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* not water */
  proc_fireball_bubble(&smbc.state);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == SMBC_SFX_FIREBALL);
  /* state $02 ran straight into the core's init: now $01 */
  assert(smbc.state.ram[SMBC_FIREBALL_STATE + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_FIREBALL_THROWING_TIMER] == 0x08u);
  assert(smbc.state.ram[SMBC_PLAYER_ANIM_TIMER] == 0x07u);
  assert(smbc.state.ram[SMBC_FIREBALL_COUNTER] == 0x01u);

  /* B held from the last frame: no spawn */
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u;
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_PAD_B;
  smbc.state.ram[SMBC_PREV_A_B_BUTTONS] = SMBC_PAD_B;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  proc_fireball_bubble(&smbc.state);
  assert(smbc.state.ram[SMBC_FIREBALL_STATE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x00u);

  /* not fiery: no spawn, but a live fireball still runs */
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_A_B_BUTTONS] = SMBC_PAD_B;
  smbc.state.ram[SMBC_FIREBALL_STATE + 1] = 0x01u;
  smbc.state.ram[SMBC_FIREBALL_X_POSITION + 1] = 0x80u;
  smbc.state.ram[SMBC_FIREBALL_Y_POSITION + 1] = 0x60u;
  smbc.state.ram[SMBC_FIREBALL_Y_HIGH_POS + 1] = 0x01u;
  smbc.state.ram[SMBC_FIREBALL_X_SPEED + 1] = 0x40u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  proc_fireball_bubble(&smbc.state);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_FIREBALL_X_POSITION + 1] == 0x84u);
}

static void proc_runs_bubbles_on_water(void) {
  /* smb.asm: ProcAirBubbles — water levels only; all three slots run
   * the check, the position, the bits and the draw */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u; /* water */
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  /* slots 1 and 0 live, slot 2 still offscreen at $00 — its borrow
   * wraps it to $ff; the shared relative pair ends with slot 0's */
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 1] = 0x60u;
  smbc.state.ram[SMBC_BUBBLE_X_POSITION + 1] = 0x80u;
  smbc.state.ram[SMBC_BUBBLE_PAGE_LOC + 1] = 0x00u;
  smbc.state.ram[SMBC_BUBBLE_YMF_DUMMY + 1] = 0x00u; /* borrows: up */
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 0] = 0x60u;
  smbc.state.ram[SMBC_BUBBLE_X_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_BUBBLE_PAGE_LOC + 0] = 0x00u;
  smbc.state.ram[SMBC_BUBBLE_YMF_DUMMY + 0] = 0x00u;
  proc_fireball_bubble(&smbc.state);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 1] == 0x5fu);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 2] == 0xffu);
  assert(smbc.state.ram[SMBC_BUBBLE_REL_YPOS] == 0x5fu); /* slot 0 */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER] == 0x74u);
  assert(smbc.state.ram[SMBC_OBJECT_OFFSET] == 0x00u); /* last slot */

  /* dry land skips the bubbles entirely */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 1] = 0x60u;
  proc_fireball_bubble(&smbc.state);
  assert(smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 1] == 0x60u);
}

int main(void) {
  fireball_sprite();
  explosion_sprites();
  bubble_sprite();
  fireball_core_init();
  fireball_core_states();
  fireball_core_erases();
  bubble_creates();
  bubble_rises();
  proc_spawns_fireball();
  proc_runs_bubbles_on_water();
  puts("fball: ok");
  return 0;
}
