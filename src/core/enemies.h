/* core/enemies.h — the enemy data interpreter.
 *
 * Owns: EnemiesAndLoopsCore and everything it reaches when an enemy
 * slot is idle — ProcLoopCommand (with ExecGameLoopback and
 * KillAllEnemies), the frenzy queue check, ProcessEnemyData with its
 * page control and right-boundary logic, HandleGroupEnemies,
 * CheckFrenzyBuffer, InitEnemyObject and CheckpointEnemyID.
 *
 * Pending its own atom: RunEnemyObjectsCore (the per-frame runners,
 * jumped to when a slot's flag is already set). The InitEnemyRoutines
 * jump table lives in core/enemyinit.
 *
 * Collaborators: the game engine and victory mode will call
 * enemies_and_loops_core once per enemy slot; data/enemies supplies the
 * loop command tables; data/areadata supplies the enemy object stream
 * the interpreter walks.
 */
#ifndef SMBC_CORE_ENEMIES_H
#define SMBC_CORE_ENEMIES_H

#include "state.h"

/* smb.asm's enemy object constants, the ids this atom reads */
#define SMBC_GREEN_KOOPA 0x00u   /* smb.asm: GreenKoopa */
#define SMBC_BUZZY_BEETLE 0x02u  /* smb.asm: BuzzyBeetle */
#define SMBC_GOOMBA 0x06u        /* smb.asm: Goomba */
#define SMBC_VINE_OBJECT 0x2fu   /* smb.asm: VineObject */

/* smb.asm: EnemiesAndLoopsCore — one enemy slot's per-frame entry. An
 * active slot jumps to the (pending) enemy runners; an idle slot checks
 * the parser task, processes loop commands and loads new enemies from
 * the data stream. */
void enemies_and_loops_core(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_ENEMIES_H */
