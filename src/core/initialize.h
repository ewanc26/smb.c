/* core/initialize.h — the GameMode area-initialisation task.
 *
 * Owns: InitializeArea, the first task of the GameMode tree. It clears
 * the working RAM, resets the timers, chooses the starting page (a saved
 * entry page overrides the halfway page), sets the screen boundary and
 * name-table render address, primes the block-buffer column, resolves
 * the area data, applies the secondary-hard-mode rule, and hands control
 * to the next mode task.
 *
 * Collaborators: core/memory (InitializeMemory), core/area
 * (GetScreenPosition, GetAreaDataAddrs), the page/hard-mode/timer RAM map
 * in core/state, and the Silence music constant (data/music). Called from
 * game_mode's dispatch in core/gamemode.
 */
#ifndef SMBC_CORE_INITIALIZE_H
#define SMBC_CORE_INITIALIZE_H

#include "state.h"

struct Smbc;

/* smb.asm: InitializeArea */
void initialize_area(struct Smbc *smbc);

#endif /* SMBC_CORE_INITIALIZE_H */
