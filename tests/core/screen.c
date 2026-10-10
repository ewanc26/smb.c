/* tests/core/screen.c — exercises src/core/screen.c: the buffer writer's
 * address, length, repeat and increment modes, the indirect advance and
 * the address table load. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/screen.h"
#include "../../src/core/state.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* smb.asm: the pointer load — buffer 1, its offset, and buffer 2 */
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x00u;
  load_vram_addr_table(&smbc.state);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x01u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x05u;
  load_vram_addr_table(&smbc.state);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x00u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x07u;
  load_vram_addr_table(&smbc.state);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x41u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);

  /* one set: three bytes to $2100, increment-by-1, no repeat */
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x00u;
  load_vram_addr_table(&smbc.state);
  smbc.state.ram[0x0301] = 0x21u;
  smbc.state.ram[0x0302] = 0x00u;
  smbc.state.ram[0x0303] = 0x03u;
  smbc.state.ram[0x0304] = 0xaau;
  smbc.state.ram[0x0305] = 0xbbu;
  smbc.state.ram[0x0306] = 0xccu;
  smbc.state.ram[0x0307] = 0x00u; /* terminator */
  update_screen(&smbc);
  assert(smbc.ppu.vram[0x2100] == 0xaau);
  assert(smbc.ppu.vram[0x2101] == 0xbbu);
  assert(smbc.ppu.vram[0x2102] == 0xccu);
  /* the indirect advanced onto the terminator */
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x07u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);
  /* InitScroll ran with A=0 */
  assert(smbc.ppu.scroll_x == 0x00u);
  assert(smbc.ppu.scroll_y == 0x00u);

  /* d6 set: repeat one byte; d7 set: increment-by-32 */
  load_vram_addr_table(&smbc.state);
  smbc.state.ram[0x0301] = 0x22u;
  smbc.state.ram[0x0302] = 0x40u;
  smbc.state.ram[0x0303] = 0xc2u; /* repeat + inc-32, length 2 */
  smbc.state.ram[0x0304] = 0xeeu;
  smbc.state.ram[0x0305] = 0x00u;
  update_screen(&smbc);
  assert(smbc.ppu.vram[0x2240] == 0xeeu);
  assert(smbc.ppu.vram[0x2260] == 0xeeu); /* +32, same byte */
  /* mirror took the increment-by-32 bit for the set */
  assert((smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] & 0x04u) != 0);

  /* an empty buffer is just the fall-through to InitScroll */
  load_vram_addr_table(&smbc.state);
  smbc.state.ram[0x0301] = 0x00u;
  smbc.state.ram[SMBC_HORIZONTAL_SCROLL] = 0x99u; /* untouched here */
  update_screen(&smbc);
  assert(smbc.ppu.scroll_x == 0x00u);

  printf("tests/core/screen: ok\n");
  return 0;
}
