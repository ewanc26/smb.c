/* hw/ppu.c — see hw/ppu.h for the contract.
 *
 * Failure semantics: register indices are masked to 0-7, matching the NES's
 * mirrored decoding of $2000-$3FFF; out-of-range callers cannot corrupt
 * adjacent state.
 */
#include "ppu.h"

#include <stddef.h>

void ppu_init(Ppu *ppu) {
  *ppu = (Ppu){0};
}

void ppu_set_vblank(Ppu *ppu, bool active) {
  ppu->vblank = active;
}

void ppu_set_chr(Ppu *ppu, const uint8_t *chr) {
  ppu->chr = chr;
}

void ppu_oam_dma(Ppu *ppu, const uint8_t *page) {
  for (unsigned i = 0; i < 0x100u; i++) {
    ppu->oam[(ppu->spr_addr + i) & 0xffu] = page[i];
  }
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

/* Map a 14-bit PPU address onto its actual storage byte in the flat 16KB
 * image, per the donor cartridge (vertical mirroring / horizontal
 * arrangement per its iNES header):
 *   - name tables ($2000-$2FFF): A11 decodes to the duplicate, A10 selects
 *     the physical 1KB bank, so $2800/$2C00 alias $2000/$2400;
 *   - the bus mirrors $3000-$3EFF back onto $2000-$2EFF (only PA13
 *     distinguishes them), then the name-table rule applies;
 *   - palette RAM ($3F00-$3FFF) occupies 32 bytes: address bits 5+ are
 *     ignored, so $3F20-$3FFF alias $3F00-$3F1F. (The additional backdrop
 *     mirroring of $3F00/04/08/0C into $3F10/14/18/1C is not modelled; the
 *     game writes the same value to both.)
 * The rest of the space ($0000-$1FFF) is stored flat; the port keeps its
 * CHR tables separately in src/data/chr. */
static uint16_t ppu_vram_addr(uint16_t addr) {
  addr &= 0x3fffu;
  if (addr >= 0x3f00u) {
    return (uint16_t)(0x3f00u | (addr & 0x1fu));
  }
  if (addr >= 0x3000u) {
    addr = (uint16_t)(addr & 0x2fffu); /* $3000-$3EFF <-> $2000-$2EFF */
  }
  if (addr >= 0x2000u) {
    addr = (uint16_t)(0x2000u | (addr & 0x07ffu)); /* drop A11, keep A10 */
  }
  return addr;
}

void ppu_write(Ppu *ppu, uint8_t reg, uint8_t value) {
  switch (reg & 0x7u) {
    case 0: ppu->ctrl1 = value; break;
    case 1: ppu->ctrl2 = value; break;
    case 3: ppu->spr_addr = value; break;
    case 4: /* $2004: write at the OAM address, then increment */
      ppu->oam[ppu->spr_addr] = value;
      ppu->spr_addr = (uint8_t)(ppu->spr_addr + 1u);
      break;
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
      ppu->vram[ppu_vram_addr(ppu->vram_addr)] = value;
      ppu->vram_addr = (uint16_t)(ppu->vram_addr + ppu_data_increment(ppu));
      break;
    default: break; /* $2002 is read-only */
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
    case 7: { /* $2007: reads outside the palette are buffered one access
               * behind. The palette region ($3F00+) returns its byte
               * immediately — no priming read — and the read buffer is
               * filled with the byte "underneath" (the $2Fxx bus mirror of
               * the name tables), which a later buffered read can observe. */
      uint16_t addr = (uint16_t)(ppu->vram_addr & 0x3fffu);
      if (addr >= 0x3f00u) {
        uint8_t palette = ppu->vram[ppu_vram_addr(addr)];
        ppu->read_buffer =
            ppu->vram[ppu_vram_addr((uint16_t)(addr & 0x2fffu))];
        ppu->vram_addr = (uint16_t)(addr + ppu_data_increment(ppu));
        return palette;
      }
      uint8_t buffered = ppu->read_buffer;
      /* the pattern tables read from the cartridge's CHR when one is
       * nominated; the flat image answers otherwise */
      ppu->read_buffer = (addr < 0x2000u && ppu->chr != NULL)
                             ? ppu->chr[addr]
                             : ppu->vram[ppu_vram_addr(addr)];
      ppu->vram_addr = (uint16_t)(addr + ppu_data_increment(ppu));
      return buffered;
    }
    case 4: /* $2004: read the byte at the OAM address, no increment */
      return ppu->oam[ppu->spr_addr];
    default:
      return 0;
  }
}
