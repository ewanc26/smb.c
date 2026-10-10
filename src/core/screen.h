/* core/screen.h — screen update helpers.
 *
 * Owns: the small PPU-writing subroutines the reset and NMI paths share,
 * and the top-score bookkeeping. The VRAM buffer writer itself
 * (UpdateScreen) is pending translation and currently a documented no-op.
 */
#ifndef SMBC_CORE_SCREEN_H
#define SMBC_CORE_SCREEN_H

#include <stdint.h>

#include "state.h"

struct Smbc; /* frame.h */

/* smb.asm: WritePPUReg1 — write to $2000 and its mirror together. */
void write_ppu_reg1(struct Smbc *smbc, uint8_t value);

/* smb.asm: InitScroll — write the same value to $2005 twice. */
void init_scroll(struct Smbc *smbc, uint8_t value);

/* smb.asm: UpdateScreen — TODO(port): the VRAM buffer writer, pending the
 * VRAM buffer data tables. Currently a no-op; the NMI's surrounding buffer
 * bookkeeping is already in place. */
void update_screen(struct Smbc *smbc);

/* smb.asm: UpdateTopScore — copy either player's score into the top score
 * display when it exceeds it. */
void update_top_score(CoreState *state);

#endif /* SMBC_CORE_SCREEN_H */
