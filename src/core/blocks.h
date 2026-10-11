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

#endif /* SMBC_CORE_BLOCKS_H */
