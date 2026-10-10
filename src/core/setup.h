/* core/setup.h — game setup task routines.
 *
 * Owns: SecondaryGameSetup and GetAreaMusic. SecondaryGameSetup runs as
 * OperMode task 1 for every mode that sets up a game: it re-enables screen
 * output, clears the VRAM buffer page, resets the game flags, seeds the
 * sprite shuffler, loads the default sprite data offsets and sprite 0's
 * template, then advances the task counter.
 */
#ifndef SMBC_CORE_SETUP_H
#define SMBC_CORE_SETUP_H

#include "frame.h"
#include "state.h"
#include "../hw/ppu.h"

/* smb.asm: GetAreaMusic — queue the area's music, unless on the title
 * screen. Pipe intro scenes override the area type, the cloud override
 * overrides everything else. */
void get_area_music(CoreState *state);

/* smb.asm: SecondaryGameSetup */
void secondary_game_setup(struct Smbc *smbc);

#endif /* SMBC_CORE_SETUP_H */
