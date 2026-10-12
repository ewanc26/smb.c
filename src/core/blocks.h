/* core/blocks.h — the block object metatile updater: writes replaced
 * metatiles back to the block buffer and the vram buffer. */
#ifndef SMBC_CORE_BLOCKS_H
#define SMBC_CORE_BLOCKS_H

#include "state.h"

/* smb.asm: BlockObjMT_Updater — for each block object with a replace
 * flag and a free vram buffer, write its metatile to the block buffer
 * and queue the vram replacement */
void block_obj_mt_updater(CoreState *state);

/* smb.asm: ReplaceBlockMetatile — queue the metatile replacement and
 * run the residual counter/flag bookkeeping */
void replace_block_metatile(CoreState *state, uint8_t x);

/* smb.asm: BlockObjectsCore — one block object's per-frame update:
 * gravity and horizontal movement for its chunk(s), relative position,
 * offscreen bits, drawing, and the bottom-of-screen / replace-flag
 * state handling. block is the object offset (0 or 1) */
void block_objects_core(CoreState *state, uint8_t block);

/* smb.asm: DrawBlock — the four sprites of a bouncing block */
void draw_block(CoreState *state, uint8_t block);

/* smb.asm: DrawBrickChunks — the four sprites of a shattered brick's
 * flying chunks */
void draw_brick_chunks(CoreState *state, uint8_t block);

/* smb.asm: InitBlock_XY_Pos — seed a block object's position from the
 * player's, snapped to the 16-pixel grid */
void init_block_xy_pos(CoreState *state, uint8_t x);

/* smb.asm: DestroyBlockMetatile — queue the blank metatile write for
 * the spot the block-buffer collision scratch ($06/$02) points at */
void destroy_block_metatile(CoreState *state);

/* smb.asm: RemoveCoin_Axe — erase the touched coin or axe metatile
 * from the screen and set the vram address controller to $0341 */
void remove_coin_axe(CoreState *state);

#endif /* SMBC_CORE_BLOCKS_H */
