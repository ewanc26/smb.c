/* tests/hw/ppu.c — exercises src/hw/ppu.c: the $2005/$2006 write-pair latch
 * and the $2002 status read clearing vblank and the latch. */
#include <assert.h>
#include <stdio.h>

#include "../../src/hw/ppu.h"

int main(void) {
  Ppu ppu;
  ppu_init(&ppu);

  /* $2006 pair assembles a 16-bit VRAM address, high byte first */
  ppu_write(&ppu, 6, 0x21u);
  ppu_write(&ppu, 6, 0x08u);
  assert(ppu.vram_addr == 0x2108u);

  /* $2005 pair stores X then Y */
  ppu_write(&ppu, 5, 0x40u);
  ppu_write(&ppu, 5, 0x18u);
  assert(ppu.scroll_x == 0x40u);
  assert(ppu.scroll_y == 0x18u);

  /* vblank is observable at $2002 bit 7, and the read clears it */
  ppu_set_vblank(&ppu, true);
  assert((ppu_read(&ppu, 2) & 0x80u) != 0);
  assert((ppu_read(&ppu, 2) & 0x80u) == 0);

  /* a $2002 read also resets the write-pair latch */
  ppu_write(&ppu, 6, 0x3fu);
  (void)ppu_read(&ppu, 2);
  ppu_write(&ppu, 6, 0x10u);
  assert(ppu.vram_addr == 0x1000u);

  /* register indices mirror mod 8, as the hardware decoding does */
  ppu_write(&ppu, 8, 0x99u); /* $2000 mirrored */
  assert(ppu.ctrl1 == 0x99u);

  printf("tests/hw/ppu: ok\n");
  return 0;
}
