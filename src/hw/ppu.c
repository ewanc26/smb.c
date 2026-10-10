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
        ppu->vram_addr = (uint16_t)(value << 8);
      }
      ppu->latch = !ppu->latch;
      break;
    default: break; /* $2002 is read-only; $2004/$2007 data paths come later */
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
    case 7:
      return ppu->read_buffer;
    default:
      return 0;
  }
}
