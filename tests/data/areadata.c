/* tests/data/areadata.c — exercises src/data/areadata.c: the pointer
 * tables' order, the high-byte offset bases, and stream terminators.
 * Every stream's bytes were verified verbatim against the donor ROM
 * image when the atom was extracted. */
#include <assert.h>
#include <stdio.h>

#include "../../src/data/areadata.h"

static void terminated(const uint8_t *stream, uint8_t terminator) {
  for (unsigned i = 0u; i < 256u; i++) {
    if (stream[i] == terminator) return;
  }
  assert(0 && "stream missing its terminator");
}

int main(void) {
  /* smb.asm: the bases the area type selects between */
  assert(SMBC_ENEMY_ADDR_H_OFFSETS[0] == 0x1fu);
  assert(SMBC_ENEMY_ADDR_H_OFFSETS[1] == 0x06u);
  assert(SMBC_ENEMY_ADDR_H_OFFSETS[2] == 0x1cu);
  assert(SMBC_ENEMY_ADDR_H_OFFSETS[3] == 0x00u);
  assert(SMBC_AREA_DATA_H_OFFSETS[0] == 0x00u);
  assert(SMBC_AREA_DATA_H_OFFSETS[1] == 0x03u);
  assert(SMBC_AREA_DATA_H_OFFSETS[2] == 0x19u);
  assert(SMBC_AREA_DATA_H_OFFSETS[3] == 0x1cu);

  /* smb.asm: E_CastleArea1 opens 1-4's enemy stream */
  assert(SMBC_ENEMY_AREAS[0][0] == 0x76u);
  assert(SMBC_ENEMY_AREAS[0][1] == 0xddu);
  /* smb.asm: E_GroundArea1 (3-3) follows the six castles */
  assert(SMBC_ENEMY_AREAS[6][0] == 0xa5u);
  assert(SMBC_ENEMY_AREAS[6][1] == 0x86u);
  /* smb.asm: the waters close the enemy table */
  assert(SMBC_ENEMY_AREAS[33][0] == 0x47u);
  assert(SMBC_ENEMY_AREAS[33][1] == 0x9bu);
  /* smb.asm: the level table opens with the waters */
  assert(SMBC_LEVEL_AREAS[0][0] == 0x41u);
  assert(SMBC_LEVEL_AREAS[0][1] == 0x01u);
  assert(SMBC_LEVEL_AREAS[3][0] == 0x94u); /* L_GroundArea1 */

  /* every enemy stream ends $ff, every level stream $fd */
  for (unsigned i = 0u; i < SMBC_ENEMY_AREAS_COUNT; i++)
    terminated(SMBC_ENEMY_AREAS[i], 0xffu);
  for (unsigned i = 0u; i < SMBC_LEVEL_AREAS_COUNT; i++)
    terminated(SMBC_LEVEL_AREAS[i], 0xfdu);

  printf("tests/data/areadata: ok\n");
  return 0;
}
