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

  /* $2006/$2007: write through the address pair, then walk by 1 or 32.
   * The latch is mid-pair from the checks above; a $2002 read re-arms it,
   * and ctrl1's d2 must be clear for the increment-by-1 case. */
  (void)ppu_read(&ppu, 2);
  ppu_write(&ppu, 0, 0x00);
  ppu_write(&ppu, 6, 0x23);
  ppu_write(&ppu, 6, 0x10);
  ppu_write(&ppu, 7, 0x5a);
  ppu_write(&ppu, 7, 0x6b);
  assert(ppu.vram[0x2310] == 0x5a);
  assert(ppu.vram[0x2311] == 0x6b);

  ppu_write(&ppu, 0, 0x04); /* d2: increment by 32 */
  ppu_write(&ppu, 6, 0x20);
  ppu_write(&ppu, 6, 0x00);
  ppu_write(&ppu, 7, 0x77);
  ppu_write(&ppu, 7, 0x88);
  assert(ppu.vram[0x2000] == 0x77);
  assert(ppu.vram[0x2020] == 0x88);

  /* $2007 reads are buffered one access behind */
  ppu_write(&ppu, 0, 0x00);
  ppu_write(&ppu, 6, 0x23);
  ppu_write(&ppu, 6, 0x10);
  assert(ppu_read(&ppu, 7) == 0x00); /* stale buffer */
  assert(ppu_read(&ppu, 7) == 0x5a);

  printf("tests/hw/ppu: ok\n");
  return 0;
}
