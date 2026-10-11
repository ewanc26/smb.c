/* core/cannon.h — the bullet bill cannons.
 *
 * Owns: ProcessCannons, the per-frame check that fires a bullet bill
 * from any cannon within range across the first three enemy slots, and
 * BulletBillHandler, the handler cannon-fired bullet bills fall into
 * (movement, the too-close check, collision and drawing).
 *
 * Collaborators: the enemy object slots in core/state; the movement
 * (core/move), offscreen (core/offscreen), bounding-box and collision
 * (core/collide) and enemy erasure (core/enemies) helpers; and the
 * cannon tables in data/cannon. It is not on the RunEnemyObjectsCore
 * jump table — the game engine calls process_cannons directly.
 */
#ifndef SMBC_CORE_CANNON_H
#define SMBC_CORE_CANNON_H

#include "state.h"

/* smb.asm: ProcessCannons — walk the first three enemy slots, firing a
 * cannon when its timer has expired, and run the cannon-fired bullet
 * bills found in those slots */
void process_cannons(CoreState *state);

/* smb.asm: BulletBillHandler — handle one cannon-fired bullet bill in
 * slot x */
void bullet_bill_handler(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_CANNON_H */
