/* core/flagpole.h — the flagpole flag and its score floatey number.
 *
 * Owns: the per-frame flagpole routine — sliding the flag down the pole
 * while mario slides, awarding the height score and ending the level —
 * and the draw of the flag and floatey number.
 */
#ifndef SMBC_CORE_FLAGPOLE_H
#define SMBC_CORE_FLAGPOLE_H

#include "state.h"

/* smb.asm: FlagpoleRoutine — enemy slot $05, run from GameEngine. */
void flagpole_routine(CoreState *state);

#endif /* SMBC_CORE_FLAGPOLE_H */
