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

int main(void) {
  fireball_sprite();
  explosion_sprites();
  bubble_sprite();
  puts("fball: ok");
  return 0;
}
