/* core/colour.h — the background colour cycle.
 *
 * Owns: ColorRotation, the per-frame background palette cycle. Every
 * eighth frame it appends a palette-3 update set to VRAM buffer 1: the
 * area's palette is patched in from data/palettes, one slot advancing
 * through ColorRotatePalette each cycle. The buffer offset grows by
 * seven and the cycle offset wraps at six. Called from the game engine.
 */
#ifndef SMBC_CORE_COLOUR_H
#define SMBC_CORE_COLOUR_H

#include "state.h"

/* smb.asm: ColorRotation — cycle one background colour, every eighth
 * frame, while VRAM buffer 1 has room (offset below $31). */
void color_rotation(CoreState *state);

#endif /* SMBC_CORE_COLOUR_H */
