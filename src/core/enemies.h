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
#define SMBC_RED_KOOPA 0x03u
#define SMBC_GREEN_KOOPA 0x00u   /* smb.asm: GreenKoopa */
#define SMBC_BUZZY_BEETLE 0x02u  /* smb.asm: BuzzyBeetle */
#define SMBC_GOOMBA 0x06u        /* smb.asm: Goomba */
#define SMBC_VINE_OBJECT 0x2fu   /* smb.asm: VineObject */
#define SMBC_LAKITU 0x11u        /* smb.asm: Lakitu */
#define SMBC_JUMPSPRING_OBJECT 0x32u /* smb.asm: JumpspringObject */
#define SMBC_RETAINER_OBJECT 0x35u   /* smb.asm: RetainerObject */
#define SMBC_SPINY 0x12u         /* smb.asm: Spiny */
#define SMBC_HAMMER_BRO 0x05u    /* smb.asm: HammerBro */
#define SMBC_BLOOBER 0x07u       /* smb.asm: Bloober */
#define SMBC_BULLET_BILL_FRENZY 0x08u   /* smb.asm: BulletBill_FrenzyVar */
#define SMBC_PIRANHA_PLANT 0x0du /* smb.asm: PiranhaPlant */
#define SMBC_GREEN_PARATROOPA_JUMP 0x0eu /* smb.asm: GreenParatroopaJump */
#define SMBC_BULLET_BILL_CANNON 0x33u  /* smb.asm: BulletBill_CannonVar */
#define SMBC_PODOBOO 0x0cu       /* smb.asm: Podoboo */
#define SMBC_FLYING_CHEEP_CHEEP 0x14u /* smb.asm: FlyingCheepCheep */
#define SMBC_POWER_UP_OBJECT 0x2eu  /* smb.asm: PowerUpObject */

/* smb.asm: EnemiesAndLoopsCore — one enemy slot's per-frame entry. An
 * active slot jumps to the (pending) enemy runners; an idle slot checks
 * the parser task, processes loop commands and loads new enemies from
 * the data stream. */
void enemies_and_loops_core(CoreState *state, uint8_t x);

/* smb.asm: CheckpointEnemyID — identifiers below $15 get eight pixels
 * added to their vertical coordinate and the offscreen masked bit set,
 * then the init jump engine runs. Exported for the frenzy generators
 * in core/enemyinit, whose bullet bill / cheep-cheep path jumps here */
void checkpoint_enemy_id(CoreState *state, uint8_t x);

/* smb.asm: EraseEnemyObject — clear all of a slot's variables */
void erase_enemy_object(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_ENEMIES_H */
