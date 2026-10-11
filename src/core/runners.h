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

/* smb.asm: MoveBloober — the bloober: defeated ones sink slowly;
 * otherwise the LSFR (masked by secondary hard mode) occasionally
 * picks a moving direction, ProcSwimmingB runs the swim cycle of
 * speeding up, slowing down and floating down towards the player,
 * and the movement force bobs the bloober up while its speed swims
 * it sideways (BlooperMoveSpeed aliases the X speed at $58,
 * BlooperMoveCounter the primary movement counter at $a0) */
void move_bloober(CoreState *state, uint8_t x);

/* smb.asm: MoveBulletBill — the frenzy bullet bill: defeated ones
 * drop with jumping gravity; otherwise a fixed $e8 leftward speed
 * (the cannon-fired bills move elsewhere) */
void move_bullet_bill(CoreState *state, uint8_t x);

/* smb.asm: MoveSwimmingCheepCheep — the underwater cheep-cheeps:
 * defeated ones sink slowly; otherwise a per-identifier horizontal
 * force drain drifts them left, and slots beyond the first two
 * oscillate vertically around their origin, the movement flag
 * flipping direction fifteen pixels out (CheepCheepMoveMFlag
 * aliases the X speed at $58) */
void move_swimming_cheep_cheep(CoreState *state, uint8_t x);

/* smb.asm: MovePodoboo — when the interval timer expires the
 * podoboo is reinitialised below the screen with a randomised
 * movement force and timer and a $f9 upward speed; gravity runs
 * every frame */
void move_podoboo(CoreState *state, uint8_t x);

/* smb.asm: ProcHammerBro — defeated bros fall; otherwise the jump
 * timer counts down, an on-screen bro with an expired throwing
 * timer spawns a hammer (d3 of the state marks the throw) and the
 * jump code picks a speed by screen position and LSFR, the length
 * timer by hard mode and more random bits. Every frame the bro
 * shimmies by the frame counter's d6, faces the player, and walks
 * left at them when the walking timer expires */
void proc_hammer_bro(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_RUNNERS_H */
