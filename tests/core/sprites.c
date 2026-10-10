/* tests/core/sprites.c — exercises src/core/sprites.c: offscreen moves and
 * the offset shuffler's wrap and carry behaviour. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/sprites.h"
#include "../../src/core/state.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* all 64 sprites offscreen */
  move_all_sprites_offscreen(&smbc.state);
  for (int i = 0; i < 64; i++) {
    assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + i * 4] == 0xf8u);
  }

  /* all but sprite 0 */
  smbc.state.ram[SMBC_SPRITE_Y_POSITION] = 0x12u;
  move_sprites_offscreen(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION] == 0x12u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 4] == 0xf8u);

  /* shuffler, with the amounts seeded as SecondaryGameSetup seeds them */
  smbc.state.ram[SMBC_SPR_SHUFFLE_AMT + 0] = 0x58u;
  smbc.state.ram[SMBC_SPR_SHUFFLE_AMT + 1] = 0x48u;
  smbc.state.ram[SMBC_SPR_SHUFFLE_AMT + 2] = 0x38u;
  smbc.state.ram[SMBC_SPR_SHUFFLE_AMT_OFFSET] = 0x00u;
  smbc.state.ram[SMBC_SPR_DATA_OFFSET + 0] = 0x28u; /* >= preset: shuffles */
  smbc.state.ram[SMBC_SPR_DATA_OFFSET + 1] = 0xf0u; /* overflows the byte */
  smbc.state.ram[SMBC_SPR_DATA_OFFSET + 2] = 0x10u; /* below preset: kept */
  sprite_shuffler(&smbc.state);
  assert(smbc.state.ram[SMBC_SPR_DATA_OFFSET + 0] == 0x80u); /* 0x28+0x58 */
  assert(smbc.state.ram[SMBC_SPR_DATA_OFFSET + 1] == 0x70u); /* wraps, +0x28 */
  assert(smbc.state.ram[SMBC_SPR_DATA_OFFSET + 2] == 0x10u);
  assert(smbc.state.ram[SMBC_SPR_SHUFFLE_AMT_OFFSET] == 0x01u);

  printf("tests/core/sprites: ok\n");
  return 0;
}
