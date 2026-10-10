/* tests/data/metatiles.c — exercises src/data/metatiles.c: table sizes,
 * the pointer table, and spot-checked contents against the disassembly. */
#include <assert.h>
#include <stdio.h>

#include "../../src/data/metatiles.h"

int main(void) {
  assert(SMBC_METATILE_GRAPHICS[0] == SMBC_PALETTE0_MTILES);
  assert(SMBC_METATILE_GRAPHICS[3] == SMBC_PALETTE3_MTILES);
  assert(SMBC_METATILE_COUNTS[0] == SMBC_PALETTE0_MTILES_COUNT);
  assert(SMBC_METATILE_COUNTS[1] == SMBC_PALETTE1_MTILES_COUNT);

  /* smb.asm: palette 0 entry 0 is blank, entry 1 is the black metatile */
  assert(SMBC_PALETTE0_MTILES[0] == 0x24u);
  assert(SMBC_PALETTE0_MTILES[4] == 0x27u);
  assert(SMBC_PALETTE0_MTILES[7] == 0x27u);
  /* smb.asm: palette 3 entry 5 is the axe */
  assert(SMBC_PALETTE3_MTILES[5 * 4 + 0] == 0x7bu);
  assert(SMBC_PALETTE3_MTILES[5 * 4 + 3] == 0x7eu);
  /* smb.asm: palette 2 entry 9 is bowser's bridge */
  assert(SMBC_PALETTE2_MTILES[9 * 4 + 0] == 0x77u);
  assert(SMBC_PALETTE2_MTILES[9 * 4 + 1] == 0x79u);

  printf("tests/data/metatiles: ok\n");
  return 0;
}
