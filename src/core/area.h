/* core/area.h — area pointer resolution and area data setup.
 *
 * Owns: turning (WorldNumber, AreaNumber) into the area pointer byte and
 * its two-bit area type, via the offset tables in data/area, and
 * resolving that pointer into the object streams in data/areadata —
 * GetAreaDataAddrs, which also unpacks the level header into the
 * scenery, entrance, timer, terrain and style state the parser reads.
 * Also the screen-edge geometry the area setup and scroll handler share
 * (GetScreenPosition).
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

/* smb.asm: GetAreaDataAddrs — points the state at the enemy and level
 * object streams for the current area pointer and unpacks the level
 * header's two bytes. Advances the level stream past the header, as the
 * original's address increment does. */
void get_area_data_addrs(struct Smbc *smbc);

/* smb.asm: GetScreenPosition — right screen boundary = left + 255 pixels,
 * with the page carrying. Returns the right page location (the value A
 * holds on exit). */
uint8_t get_screen_position(CoreState *state);

#endif /* SMBC_CORE_AREA_H */
