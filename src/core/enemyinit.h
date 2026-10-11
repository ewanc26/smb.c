/* core/enemyinit.h — the InitEnemyRoutines jump table.
 *
 * Owns: the per-identifier initialisers run when an enemy object is
 * loaded — the shared bounding-box and velocity helpers, the normal
 * enemy group, hammer bro, cheep-cheeps, lakitu, firebars (with
 * DuplicateEnemyObj), platforms large and small (with PosPlatform),
 * bowser, the power-up jump point, the vine setup, and EndFrenzy.
 *
 * Pending their own atom: the frenzy generators — InitEnemyFrenzy's
 * sub-dispatch with LakituAndSpinyHandler, InitFlyingCheepCheep,
 * InitBowserFlame, InitFireworks and BulletBillCheepCheep. Identifiers
 * $12 and $14-$17 currently behave as NoInitCode.
 *
 * Collaborators: core/enemies calls run_init_enemy_routines from
 * CheckpointEnemyID; the block collision code will call setup_vine
 * with a real block buffer offset when it is ported.
 */
#ifndef SMBC_CORE_ENEMYINIT_H
#define SMBC_CORE_ENEMYINIT_H

#include "state.h"

/* smb.asm: InitEnemyRoutines — dispatch on the slot's identifier and
 * run the matching initialiser */
void run_init_enemy_routines(CoreState *state, uint8_t x);

/* smb.asm: Setup_Vine — copy the vine's position from the block that
 * spawned it and claim the next vine slot */
void setup_vine(CoreState *state, uint8_t x, uint8_t block_offset);

#endif /* SMBC_CORE_ENEMYINIT_H */
