/* core/runners.h — the per-frame enemy object runners.
 *
 * smb.asm's RunEnemyObjectsCore jump table and its runner routines.
 * This atom starts with OffscreenBoundsCheck, the shared erasure
 * guard every runner tails into; the runners themselves and the
 * dispatch follow. */
#ifndef SMBC_CORE_RUNNERS_H
#define SMBC_CORE_RUNNERS_H

#include <stdint.h>

#include "state.h"

/* smb.asm: OffscreenBoundsCheck — erase the object when it strays
 * seventy-two pixels (fifty-six less for hammer bros and piranha
 * plants) beyond the left screen edge, or seventy-two beyond the
 * right edge, with exemptions for the spiny's egg state and several
 * objects, and never for the flying cheep-cheep */
void offscreen_bounds_check(CoreState *state, uint8_t x);

/* smb.asm: MoveNormalEnemy — the mover for most walker enemies:
 * gravity and horizontal movement by state, slowed movement for
 * d6-state objects, defeated enemies falling away, and stunned
 * enemies reviving on their interval timer (goombas are erased
 * instead when the timer reaches $0e) */
void move_normal_enemy(CoreState *state, uint8_t x);

/* smb.asm: MoveJumpingEnemy — gravity with the jumping amount, then
 * horizontal movement (green paratroopas and the like) */
void move_jumping_enemy(CoreState *state, uint8_t x);

/* smb.asm: ProcMoveRedPTroopa — the flying red paratroopa: nudged up
 * one pixel every eighth frame while at rest above its origin,
 * otherwise moved down or up towards its central vertical
 * coordinate */
void proc_move_red_p_troopa(CoreState *state, uint8_t x);

/* smb.asm: MoveFlyGreenPTroopa — the flying green paratroopa: the
 * primary/secondary movement counters ramp and reflect the X speed
 * for its sine-like horizontal flight (XMoveCntr_GreenPTroopa,
 * MoveWithXMCntrs), and every fourth frame it sways one pixel down
 * or up by the frame counter's d6 */
void move_fly_green_p_troopa(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_RUNNERS_H */
