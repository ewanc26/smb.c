/* tests/core/blocks.c — exercises src/core/blocks.c: the block object
 * metatile updater and its vram write. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/blocks.h"

static void busy_vram_buffer_skips(void) {
  /* smb.asm: a vram buffer already in use leaves the block object
   * untouched */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_VRAM_BUFFER1] = 0x20u;
  smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_METATILE + 0] = 0x58u;
  block_obj_mt_updater(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_BLOCK_RESIDUAL_COUNTER] == 0x00u);
}

static void clear_flag_skips(void) {
  /* smb.asm: a clear replace flag leaves the buffer untouched */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BLOCK_BBUF_LOW + 0] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_ORIG_Y_POS + 0] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_METATILE + 0] = 0x58u;
  block_obj_mt_updater(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0] == 0x00u);
}

static void brick_with_line_replaces(void) {
  /* smb.asm: the full path — the metatile back into the block buffer,
   * the brick-with-line quad queued for the vram buffer, the offset
   * advanced by ten, the flag cleared */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_BBUF_LOW + 0] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_ORIG_Y_POS + 0] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_METATILE + 0] = 0x58u;
  block_obj_mt_updater(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0] == 0x58u);
  /* smb.asm: two rows of [hi, lo, length, tile, tile] */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 1] == 0x80u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 2] == 0x02u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x45u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 4] == 0x45u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 5] == 0x20u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 6] == 0xa0u); /* lo + $20 */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 7] == 0x02u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 8] == 0x47u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 9] == 0x47u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 10] == 0x00u); /* terminator */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x0au);
  assert(smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_RESIDUAL_COUNTER] == 0x01u);
}

static void odd_page_uses_name_table_one(void) {
  /* smb.asm: a block buffer low byte at $d0 or beyond maps the write
   * to name table 1 */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BLOCK_REP_FLAG + 1] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_BBUF_LOW + 1] = 0xd0u;
  smbc.state.ram[SMBC_BLOCK_ORIG_Y_POS + 1] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_METATILE + 1] = 0x24u; /* any other: empty */
  block_obj_mt_updater(&smbc.state);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 0] == 0x24u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x57u); /* empty block */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 4] == 0x58u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 8] == 0x59u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 9] == 0x5au);
}

static void high_rows_carry_into_the_high_byte(void) {
  /* smb.asm: a low block buffer row doubles the vertical offset into
   * the name table high byte's low bits */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_BBUF_LOW + 0] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_ORIG_Y_POS + 0] = 0x60u;
  smbc.state.ram[SMBC_BLOCK_METATILE + 0] = 0x51u; /* brick with line */
  block_obj_mt_updater(&smbc.state);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0x60] == 0x51u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 0] == 0x22u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 1] == 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] == 0x00u);
}

static void draw_block_basic(void) {
  /* smb.asm: DrawBlock — the two default rows fill four sprites, and
   * a ground-type area keeps the lined brick tiles */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_REL_YPOS] = 0x10u;
  smbc.state.ram[SMBC_BLOCK_REL_XPOS] = 0x20u;
  smbc.state.ram[SMBC_BLOCK_METATILE + 0] = 0x58u;
  draw_block(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0] == 0x85u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 4] == 0x85u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 8] == 0x86u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 12] == 0x86u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0] == 0x03u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 8] == 0x18u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 4] == 0x28u);
}

static void draw_block_other_area_removes_lines(void) {
  /* smb.asm: DrawBlock — outside ground-type areas the two upper tiles
   * become the lineless brick tile */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_REL_YPOS] = 0x10u;
  smbc.state.ram[SMBC_BLOCK_REL_XPOS] = 0x20u;
  draw_block(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0] == 0x86u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 4] == 0x86u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 8] == 0x86u);
}

static void draw_block_used_block_tile(void) {
  /* smb.asm: DrawBlock — the used-block ($c4) variant overrides the
   * tiles with $87 and sets the flipped attribute bytes */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_METATILE + 0] = 0xc4u;
  draw_block(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0] == 0x87u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 4] == 0x87u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 8] == 0x87u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 12] == 0x87u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0] == 0x03u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 4] == 0x43u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 8] == 0x83u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 12] == 0xc3u);
}

static void draw_block_offscreen(void) {
  /* smb.asm: DrawBlock — d2 masks the right column, d3 the left */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_OFFSCREEN_BITS] = 0x0cu;
  draw_block(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 4] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 8] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 12] == 0xf8u);
}

