/* hw/ppu.c — see hw/ppu.h for the contract.
 *
 * Failure semantics: register indices are masked to 0-7, matching the NES's
 * mirrored decoding of $2000-$3FFF; out-of-range callers cannot corrupt
 * adjacent state.
 */
#include "ppu.h"

void ppu_init(Ppu *ppu) {
  *ppu = (Ppu){0};
}

void ppu_set_vblank(Ppu *ppu, bool active) {
  ppu->vblank = active;
}

void ppu_oam_dma(Ppu *ppu, uint8_t page) {
  ppu->oam_page = page;
}

void ppu_await_vblank(Ppu *ppu) {
  /* satisfied immediately: present, then consumed as a $2002 read would */
  ppu->vblank = true;
  ppu->vblank = false;
  ppu->latch = false;
}

void ppu_await_sprite0_clear(Ppu *ppu) {
  ppu->sprite0 = false;
}

void ppu_await_sprite0_set(Ppu *ppu) {
  ppu->sprite0 = true;
}

/* $2007 increment shared by reads and writes: d2 of $2000 selects 32. */
static uint16_t ppu_data_increment(const Ppu *ppu) {
  return (ppu->ctrl1 & 0x04u) ? 32u : 1u;
}

void ppu_write(Ppu *ppu, uint8_t reg, uint8_t value) {
  switch (reg & 0x7u) {
    case 0: ppu->ctrl1 = value; break;
    case 1: ppu->ctrl2 = value; break;
    case 3: ppu->spr_addr = value; break;
    case 5: /* $2005: first write X, second Y, toggling */
      if (ppu->latch) {
        ppu->scroll_y = value;
      } else {
        ppu->scroll_x = value;
      }
      ppu->latch = !ppu->latch;
      break;
    case 6: /* $2006: high byte then low byte, toggling */
      if (ppu->latch) {
        ppu->vram_addr = (uint16_t)(ppu->vram_addr | value);
      } else {
        ppu->vram_addr = (uint16_t)((value & 0x3fu) << 8); /* 14-bit space */
      }
      ppu->latch = !ppu->latch;
      break;
    case 7: /* $2007: write at the address, then increment by 1 or 32 */
      ppu->vram[ppu->vram_addr & 0x3fffu] = value;
      ppu->vram_addr = (uint16_t)(ppu->vram_addr + ppu_data_increment(ppu));
      break;
    default: break; /* $2002 is read-only; $2004 OAM data comes later */
  }
}


uint8_t ppu_read(Ppu *ppu, uint8_t reg) {
  switch (reg & 0x7u) {
    case 2: { /* $2002: observe status, clear vblank and the write latch */
      uint8_t status = (uint8_t)(ppu->vblank ? 0x80u : 0x00u);
      ppu->vblank = false;
      ppu->latch = false;
      return status;
    }
    case 7: { /* $2007: reads are buffered one access behind. The palette
               * region ($3F00+) reads immediately on hardware; that special
               * case comes with palette work. */
      uint8_t buffered = ppu->read_buffer;
      ppu->read_buffer = ppu->vram[ppu->vram_addr & 0x3fffu];
      ppu->vram_addr = (uint16_t)(ppu->vram_addr + ppu_data_increment(ppu));
      return buffered;
    }
    default:
      return 0;
  }
}
