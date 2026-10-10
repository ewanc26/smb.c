/* hw/ppu.h — PPU register model.
 *
 * Owns: the CPU-visible state of the picture processing unit's eight
 * mirrored registers, including the $2005/$2006 write-pair latch and the
 * status flags reads can clear, and a flat VRAM image written through
 * $2006/$2007. Reads and writes reach actual storage through the donor
 * cartridge's address decoding: name-table mirroring ($2800/$2C00 alias
 * $2000/$2400), the $3000-$3EFF bus mirror of the name tables, and the
 * 32-byte palette RAM at $3F00-$3FFF. Rendering is not modelled; a host
 * frontend consumes the VRAM image later.
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
  uint8_t vram[0x4000]; /* flat VRAM image; mirroring applied on access */
  uint8_t scroll_x;   /* first $2005 write */
  uint8_t scroll_y;   /* second $2005 write */
  uint8_t read_buffer; /* buffered $2007 data */
  bool latch;         /* $2005/$2006 first/second-write toggle */
  bool vblank;        /* status bit 7, set by the frame loop */
  bool sprite0;       /* status bit 6, driven by the frame loop like vblank */
  uint8_t oam_page;   /* last page nominated by a $4014 OAM DMA write */
} Ppu;

void ppu_init(Ppu *ppu);

/* Register access by index 0-7, as the CPU sees $2000-$2007. */
void ppu_write(Ppu *ppu, uint8_t reg, uint8_t value);
uint8_t ppu_read(Ppu *ppu, uint8_t reg);

/* Driven by the frame loop: entering vblank sets the flag, and a $2002 read
 * observes then clears it. */
void ppu_set_vblank(Ppu *ppu, bool active);

/* $4014 OAM DMA: page is the high byte of the source address ($02 for the
 * $0200-$02FF sprite page). OAM contents are that RAM page itself in the
 * port, so this only records which page was nominated. */
void ppu_oam_dma(Ppu *ppu, uint8_t page);

/* Host synchronisation points. The original spins on $2002 until the
 * hardware reaches vblank / the sprite-0 hit; in this single-threaded port
 * the frame driver owns timing, so the waits are satisfied immediately and
 * exist to keep the translated code's shape. A cycle-faithful frontend may
 * give them real duration; game logic must not depend on it. */
void ppu_await_vblank(Ppu *ppu);
void ppu_await_sprite0_clear(Ppu *ppu);
void ppu_await_sprite0_set(Ppu *ppu);

#endif /* SMBC_HW_PPU_H */
