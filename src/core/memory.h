/* core/memory.h — memory initialisation.
 *
 * Owns: the boot-time RAM clear and (pending) name table initialisation.
 */
#ifndef SMBC_CORE_MEMORY_H
#define SMBC_CORE_MEMORY_H

#include <stdint.h>

#include "state.h"

struct Smbc;

/* smb.asm: InitializeMemory — clear pages $07 down to $00 from the given
 * starting offset in each page's first iteration (warm/cold boot offset),
 * skipping $0160-$01FF (the stack). */
void initialize_memory(CoreState *state, uint8_t offset);

/* smb.asm: InitializeNameTables — TODO(port): writes both name tables via
 * the VRAM buffer, pending UpdateScreen. Currently a no-op. */
void initialize_name_tables(struct Smbc *smbc);

#endif /* SMBC_CORE_MEMORY_H */
