/* tests/core/offscreen.c — exercises src/core/offscreen.c: relative
 * position and the offscreen bits. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/offscreen.h"

/* screen spanning $0020-$0110 for the horizontal checks */
static void set_screen(Smbc *smbc) {
  smbc->state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x00u;
  smbc->state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
}

static void relative_position(void) {
  /* smb.asm: GetObjRelativePosition — Y verbatim, X minus the left
   * edge, wrapping on a borrow */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x30u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x90u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x55u;
  relative_enemy_position(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_REL_XPOS] == 0x60u);
  assert(smbc.state.ram[SMBC_ENEMY_REL_YPOS] == 0x55u);

  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u;
  relative_enemy_position(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_REL_XPOS] == 0xe0u);
}

static void x_bits_mid_and_right(void) {
  /* smb.asm: GetXOffscreenBits — a mid-screen object has no mask; an
   * object eight pixels from the right edge divides the difference
   * by eight for the table offset; past the edge the fallback offset
   * gives the full mask */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] == 0x00u);

  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x08u; /* $110 - $108 = 8 */
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0fu) == 0x03u);

  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0xf0u; /* beyond the edge */
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0fu) == 0x0fu);
}

static void x_bits_left(void) {
  /* smb.asm: GetXOffscreenBits — the left side adds eight to the
   * table offset, reaching the second half of XOffscreenBitsData */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x10u; /* $20 - $10 = $10 */
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0fu) == 0x0eu);

  /* a page or more to the left: the fallback offset, full mask */
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x00u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x02u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x03u;
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0fu) == 0x0fu);
}

static void y_bits(void) {
  /* smb.asm: GetYOffscreenBits — above the top edge the difference
   * from $100 divides by eight; far above, the full mask. Below the
   * bottom edge the adder of four reaches the table's second half */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x80u;

  /* just above the top: $100 - $f0 = $10, offset two */
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0xf0u;
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] == 0xc0u);

  /* a full unit above: the fallback offset, full mask */
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x00u;
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] == 0xf0u);

  /* near the bottom edge: $1ff - $1e8 = $17, offset six */
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0xe8u;
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] == 0x30u);

  /* below the screen entirely: both checks fall to the full mask */
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x00u;
  get_enemy_offscreen_bits(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] == 0xf0u);
}


static void player_bits(void) {
  /* smb.asm: GetPlayerOffscreenBits — the same pair on the player's
   * buffer index, stored in the player's offscreen bits; an onscreen
   * player reads zero, one past the right edge the full horizontal
   * mask */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x00u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  get_player_offscreen_bits(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_OFFSCREEN_BITS] == 0x00u);

  smbc.state.ram[SMBC_PLAYER_PAGE_LOC] = 0x01u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0xf0u; /* beyond the edge */
  get_player_offscreen_bits(&smbc.state);
  assert((smbc.state.ram[SMBC_PLAYER_OFFSCREEN_BITS] & 0x0fu) == 0x0fu);
  /* the enemy offscreen bits are untouched */
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCREEN_BITS] == 0x00u);
}


static void relative_block(void) {
  /* smb.asm: RelativeBlockPosition — the two chunks (Block arrays at
   * +block and +block+2) written into the block pool's relative pair */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc.state.ram[SMBC_BLOCK_X_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_BLOCK_X_POSITION + 2] = 0x90u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] = 0x30u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 2] = 0x38u;
  relative_block_position(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_REL_XPOS] == 0x60u);
  assert(smbc.state.ram[SMBC_BLOCK_REL_XPOS + 1] == 0x70u);
  assert(smbc.state.ram[SMBC_BLOCK_REL_YPOS] == 0x30u);
  assert(smbc.state.ram[SMBC_BLOCK_REL_YPOS + 1] == 0x38u);

  /* the second block object reads the odd chunk indices */
  smbc.state.ram[SMBC_BLOCK_X_POSITION + 1] = 0x50u;
  smbc.state.ram[SMBC_BLOCK_X_POSITION + 3] = 0x60u;
  relative_block_position(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_BLOCK_REL_XPOS] == 0x30u);
  assert(smbc.state.ram[SMBC_BLOCK_REL_XPOS + 1] == 0x40u);
}

static void block_bits(void) {
  /* smb.asm: GetBlockOffscreenBits — the block object's buffer index
   * (Block arrays) with the combined bits in Block_OffscreenBits */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_BLOCK_PAGE_LOC + 0] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_X_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_BLOCK_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] = 0x40u;
  get_block_offscreen_bits(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_OFFSCREEN_BITS] == 0x00u);

  smbc.state.ram[SMBC_BLOCK_PAGE_LOC + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_X_POSITION + 0] = 0xf0u; /* beyond the edge */
  get_block_offscreen_bits(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_BLOCK_OFFSCREEN_BITS] & 0x0fu) == 0x0fu);
}

