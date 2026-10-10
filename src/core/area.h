/* core/area.h — area pointer resolution.
 *
 * Owns: turning (WorldNumber, AreaNumber) into the area pointer byte and
 * its two-bit area type, via the offset tables in data/area. First
 * consumer is GameOverMode's ContinueGame; TitleScreenMode and GameMode
 * call the same routines.
 */
#ifndef SMBC_CORE_AREA_H
#define SMBC_CORE_AREA_H

#include <stdint.h>

#include "state.h"

struct Smbc;

/* smb.asm: FindAreaPointer — table lookup only, no side effects. */
uint8_t find_area_pointer(const CoreState *state);

/* smb.asm: LoadAreaPointer — stores the pointer into AreaPointer and
 * falls through to GetAreaType, storing the top two bits (rotated down)
 * into AreaType. */
void load_area_pointer(struct Smbc *smbc);

#endif /* SMBC_CORE_AREA_H */
