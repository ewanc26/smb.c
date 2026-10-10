/* hw/ppu.h — PPU register model.
 *
 * Owns: the CPU-visible state of the picture processing unit's eight
 * mirrored registers, including the $2005/$2006 write-pair latch and the
 * status flags reads can clear. Rendering is not modelled; a host frontend
 * consumes VRAM writes later.
 *
 * Collaborators: core/frame drives vblank; translated game code touches the
 * registers exactly as the original does.
 */
#ifndef SMBC_HW_PPU_H
#define SMBC_HW_PPU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t ctrl1;      /* $2000 writes, and the game's mirror lives in RAM */
  uint8_t ctrl2;      /* $2001 */
  uint8_t spr_addr;   /* $2003 */
  uint16_t vram_addr; /* assembled by the $2006 write pair */
  uint8_t scroll_x;   /* first $2005 write */
  uint8_t scroll_y;   /* second $2005 write */
  uint8_t read_buffer; /* buffered $2007 data */
  bool latch;         /* $2005/$2006 first/second-write toggle */
  bool vblank;        /* status bit 7, set by the frame loop */
} Ppu;

void ppu_init(Ppu *ppu);

/* Register access by index 0-7, as the CPU sees $2000-$2007. */
void ppu_write(Ppu *ppu, uint8_t reg, uint8_t value);
uint8_t ppu_read(Ppu *ppu, uint8_t reg);

/* Driven by the frame loop: entering vblank sets the flag, and a $2002 read
 * observes then clears it. */
void ppu_set_vblank(Ppu *ppu, bool active);

#endif /* SMBC_HW_PPU_H */
