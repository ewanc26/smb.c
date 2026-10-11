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

/* smb.asm: RelativePlayerPosition — vertical verbatim, horizontal
 * minus the screen's left edge */
void relative_player_position(CoreState *state);

/* smb.asm: RelativeBlockPosition — the two chunks of a block object
 * (spr-object indices 9 + block and 11 + block) written into the
 * block pool's two-byte relative coordinate pair */
void relative_block_position(CoreState *state, uint8_t block);

/* smb.asm: GetBlockOffscreenBits — the combined offscreen bits for a
 * block object, stored in Block_OffscreenBits */
void get_block_offscreen_bits(CoreState *state, uint8_t block);

/* smb.asm: GetEnemyOffscreenBits — write the combined offscreen bits
 * (vertical in the high nybble, horizontal in the low) into
 * Enemy_OffscreenBits */
void get_enemy_offscreen_bits(CoreState *state, uint8_t x);

/* smb.asm: GetPlayerOffscreenBits — horizontal mask in the low nybble,
 * vertical mask in the high nybble */
void get_player_offscreen_bits(CoreState *state);

/* smb.asm: RelativeFireballPosition — the fireball's coordinates
 * relative to the screen, written to the fireball's row of the
 * shared relative coordinates */
void relative_fireball_position(CoreState *state, uint8_t x);

/* smb.asm: RelativeBubblePosition — the air bubble's coordinates
 * relative to the screen, one row past the fireball's */
void relative_bubble_position(CoreState *state, uint8_t x);

/* smb.asm: GetFireballOffscreenBits — the combined offscreen bits for
 * a fireball, stored in FBall_OffscreenBits */
void get_fireball_offscreen_bits(CoreState *state, uint8_t x);

/* smb.asm: GetBubbleOffscreenBits — the combined offscreen bits for
 * an air bubble, stored in Bubble_OffscreenBits */
void get_bubble_offscreen_bits(CoreState *state, uint8_t x);

/* smb.asm: RelativeMiscPosition — ObjOffsetData[2] puts the misc object
 * $0d bytes into the spr-object arrays; the relative pair is row 6. */
void relative_misc_position(CoreState *state, uint8_t x);

/* smb.asm: GetMiscOffscreenBits — the combined bits for a misc object,
 * stored in Misc_OffscreenBits. */
void get_misc_offscreen_bits(CoreState *state, uint8_t x);

/* smb.asm: GetXOffscreenBits — the horizontal offscreen bits for
 * one object, exposed for the platform bound boxes */
/* smb.asm: GetXOffscreenBits — the raw horizontal offscreen mask for
 * a spr-object buffer index (player $00, enemies $01+) */
uint8_t get_x_offscreen_bits(CoreState *state, uint8_t x);

/* smb.asm: PlayerEnemyDiff — the sixteen-bit horizontal distance
 * from the player to the object; the returned value is the high
 * byte, whose sign says which side the object is on */
int8_t player_enemy_diff(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_OFFSCREEN_H */
