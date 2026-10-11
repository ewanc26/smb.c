/* src/core/collide.h — smb.asm's bounding box and collision layer:
 * the enemy bounding box computation and the collision detectors
 * built on it. */
#ifndef SMBC_CORE_COLLIDE_H
#define SMBC_CORE_COLLIDE_H

#include <stdint.h>

#include "state.h"

/* smb.asm: GetEnemyBoundBox — masks the offscreen bits against the
 * screen-relative position, parks the bounding box offscreen when
 * anything is masked, and otherwise computes the corners from the
 * relative position and the bound box control data, fixing up
 * coordinates that crossed a screen edge */
void get_enemy_bound_box(CoreState *state, uint8_t x);

/* smb.asm: GetEnemyBoundBoxOfs / GetEnemyBoundBoxOfsArg — the enemy
 * bounding box offset (slot * 4 + 4, past the player's) and the
 * low offscreen nybble, with a flag for all four bits set */
uint8_t get_enemy_bound_box_ofs(CoreState *state, uint8_t slot,
                                uint8_t *all_set);

/* smb.asm: ChkToStunEnemies — the shared stun setup for defeated
 * enemies: ids $09, $0e, $0f and $10 demote to plain koopa
 * (the LSB), the state takes d1 under the preserved high nybble,
 * the enemy lifts two pixels and gets the water-dependent
 * vertical speed, and the horizontal speed and direction face the
 * player — bullet bills keep their direction. A arrives as the
 * caller's accumulator: the asm's piranha-plant caller passes the
 * adjusted Y coordinate, which always fails the demote tests */
void chk_to_stun_enemies(CoreState *state, uint8_t x, uint8_t a);

#endif
