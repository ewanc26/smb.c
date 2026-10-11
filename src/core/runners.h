/* core/runners.h — the per-frame enemy object runners.
 *
 * smb.asm's RunEnemyObjectsCore jump table and its runner routines.
 * This atom starts with OffscreenBoundsCheck, the shared erasure
 * guard every runner tails into; the runners themselves and the
 * dispatch follow. */
#ifndef SMBC_CORE_RUNNERS_H
#define SMBC_CORE_RUNNERS_H

#include <stdint.h>

#include "state.h"

/* smb.asm: OffscreenBoundsCheck — erase the object when it strays
 * seventy-two pixels (fifty-six less for hammer bros and piranha
 * plants) beyond the left screen edge, or seventy-two beyond the
 * right edge, with exemptions for the spiny's egg state and several
 * objects, and never for the flying cheep-cheep */
void offscreen_bounds_check(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_RUNNERS_H */
