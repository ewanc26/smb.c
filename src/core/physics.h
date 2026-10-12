/* src/core/physics.h — the player's movement physics and background
 * collision. smb.asm: PlayerMovementSubs (with the four state
 * subs), PlayerPhysicsSub (with X_Physics, GetPlayerAnimSpeed and
 * ImposeFriction), MovePlayerHorizontally / MovePlayerVertically,
 * and the whole PlayerBGCollision tree with its metatile checks,
 * coin / axe / pipe / vine / flagpole handlers and the head-bump
 * block routines (PlayerHeadCollision, BumpBlock, BlockBumpedChk,
 * BrickShatter, SpawnBrickChunks, CheckTopOfBlock, SetupPowerUp,
 * SetupJumpCoin, FindEmptyMiscSlot). */

#ifndef SMBC_CORE_PHYSICS_H
#define SMBC_CORE_PHYSICS_H

#include "state.h"

/* smb.asm: PlayerMovementSubs — crouch bookkeeping, the physics sub
 * and the per-state movement sub */
void player_movement_subs(CoreState *state);

/* smb.asm: PlayerPhysicsSub — climbing physics on a climbable tile,
 * otherwise jump / swim initiation and the horizontal physics */
void player_physics_sub(CoreState *state);

/* smb.asm: PlayerBGCollision — the head / feet / side block-buffer
 * collision tree and everything it dispatches to */
void player_bg_collision(CoreState *state);

/* smb.asm: HandlePipeEntry — pipe entry from the feet check, kept
 * public because the exit-pipe routines reuse it */
void handle_pipe_entry(CoreState *state, uint8_t right_mt, uint8_t left_mt);

#endif /* SMBC_CORE_PHYSICS_H */
