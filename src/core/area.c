/* core/area.c — see core/area.h for the contract. */
#include "area.h"

#include "../data/area.h"
#include "frame.h"
#include "state.h"

uint8_t find_area_pointer(const CoreState *state) {
  /* smb.asm: FindAreaPointer — offset from world variable, plus the area
   * number, indexes the flat area table. */
  uint8_t y = SMBC_WORLD_ADDR_OFFSETS[state->ram[SMBC_WORLD_NUMBER]];
  return SMBC_AREA_ADDR_OFFSETS[y + state->ram[SMBC_AREA_NUMBER]];
}

void load_area_pointer(Smbc *smbc) {
  uint8_t a = find_area_pointer(&smbc->state);
  smbc->state.ram[SMBC_AREA_POINTER] = a;
  /* smb.asm: GetAreaType — and #%01100000, then asl/rol/rol/rol rotates
   * %0xx00000 into %000000xx; equivalent to a shift. */
  smbc->state.ram[SMBC_AREA_TYPE] = (a >> 5) & 0x03u;
}
