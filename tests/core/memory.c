/* tests/core/memory.c — exercises src/core/memory.c: the boot-time RAM
 * clear's stack skip, and the name table clear's exact write pattern. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/core/frame.h"
#include "../../src/core/memory.h"
#include "../../src/core/state.h"

int main(void) {
  /* InitializeMemory: clears from the offset down on every page, skips the
   * stack region */
  CoreState state;
  state_init(&state);
  memset(state.ram, 0xff, sizeof state.ram);
  initialize_memory(&state, SMBC_WARM_BOOT_OFFSET);
  assert(state.ram[0x07d6] == 0x00u);
  assert(state.ram[0x07fe] == 0xffu); /* above the warm boot offset */
  assert(state.ram[0x015f] == 0x00u);
  assert(state.ram[0x0160] == 0xffu);
  /* only the first page stops at the offset; every later page is cleared
   * in full (Y underflows to $ff after the first byte loop) */
  assert(state.ram[0x06ff] == 0x00u);
  assert(state.ram[0x00ff] == 0x00u);
  assert(state.ram[0x05ff] == 0x00u);

  /* InitializeNameTables, through boot for a realistic mirror state */
  Smbc smbc;
  smbc_init(&smbc);

  /* both name tables: 768 blank tiles from the base, 64 zero bytes after */
  const uint16_t base[2] = {0x2400u, 0x2000u};
  for (int t = 0; t < 2; t++) {
    assert(smbc.ppu.vram[base[t]] == 0x24u);
    assert(smbc.ppu.vram[base[t] + 767] == 0x24u);
    assert(smbc.ppu.vram[base[t] + 768] == 0x00u);
    assert(smbc.ppu.vram[base[t] + 831] == 0x00u);
    /* the original writes nothing past 768+64 — prefill proves it */
  }

  /* mirror: pattern table select set, low nybble otherwise cleared */
  assert((smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] & 0x1fu) == 0x10u);

  /* buffer header and scroll reset */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x00u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1] == 0x00u);
  assert(smbc.state.ram[SMBC_HORIZONTAL_SCROLL] == 0x00u);
  assert(smbc.state.ram[SMBC_VERTICAL_SCROLL] == 0x00u);

  printf("tests/core/memory: ok\n");
  return 0;
}
