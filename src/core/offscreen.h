/* core/offscreen.h — relative position and offscreen bits.
 *
 * smb.asm's position layer: RelativeEnemyPosition (through
 * VariableObjOfsRelPos and GetObjRelativePosition) and
 * GetEnemyOffscreenBits (through SetOffscrBitsOffset,
 * GetOffScreenBitsSet, GetXOffscreenBits, GetYOffscreenBits and
 * DividePDiff). As with the movement primitives the asm works on the
 * generic SprObject arrays with the offset incremented; the port
 * addresses the enemy arrays directly. The player, fireball, bubble,
 * misc and block variants belong to their own atoms. */
#ifndef SMBC_CORE_OFFSCREEN_H
#define SMBC_CORE_OFFSCREEN_H

#include <stdint.h>

#include "state.h"

/* smb.asm: RelativeEnemyPosition — write the object's coordinates
 * relative to the screen into Enemy_Rel_YPos / Enemy_Rel_XPos */
void relative_enemy_position(CoreState *state, uint8_t x);

/* smb.asm: GetEnemyOffscreenBits — write the combined offscreen bits
 * (vertical in the high nybble, horizontal in the low) into
 * Enemy_OffscreenBits */
void get_enemy_offscreen_bits(CoreState *state, uint8_t x);

/* smb.asm: PlayerEnemyDiff — the sixteen-bit horizontal distance
 * from the player to the object; the returned value is the high
 * byte, whose sign says which side the object is on */
int8_t player_enemy_diff(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_OFFSCREEN_H */
