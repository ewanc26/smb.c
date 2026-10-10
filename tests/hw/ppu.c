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

  /* name-table mirroring: the donor cartridge is vertical mirroring /
   * horizontal arrangement, so A11 decodes to the duplicate and A10
   * selects the physical 1KB bank */
  (void)ppu_read(&ppu, 2);
  ppu_write(&ppu, 0, 0x00);
  ppu_write(&ppu, 6, 0x20);
  ppu_write(&ppu, 6, 0x00);
  ppu_write(&ppu, 7, 0x11); /* $2000 -> storage $2000 */
  assert(ppu.vram[0x2000] == 0x11);
  assert(ppu.vram[0x2800] == 0x00); /* name table 2 is a mirror */

  /* a write through the mirror lands on the same storage */
  ppu_write(&ppu, 6, 0x28);
  ppu_write(&ppu, 6, 0x07);
  ppu_write(&ppu, 7, 0x33); /* $2807 -> storage $2007 */
  assert(ppu.vram[0x2007] == 0x33);
  assert(ppu.vram[0x2807] == 0x00);

  /* name table 1 is a distinct bank */
  ppu_write(&ppu, 6, 0x24);
  ppu_write(&ppu, 6, 0x00);
  ppu_write(&ppu, 7, 0x22); /* $2400 -> storage $2400 */
  assert(ppu.vram[0x2400] == 0x22);
  assert(ppu.vram[0x2000] == 0x11);

  /* $2C00 is name table 3, the mirror of name table 1 */
  ppu_write(&ppu, 6, 0x2c);
  ppu_write(&ppu, 6, 0x00);
  ppu_write(&ppu, 7, 0x44);
  assert(ppu.vram[0x2400] == 0x44);
  assert(ppu.vram[0x2c00] == 0x00);

  /* $3000-$3EFF is a bus mirror of $2000-$2EFF: no extra storage */
  ppu_write(&ppu, 6, 0x30);
  ppu_write(&ppu, 6, 0x00);
  ppu_write(&ppu, 7, 0x55);
  assert(ppu.vram[0x2000] == 0x55);
  assert(ppu.vram[0x3000] == 0x00);

  /* palette-region reads return the palette byte immediately — no priming
   * read — and fill the buffer with the byte "underneath" at $2Fxx (the
   * $3000-$3EFF mirror of the name tables) */
  (void)ppu_read(&ppu, 2);
  ppu_write(&ppu, 0, 0x00);
  ppu_write(&ppu, 6, 0x3f);
  ppu_write(&ppu, 6, 0x00);
  ppu_write(&ppu, 7, 0x2a); /* palette byte at $3F00 */
  ppu_write(&ppu, 6, 0x2f);
  ppu_write(&ppu, 6, 0x00);
  ppu_write(&ppu, 7, 0x5b); /* underlying $2F00 -> storage at $2700 */
  ppu_write(&ppu, 6, 0x3f);
  ppu_write(&ppu, 6, 0x00);
  assert(ppu_read(&ppu, 7) == 0x2a); /* immediate palette byte */
  /* the read buffer now holds the shadowed $2700 byte; a buffered read
   * outside the palette region returns it */
  ppu_write(&ppu, 6, 0x20);
  ppu_write(&ppu, 6, 0x00);
  assert(ppu_read(&ppu, 7) == 0x5b);

  /* palette RAM is 32 bytes: $3F20-$3FFF aliases $3F00-$3F1F */
  ppu_write(&ppu, 6, 0x3f);
  ppu_write(&ppu, 6, 0x21);
  ppu_write(&ppu, 7, 0x7e);
  assert(ppu.vram[0x3f01] == 0x7e); /* $3F21 folds onto $3F01 */
  assert(ppu.vram[0x3f21] == 0x00); /* the mirror carries no storage */
  assert(ppu.vram[0x3f20] == 0x00);

  printf("tests/hw/ppu: ok\n");
  return 0;
}
