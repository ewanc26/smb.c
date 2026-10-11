/* core/miscobj.h — misc objects: the hammer bro's hammers, jumping
 * coins and their floatey numbers.
 *
 * Owns: the per-frame MiscObjectsCore pass over the nine misc slots —
 * the jumping coin's rise and timer, the hammer's spawn positioning
 * and gravity, the hammer's collision with the player, and both
 * objects' draws.
 */
#ifndef SMBC_CORE_MISCOBJ_H
#define SMBC_CORE_MISCOBJ_H

#include "state.h"

/* smb.asm: MiscObjectsCore — slots $08 down to $00, run from
 * GameEngine between BlockObjectsCore and ProcessCannons. */
void misc_objects_core(CoreState *state);

#endif /* SMBC_CORE_MISCOBJ_H */
