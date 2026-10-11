/* core/whirlpool.h — the water-level whirlpools.
 *
 * Owns: ProcessWhirlpools, the per-frame check that finds a whirlpool
 * the player is within (across the five whirlpool data slots), pulls
 * him horizontally towards its centre on alternate frames and imposes
 * the vertical whirlpool gravity on him.
 *
 * Collaborators: the whirlpool data slots (shared with the cannons in
 * core/state), the player's position, page location and collision bits,
 * and the generic vertical physics in core/move. It is not on the
 * RunEnemyObjectsCore jump table — the game engine calls
 * process_whirlpools directly.
 */
#ifndef SMBC_CORE_WHIRLPOOL_H
#define SMBC_CORE_WHIRLPOOL_H

#include "state.h"

/* smb.asm: ProcessWhirlpools — find a whirlpool containing the player in
 * a water level and apply its pull and downward gravity */
void process_whirlpools(CoreState *state);

#endif /* SMBC_CORE_WHIRLPOOL_H */
