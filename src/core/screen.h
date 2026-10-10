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

/* smb.asm: the NMI's pointer load — sets the $00/$01 indirect from
 * VRAM_AddrTable_Low/High indexed by VRAM_Buffer_AddrCtrl. ROM-backed
 * entries (palettes, messages) are pending their data tables and read as
 * empty update sets until then. */
void load_vram_addr_table(CoreState *state);

/* smb.asm: UpdateScreen / WriteBufferToScreen — walk the update sets at
 * the $00/$01 indirect, writing each to VRAM through $2006/$2007, until a
 * zero high byte. Falls through to InitScroll either way. */
void update_screen(struct Smbc *smbc);

/* smb.asm: UpdateTopScore — copy either player's score into the top score
 * display when it exceeds it. */
void update_top_score(CoreState *state);

/* smb.asm: PrintStatusBarNumbers — print the status bar values selected by
 * the two nybbles of `nybbles` into VRAM buffer 1: the low nybble selects
 * one display (score, coin tally, game timer — see data/statusbar), the
 * high nybble selects another. Selectors of six or more print nothing.
 * Callers handle zero suppression. */
void print_status_bar_numbers(CoreState *state, uint8_t nybbles);

/* smb.asm: DigitsMathRoutine — apply the six digit modifiers to the six
 * display digits ending at index `y`, borrowing and carrying between
 * digits (digits stay 0-9), then erase the modifiers. On the title screen
 * only the erase happens. */
void digits_math_routine(CoreState *state, uint8_t y);

#endif /* SMBC_CORE_SCREEN_H */
