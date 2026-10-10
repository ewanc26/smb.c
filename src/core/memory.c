/* core/memory.c — see core/memory.h for the contract. */
#include "memory.h"

void initialize_memory(CoreState *state, uint8_t offset) {
  /* smb.asm: InitializeMemory. Y starts at the caller's boot offset and
   * runs down to $00 on every page; X walks pages $07..$00. */
  for (int page = 0x07; page >= 0; page--) {
    for (int y = offset; y >= 0; y--) {
      if (page == 0x01 && y >= 0x60) {
        continue; /* smb.asm: SkipByte — the stack at $0160-$01FF */
      }
      state->ram[(page << 8) | y] = 0x00;
    }
  }
}

void initialize_name_tables(struct Smbc *smbc) {
  /* TODO(port): smb.asm: InitializeNameTables, pending UpdateScreen */
  (void)smbc;
}
