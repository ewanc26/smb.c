/* core/move.h — the object movement primitives.
 *
 * smb.asm's physics leaves: MoveObjectHorizontally (reached through
 * MoveEnemyHorizontally) and the ImposeGravity family with its entry
 * points for enemies, platforms and the red paratroopa. The asm
 * addresses the generic SprObject arrays with the object offset
 * incremented by one; the port uses the enemy arrays directly, which
 * is the same memory. The player-side wrappers (MovePlayerHorizontally
 * and MovePlayerVertically) belong to the player atom. */
#ifndef SMBC_CORE_MOVE_H
#define SMBC_CORE_MOVE_H

#include <stdint.h>

#include "state.h"

/* smb.asm: MoveEnemyHorizontally — move an enemy object horizontally
 * according to its speed and movement force counters. Returns the
 * value the asm leaves in A (the whole-pixel adder plus the fractional
 * carry). */
uint8_t move_enemy_horizontally(CoreState *state, uint8_t x);

/* smb.asm: MoveD_EnemyVertically — move an enemy object downwards.
 * The spiny's egg state ($05) falls at the falling platform's amount */
void move_d_enemy_vertically(CoreState *state, uint8_t x);

/* smb.asm: MoveFallingPlatform */
void move_falling_platform(CoreState *state, uint8_t x);

/* smb.asm: MoveJ_EnemyVertically — impose gravity with the jumping
 * object's movement amount (podoboo, green paratroopa) */
void move_j_enemy_vertically(CoreState *state, uint8_t x);

/* smb.asm: MoveDropPlatform */
void move_drop_platform(CoreState *state, uint8_t x);

/* smb.asm: MoveEnemySlowVert — the slow vertical movement used by
 * bowser and other objects */
void move_enemy_slow_vert(CoreState *state, uint8_t x);

/* smb.asm: MoveRedPTroopaDown / MoveRedPTroopaUp — the red
 * paratroopa's vertical gravity, dir zero for down, one for up */
void move_red_p_troopa(CoreState *state, uint8_t x, uint8_t dir);

/* smb.asm: SetXMoveAmt — the generic gravity entry used by movers
 * that pick their own downward force and maximum speed */
void set_x_move_amt(CoreState *state, uint8_t x, uint8_t down, uint8_t max);

/* smb.asm: MovePlatformDown / MovePlatformUp — platform vertical
 * gravity, dir zero for down, one for up */
void move_platform_vert(CoreState *state, uint8_t x, uint8_t dir);

/* smb.asm: ImposeGravityBlock — gravity for a block object chunk, with
 * the block pool's downward amount ($50) and maximum speed ($08). x is
 * the object offset in the enemy arrays (chunk A of block b is 8 + b,
 * chunk B is 10 + b) */
void impose_gravity_block(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_MOVE_H */
