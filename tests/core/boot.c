/* tests/core/boot.c — exercises src/core/boot.c: cold boot clears RAM
 * except the stack region and $07FF, then seeds the validation bytes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/core/boot.h"
#include "../../src/core/frame.h"
#include "../../src/core/state.h"

int main(void) {
  Smbc smbc;
  state_init(&smbc.state);
  joypad_init(&smbc.joypad1);
  joypad_init(&smbc.joypad2);
  ppu_init(&smbc.ppu);
  apu_init(&smbc.apu);

  memset(smbc.state.ram, 0xff, sizeof smbc.state.ram);
  /* force the cold boot path: invalid digit in the top score */
  smbc.state.ram[SMBC_TOP_SCORE_DISPLAY] = 0x0fu;

  boot_run(&smbc);

  /* InitializeMemory cleared $0000-$07FE... */
  assert(smbc.state.ram[0x0000] == 0x00u);
  assert(smbc.state.ram[0x015f] == 0x00u);
  assert(smbc.state.ram[0x07fe] == 0x00u);
  /* ...except the stack at $0160-$01FF... */
  assert(smbc.state.ram[0x0160] == 0xffu);
  assert(smbc.state.ram[0x01ff] == 0xffu);
  /* ...and $07FF, which boot then claims for the validation byte */
  assert(smbc.state.ram[SMBC_WARM_BOOT_VALIDATION] == 0xa5u);

  assert(smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG] == 0xa5u);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] == 0x80u);
  assert(smbc.state.ram[SMBC_OPER_MODE] == SMBC_MODE_TITLE);
  assert(apu_read(&smbc.apu, 0x4015u) == 0x0fu);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION] == 0xf8u);

  printf("tests/core/boot: ok\n");
  return 0;
}