static void relative_player(void) {
  /* smb.asm: RelativePlayerPosition — Y verbatim, X minus the left
   * edge, wrapping on a borrow */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x40u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  relative_player_position(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_REL_YPOS] == 0x40u);
  assert(smbc.state.ram[SMBC_PLAYER_REL_XPOS] == 0x60u);

  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x10u; /* left of the edge */
  relative_player_position(&smbc.state);
  assert(smbc.state.ram[SMBC_PLAYER_REL_XPOS] == 0xf0u);
}

static void relative_fireball(void) {
  /* smb.asm: RelativeFireballPosition — the fireball arrays with the
   * relative pair at the fireball's row of the shared coordinates */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_FIREBALL_Y_POSITION + 1] = 0x50u;
  smbc.state.ram[SMBC_FIREBALL_X_POSITION + 1] = 0xa0u;
  relative_fireball_position(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_FIREBALL_REL_YPOS] == 0x50u);
  assert(smbc.state.ram[SMBC_FIREBALL_REL_XPOS] == 0x80u);
  /* the bubble's row is untouched */
  assert(smbc.state.ram[SMBC_BUBBLE_REL_XPOS] == 0x00u);
}

static void relative_bubble(void) {
  /* smb.asm: RelativeBubblePosition — the bubble arrays with the
   * relative pair one row past the fireball's */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 2] = 0x60u;
  smbc.state.ram[SMBC_BUBBLE_X_POSITION + 2] = 0x40u;
  relative_bubble_position(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_BUBBLE_REL_YPOS] == 0x60u);
  assert(smbc.state.ram[SMBC_BUBBLE_REL_XPOS] == 0x20u);
  /* the fireball's row is untouched */
  assert(smbc.state.ram[SMBC_FIREBALL_REL_XPOS] == 0x00u);
}

static void fireball_bits(void) {
  /* smb.asm: GetFireballOffscreenBits — the fireball's spr-object
   * index (seven bytes in) with the combined bits in
   * FBall_OffscreenBits */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_FIREBALL_PAGE_LOC + 0] = 0x00u;
  smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_FIREBALL_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_FIREBALL_Y_POSITION + 0] = 0x40u;
  get_fireball_offscreen_bits(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FBALL_OFFSCREEN_BITS] == 0x00u);

  smbc.state.ram[SMBC_FIREBALL_PAGE_LOC + 0] = 0x01u;
  smbc.state.ram[SMBC_FIREBALL_X_POSITION + 0] = 0xf0u; /* past the edge */
  get_fireball_offscreen_bits(&smbc.state, 0x00u);
  assert((smbc.state.ram[SMBC_FBALL_OFFSCREEN_BITS] & 0x0fu) == 0x0fu);
  /* the bubble's offscreen bits are untouched */
  assert(smbc.state.ram[SMBC_BUBBLE_OFFSCREEN_BITS] == 0x00u);
}

static void bubble_bits(void) {
  /* smb.asm: GetBubbleOffscreenBits — the bubble's spr-object index
   * ($16 bytes in) with the combined bits in Bubble_OffscreenBits */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_BUBBLE_PAGE_LOC + 2] = 0x00u;
  smbc.state.ram[SMBC_BUBBLE_X_POSITION + 2] = 0x80u;
  smbc.state.ram[SMBC_BUBBLE_Y_HIGH_POS + 2] = 0x01u;
  smbc.state.ram[SMBC_BUBBLE_Y_POSITION + 2] = 0x40u;
  get_bubble_offscreen_bits(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_BUBBLE_OFFSCREEN_BITS] == 0x00u);

  smbc.state.ram[SMBC_BUBBLE_PAGE_LOC + 2] = 0x01u;
  smbc.state.ram[SMBC_BUBBLE_X_POSITION + 2] = 0xf0u; /* past the edge */
  get_bubble_offscreen_bits(&smbc.state, 0x02u);
  assert((smbc.state.ram[SMBC_BUBBLE_OFFSCREEN_BITS] & 0x0fu) == 0x0fu);
  /* the fireball's offscreen bits are untouched */
  assert(smbc.state.ram[SMBC_FBALL_OFFSCREEN_BITS] == 0x00u);
}

int main(void) {
  relative_position();
  x_bits_mid_and_right();
  x_bits_left();
  y_bits();
  player_bits();
  relative_player();
  relative_block();
  block_bits();
  relative_fireball();
  relative_bubble();
  fireball_bits();
  bubble_bits();
  puts("tests/core/offscreen: ok");
  return 0;
}
