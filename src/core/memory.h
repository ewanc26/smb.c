/* core/memory.h — memory initialisation.
 *
 * Owns: the boot-time RAM clear and (pending) name table initialisation.
 */
#ifndef SMBC_CORE_MEMORY_H
#define SMBC_CORE_MEMORY_H

#include <stdint.h>

#include "state.h"

struct Smbc;

/* smb.asm: InitializeMemory — clear pages $07 down to $00, skipping
 * $0160-$01FF (the stack). Only the first page stops at the given
 * starting offset; the loop underflows Y to $ff afterwards, so every
 * later page is erased in full. */
void initialize_memory(CoreState *state, uint8_t offset);

/* smb.asm: InitializeNameTables — select the pattern tables, then clear
 * name table 1 ($2400) and name table 0 ($2000) with the blank tile, reset
 * the VRAM buffer header and the scroll. Note the original clears exactly
 * 768 tiles plus 64 trailing bytes per table, not the full 960+64; that is
 * the cartridge's own behaviour (verified against the donor ROM), kept
 * as-is. */
void initialize_name_tables(struct Smbc *smbc);

#endif /* SMBC_CORE_MEMORY_H */