static void draw_brick_chunks_basic(void) {
  /* smb.asm: DrawBrickChunks — palette bits and the brick-chunk tile
   * fill the four sprites, then the original relative position staggers
   * the two X coordinates */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_REL_YPOS] = 0x30u;
  smbc.state.ram[SMBC_BLOCK_REL_XPOS] = 0x30u;
  smbc.state.ram[SMBC_BLOCK_REL_YPOS + 1] = 0x38u;
  smbc.state.ram[SMBC_BLOCK_REL_XPOS + 1] = 0x38u;
  smbc.state.ram[SMBC_BLOCK_ORIG_XPOS + 0] = 0x50u;
  draw_brick_chunks(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0] == 0x84u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 12] == 0x84u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0] == 0x03u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 8] == 0x38u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 4] == 0x37u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 8] == 0x38u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 12] == 0x2eu);
}

static void draw_brick_chunks_end_of_level(void) {
  /* smb.asm: DrawBrickChunks — the end-of-level routine swaps in the
   * residual ball tile and palette */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x05u;
  smbc.state.ram[SMBC_BLOCK_ORIG_XPOS + 0] = 0x10u;
  draw_brick_chunks(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0] == 0x75u);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0] == 0x02u);
}

static void draw_brick_chunks_parks_right_half(void) {
  /* smb.asm: DrawBrickChunks / ChnkOfs — with d7 of the original
   * relative position set, the right half is parked once its X reaches
   * the left half's */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BLOCK_REL_YPOS] = 0x20u;
  smbc.state.ram[SMBC_BLOCK_REL_YPOS + 1] = 0x28u;
  smbc.state.ram[SMBC_BLOCK_REL_XPOS] = 0xf0u;
  smbc.state.ram[SMBC_BLOCK_REL_XPOS + 1] = 0xf0u;
  smbc.state.ram[SMBC_BLOCK_ORIG_XPOS + 0] = 0x90u; /* d7 set */
  draw_brick_chunks(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 4] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 12] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 8] == 0x28u);
}

static void core_asleep_does_nothing(void) {
  /* smb.asm: BlockObjectsCore — a clear state writes zero back */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BLOCK_STATE + 0] = 0x00u;
  block_objects_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x00u);
}

static void core_bouncing_marks_and_kills(void) {
  /* smb.asm: BlockObjectsCore — a bouncing block (state low nybble one)
   * whose low nybble has wrapped marks the metatile and dies */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] = 0x50u;
  block_objects_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_Y_MOVE_FORCE + 0] == 0x50u);
  assert(smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x00u);
}

static void core_bouncing_still_rising_keeps_state(void) {
  /* smb.asm: BlockObjectsCore — a low nybble of five or more is still
   * above the amount, so the state survives */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_STATE + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] = 0x55u;
  block_objects_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_REP_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x01u);
}

static void core_brick_chunks_move_and_survive(void) {
  /* smb.asm: BlockObjectsCore — brick chunks impose gravity on both
   * chunks; while off the bottom the state survives */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_BLOCK_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 2] = 0x58u;
  block_objects_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_Y_MOVE_FORCE + 0] == 0x50u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 10] == 0x50u);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x02u);
}

static void core_brick_chunks_above_screen_keep_state(void) {
  /* smb.asm: BlockObjectsCore — a zero vertical high byte writes the
   * low nybble back unchanged */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_BLOCK_Y_HIGH_POS + 0] = 0x00u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] = 0x50u;
  block_objects_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x02u);
}

static void core_brick_chunks_bottom_kills(void) {
  /* smb.asm: BlockObjectsCore — the top chunk reaching the bottom of
   * the screen kills the object; the bottom chunk is parked first */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_STATE + 0] = 0x02u;
  smbc.state.ram[SMBC_BLOCK_Y_HIGH_POS + 0] = 0x01u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 0] = 0xf0u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 2] = 0xf8u;
  block_objects_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOCK_Y_POSITION + 2] == 0xf0u);
  assert(smbc.state.ram[SMBC_BLOCK_STATE + 0] == 0x00u);
}

int main(void) {
  busy_vram_buffer_skips();
  clear_flag_skips();
  brick_with_line_replaces();
  odd_page_uses_name_table_one();
  high_rows_carry_into_the_high_byte();
  draw_block_basic();
  draw_block_other_area_removes_lines();
  draw_block_used_block_tile();
  draw_block_offscreen();
  draw_brick_chunks_basic();
  draw_brick_chunks_end_of_level();
  draw_brick_chunks_parks_right_half();
  core_asleep_does_nothing();
  core_bouncing_marks_and_kills();
  core_bouncing_still_rising_keeps_state();
  core_brick_chunks_move_and_survive();
  core_brick_chunks_above_screen_keep_state();
  core_brick_chunks_bottom_kills();
  puts("tests/core/blocks: ok");
  return 0;
}
